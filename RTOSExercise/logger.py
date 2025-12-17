#!/usr/bin/env python3
import os
import subprocess
import sys
import threading
from dataclasses import dataclass
from datetime import datetime
import csv
from collections import Counter



# run "export.sh" inside a shell and get the resulting environment
cmd = 'bash -c "source ./esp/esp-idf/export.sh > /dev/null 2>&1 && env"'
result = subprocess.run(cmd, shell=True, capture_output=True, text=True)

# start with your current environment
new_env = os.environ.copy()

# update with entries from the sourced script
for line in result.stdout.splitlines():
    if "=" in line:
        key, value = line.split("=", 1)
        new_env[key] = value

print("Environment setup complete")
print("Starting idf.py monitor...\n")
print("=" * 60)
print("CAPTURED OUTPUT (all output from idf.py monitor will be captured and reprinted):")
print("=" * 60)
sys.stdout.flush()


identifier_dict = {
    "qs":  "traceQUEUE_SEND",
    "qsf": "traceQUEUE_SEND_FAILED",
    "qsi": "traceQUEUE_SEND_FROM_ISR",
    "qsif":"traceQUEUE_SEND_FROM_ISR_FAILED",
    "qr":  "traceQUEUE_RECEIVE",
    "qrf": "traceQUEUE_RECEIVE_FAILED",
    "qri": "traceQUEUE_RECEIVE_FROM_ISR",
    "qrif":"traceQUEUE_RECEIVE_FROM_ISR_FAILED",

    "t+":  "traceTASK_INCREMENT_TICK",

    "tc":  "traceTASK_CREATE",
    "tcf": "traceTASK_CREATE_FAILED",
    "td":  "traceTASK_DELETE",

    "ts":  "traceTASK_DELAY",
    "tsu": "traceTASK_DELAY_UNTIL",

    "ti":  "traceTASK_SWITCHED_IN",
    "to":  "traceTASK_SWITCHED_OUT"
}





@dataclass
class TraceLog:
    log_type: str = "undefined"
    tick: int = -1 
    timestamp_us: int = -1
    queue_handle: int = -1
    block_time: int = -1
    task_handle:int = -1
    identifier:str = -1
    new_tick:int = -1
    name: str = ""


def parse_hex_or_none(s: str):
    if s == "-" or s == "":
        return None
    return int(s, 16)

def parse_int_or_none(s: str):
    if s == "-" or s == "":
        return None
    return int(s)

def parse_trace_line(line: str):
    parts = line.strip().split()
    if len(parts) != 8:
        return None  # not a trace line

    tok, tick, us, task, queue, wait, newtick, name = parts

    # Accept short tokens, and also accept long switched-in/out if you keep those
    if tok not in identifier_dict and tok not in ("traceTASK_SWITCHED_IN", "traceTASK_SWITCHED_OUT"):
        return None

    return TraceLog(
        log_type=identifier_dict.get(tok, tok),  # expand token or keep long form
        tick=parse_int_or_none(tick) or -1,
        timestamp_us=parse_int_or_none(us) or -1,
        task_handle=parse_hex_or_none(task) or -1,
        queue_handle=parse_hex_or_none(queue) or -1,
        block_time=parse_int_or_none(wait) or -1,
        new_tick=parse_int_or_none(newtick) or -1,
        name = name
    )

CSV_COLUMNS = [
    "eventtype",
    "tick",
    "timestamp_us",
    "taskid",
    "queue_handle",
    "block_time",
    "new_tick",
    "name",
]

def export_to_csv(logs, filename):
    with open(filename, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=CSV_COLUMNS)
        writer.writeheader()

        for log in logs:
            writer.writerow({
                "eventtype": log.log_type,
                "tick": log.tick if log.tick != -1 else "",
                "timestamp_us": log.timestamp_us if log.timestamp_us != -1 else "",
                "taskid": f"{log.task_handle:08x}" if log.task_handle != -1 else "",
                "queue_handle": f"{log.queue_handle:08x}" if log.queue_handle != -1 else "",
                "block_time": log.block_time if log.block_time != -1 else "",
                "new_tick": log.new_tick if log.new_tick != -1 else "",
                "name": log.name,

            })


# Check for tools that can force output through pipes
# On macOS, script command syntax is: script [-a] [-e] [-F] [-k] [-p] [-q] [-r] [-t time] [file [command ...]]
use_unbuffer = subprocess.run(["which", "unbuffer"], capture_output=True).returncode == 0
use_stdbuf = subprocess.run(["which", "stdbuf"], capture_output=True).returncode == 0
use_script = subprocess.run(["which", "script"], capture_output=True).returncode == 0

if use_unbuffer:
    # Use 'unbuffer' from expect package (best option)
    cmd_line = 'unbuffer idf.py monitor'
elif use_script:
    # Use 'script' command - macOS syntax: script [-a] file [command ...]
    # -a = append, -q = quiet (no start/end messages)
    cmd_line = 'script -q /dev/null idf.py monitor'
elif use_stdbuf:
    # Use 'stdbuf' to force line buffering
    cmd_line = 'stdbuf -oL -eL idf.py monitor'
else:
    # Fallback: use shell redirection with unbuffered Python reading
    # This should work but some output might still go directly to terminal
    cmd_line = 'idf.py monitor 2>&1'

print(f"Using command: {cmd_line}\n")
sys.stdout.flush()

# Run the command and capture ALL output
process = subprocess.Popen(
    cmd_line,
    shell=True,
    env=new_env,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,  # Merge stderr into stdout
    text=True,
    bufsize=1,  # Line buffered
    universal_newlines=True
)
logs = []

def read_and_process(pipe):
    """Read from pipe and immediately reprint (this is the key - we capture then reprint)"""
    try:
        while True:
            line = pipe.readline()
            if not line:  # EOF
                break
            log = parse_trace_line(line)
            if log is not None:
                logs.append(log)
            #print(logs)

    except Exception as e:
        print(f"\nError reading output: {e}\n", file=sys.stderr, flush=True)

# Start thread to read and reprint output
output_thread = threading.Thread(target=read_and_process, args=(process.stdout,), daemon=True)
output_thread.start()



try:
    # Wait for process to complete
    process.wait()
    # Give thread a moment to finish reading remaining output
    output_thread.join(timeout=2)
    timestamp = datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
    csv_name = f"log_{timestamp}.csv"   
    counts = Counter(log.log_type for log in logs)
    print("\nLog entry counts:")
    for k, v in counts.items():
        print(f"  {k}: {v}")
    export_to_csv(logs, csv_name)
    
except KeyboardInterrupt:
    print("\n\nStopping monitor...")
    process.terminate()
    process.wait()
    output_thread.join(timeout=1)
    sys.exit(0)