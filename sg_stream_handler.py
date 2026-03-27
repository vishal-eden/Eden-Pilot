import time
import requests
from datetime import datetime
import influxdb_client
from influxdb_client import Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
import re
import os
from dotenv import load_dotenv


load_dotenv() #load environmental variables
ts = time.time() #get current time
GIGA_URL = "http://192.168.0.101/data"
INFLUX_URL    = os.getenv("INFLUX_URL")
INFLUX_TOKEN  = os.getenv("INFLUX_TOKEN")
INFLUX_ORG    = os.getenv("INFLUX_ORG")
client = influxdb_client.InfluxDBClient(
        url=INFLUX_URL, org=INFLUX_ORG, token=INFLUX_TOKEN
    ) #create influx client
write_api = client.write_api(write_options=SYNCHRONOUS)  ##initializing api that actually sends data
bucket="bravo"

while True:
    r = requests.get(GIGA_URL) #get data from arduino giga
    print("GET:", r.status_code)
    print(r.text[:200])
    ts = time.time() #get time

    number=re.compile(r'(bridge_[0-9]+)=([0-9]+)') #format in which data will arrive
    packets=re.findall(number,r.text[:200]) #extract all strain guage values and bridge numbers from arduino payload
    tags=[]
    vals=[] #initialize data array that is sent to influx
    for i,j in enumerate(packets):# iterate through numbered straing guage
        tags.append("bravo "+j[0])
        vals.append(int(j[1]))# add all data from arduino payload to data array
    #if tag is not None and val is not None:
    data=Point("strain") #initialize influx point object
    for i in range(len(packets)):
        data=data.field(tags[i],vals[i]) # append point object with data and label
    print(ts)
    data=data.time(int(ts),WritePrecision.S)# append point object with time
    try:
        write_api.write(bucket=bucket, org=INFLUX_ORG, record=data) #send point object to influx
    except Exception as e:
        print("Influx write error:", e)

    #data=Point("strain").field("x",float(x.group(1))).field("y",float(y.group(1))).field("z",float(z.group(1))).time(int(t.group(1)),WritePrecision.US)
    dt=datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
    print(dt)
    print("bravo")
    time.sleep(0.2)
