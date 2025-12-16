<a id="readme-top"></a>

# Project for Real-Time Operating Systems Design and Implementation 25/26, TU Dortmund - Build Your Own Debug Tool

<!-- ABOUT THE PROJECT -->
## About The Project

This project implements several traces for debugging based on FreeRTOS. These traces are stored in a buffer, that prints its contents in the console once it is full. These traces can be processed using a Python script, which prints a full log in a .csv format, and a task allocation diagram can be built from this final log.

Each trace has several parameters associated, namely the tick it was registered in, a timestamp based on the system clock and a task handle or name. There are other specific parameters shown for each particular trace.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- BUILT WITH -->
## Built with

* C
* C++
* Python

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- GETTING STARTED -->
## Getting Started

To run this program correctly, follow the instructions on this [https://github.com/TU-Dortmund-CS-LS12-DAES-teaching/RTOSExercise](base repo).
Install the correct required system packages and set up your system correctly, and run `local_setup.sh`.

After that, on the installation folder, run `. ./esp/esp-idf/export.sh`, and `idf.py build` should result in a valid setup.

For the Python scripts, get their required packages by running `pip install -r requirements.txt`.

# Traces
To see the traces, run `idf.py flash monitor`. `idf.py monitor` should be run for all subsequent runs. A test program is run, with messages passing from several 'senders' to a 'reciever', and which are then captured by another 'logger' task.

# Information Extraction and Visualisation
The full log can be recovered by calling `python logger.py` after the first time running the program. 

Then you can see the task diagram with `python visualize.py [your-log-name].csv`.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CHANGES TO FREERTOS-->
## Modifications to FreeRTOS

Changes are made to `FreeRTOSConfig.h` that implement the following traces:
* traceQUEUE_RECEIVE
* traceQUEUE_RECEIVE_FAILED
* traceQUEUE_RECEIVE_FROM_ISR
* traceQUEUE_RECEIVE_FROM_ISR_FAILED
* traceQUEUE_SEND
* traceQUEUE_SEND_FAILED
* traceQUEUE_SEND_FROM_ISR
* traceQUEUE_SEND_FROM_ISR_FAILED

* traceTASK_INCREMENT_TICK

* traceTASK_DELAY
* traceTASK_DELAY_UNTIL

* traceTASK_SWITCHED_IN
* traceTASK_SWITCHED_OUT

(Not working as of writing)
* traceTASK_CREATE
* traceTASK_CREATE_FAILED
* traceTASK_DELETE

New files are added, namedly `trace_log.c` and `trace_log.h`, which run the logging and console printing process.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTACT -->
## Contact

Miguel Oliveira - Student ID No. 278982 - [miguel.oliveira@tu.dortmund.de](mailto:miguel.oliveira@tu.dortmund.de)

Ilmari Hirvonen - 

Project Link: [https://github.com/Veni4/RTOS_2526_LabWork](https://github.com/Veni4/RTOS_2526_LabWork)

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

* Best-README-Template [https://github.com/othneildrew/Best-README-Template](https://github.com/othneildrew/Best-README-Template)
* FreeRTOS [https://github.com/FreeRTOS/FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel)
* TU Dortmund's RTOS Course's beginner exercise [https://github.com/TU-Dortmund-CS-LS12-DAES-teaching/RTOSExercise](https://github.com/TU-Dortmund-CS-LS12-DAES-teaching/RTOSExercise)

<p align="right">(<a href="#readme-top">back to top</a>)</p>