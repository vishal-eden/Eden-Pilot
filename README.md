This repo contains code for the sensor telemetry setup to acquire data measuring the preformance of the 2026 pilot tests for the Eden Tech reverse osmosis centrifuge (ROC). The programs are written for a server-client architecture between Arduino remote devices and a python client. Data is collected by the Arduinos before being wirelessly transmitted to python listeners running on Mac Mini computers which then push the data streams to a time-series database on influx db where live and historical data can be visualized and analyzed.




**Running Codes**

All of these programs are located locally in /Desktop/Eden Repo/ directory. To run these programs, simply type in the command line python3 [file name].

For those unfamiliar with command line syntax, the two needed commands are cd and ls. cd is used to change the working directory. ls is used to retrieve the contents of the files within the current directory. To run the programs, just open the Terminal application and type into the command line: cd Desktop. Then cd Eden\ Repo. 

**Running Strain Guage Programs.**

The two files are called sg_stream_handler.py and sg_stream_handler1.py. 

**Running Multisensor Program**
The python file is multisensor_handler_giga.py.

**Multisensor Board:  ** The multi sensor program runs on the Arduino Giga architecture and performs data collection over wi-fi at a pre-specified time interval (usually 5/10 minutes).

The analog metrics collected are:

-flow (2 sensors)


-ph (2 sensors)

-temperature (2 sensors)

-tds (2 sensors)

-rpm (future)

Sensor Collection Functions:<br/>
loop_flow(analog, digital) <br/>
loop_ph (analog, digital)
loop_temp (analog, digital)
loop_tds( analog, digital)


Each function:
1). Opens relay to enable respective analog pin access using digital signal from power.
2). Wait 250 ms for settling.
3). Take several readings from analog pin (number of readings is configurable).
4). Averaging those readings.
5). Wait another 250 ms for settling.
6). Close relay 
