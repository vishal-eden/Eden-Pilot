This repo contains code for the sensor telemetry setup to acquire data measuring the preformance of the 2026 pilot tests for the Eden Tech reverse osmosis centrifuge (ROC). The programs are written for a server-client architecture between Arduino remote devices and a python client. Data is collected by the Arduinos before being wirelessly transmitted to python listeners running on Mac Mini computers which then push the data streams to a time-series database on influx db where live and historical data can be visualized and analyzed.




**Running Code**

All of these programs are located locally in /Desktop/Eden Repo/ directory. To run these programs, simply type in the command line python3 [file name].

For those unfamiliar with command line syntax, the two needed commands are cd and ls. cd is used to change the working directory. ls is used to retrieve the contents of the files within the current directory. To run the programs, just open the Terminal application and type into the command line: cd Desktop. Then cd Eden\ Repo. 

If running python files on a new system (not the mac minis in the cuby or test site), note that the programs will only run correctly if there is a .env file in the directory

**Ending Existing Program**
Control+C On cli interface where program seems to be running.

**Running Strain Guage Programs.**
The two files are called sg_stream_handler.py and sg_stream_handler1.py. 

**Running Multisensor Program**
The python file is multisensor_handler_giga.py.

**Running Accelerometer Program**
The python file is dual.py<br/>
Arduino files are dualm4_new.ino and dualm7_new.ino (one file for each core)

**IP Address Change**

Python connects to remote devices by querying IP Address. Unfortunately, IP addresses aren't static but dynamically configured the moment a new device enteres the network. This means that if power cycling a device or flashing a new program to the device may trigger an IP adress change, since the device is momentarily disconnecting from the network befoe reconnecting. To view a device's IP address, access the network admin portal. This can be completed by either accessing tplinkwifi.net or use the local area network ip address of the router as a URL for more direct access. Since the router networks are LAN-based, they shouldn't change. Unless swapped with another router, the ip address for the netwok at the test site should remain 192.168.0.1.<br/>

To log into the local router admin portal, use X43Apples22 as the password. Once logged in, select the "Advanced" option on the top navigation bar. Then on the left side of the screen, select DHCP Server and scroll down to view the ip addresses by device.

Then copy the IP address on this list and paste it on the python file corresponding to the program of interest. You should paste this ip address as the value of the variable named either GIGA_URL or UNO_URL.

**Influx DB**
Data collected by the Arduinos will not be saved unless Influx DB is runing. Typically it is running. You can verify its presence by accessing the url localhost:8086. If the influx ui doesn't pop up, that should indicate that Influx isn't actually runnng. To fix this, open Docker Desktop. Once open, skip the "Welcome to Docker" page. Once this is done, the Docker Engine should be running. Then, open a new terminal instance and anchor the working directory using cd Desktop/Eden\ Repo. Then run the command docker compose up to spin up a new docker container with influx attached.

**Multisensor Board:  ** The multi sensor program runs on the Arduino Giga architecture and performs data collection over wi-fi at a pre-specified time interval (usually 5/10 minutes).

The analog metrics collected are:<br/>
-flow (2 sensors) <br/>
-ph (2 sensors) <br/>
-temperature (2 sensors) <br/>
-tds (2 sensors) <br/>
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

**Database Capacity Planning**

						Accelerometer Program
Influx Point:
    * Example: accel accel=1.23 1700000000000
        * Tag: 5 bytes for “accel”
        * Measurement Name: 5 bytes for “access”
        * Acceleration Value:
            * As 3-4 digit int that is converted to string as line protocol
                * 3-4 bytes
        * Unix Timestamp (ms since 1970): 
            * 13 bytes
        * Total Bytes Per Point: 13 + 4 +5 + 5 =27
    * Sampling Rate: 400 Hz
    * Bytes Per Second: 27 * 400=10,800 bps
    * Bytes Per Hour: 10,800 bps * 60 = 648,000 bph (bytes per hour)
    * Bytes Per Day: 648,000*24  = 15,552,000 
    * Bytes Per Year:  =15,552,000 *365= 5.67658 e 9, (over 5 bill bytes per year)
                        * Over 5 GB

