#!/usr/bin/env python3

import argparse
import os
import re
import sys
import threading
import time
from collections import deque
from datetime import datetime, timezone, timedelta
import serial.tools.list_ports
import influxdb_client
import serial
from dotenv import load_dotenv
from influxdb_client import Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
import requests
from pathlib import Path
#

# ================== ENV ==================
load_dotenv()  #allows for environmental variables to be extracted from .env

port5=os.getenv("giga")
SER_BAUD = int(os.getenv("SER_BAUD", "9600"))
SER_TIMEOUT = float(os.getenv("SER_TIMEOUT", "2"))

INFLUX_URL = os.getenv("INFLUX_URL")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")
INFLUX_BUCKET = "default_bucket"
INFLUX_ORG = os.getenv("INFLUX_ORG")
bucket="default_bucket"
client = influxdb_client.InfluxDBClient(url=INFLUX_URL, org=INFLUX_ORG, token=INFLUX_TOKEN) #initialize influx client library 
write_api = client.write_api(write_options=SYNCHRONOUS) #initializing what actually sends data
last_write_ts=time.time()

GIGA_URL="http://192.168.0.100/data"
flow=re.compile(r'flow=([0-9]+.[0-9]+)')
flow1=re.compile(r'flow1=([0-9]+.[0-9]+)')
ph=re.compile(r'ph=([0-9]+.[0-9]+)')
ph1=re.compile(r'ph1=([0-9]+.[0-9]+)')
tds=re.compile(r'tds=([0-9]+.[0-9]+)')
tds1=re.compile(r'tds1=([0-9]+.[0-9]+)')
temp=re.compile(r'temp=([0-9]+.[0-9]+)')
temp1=re.compile(r'temp1=([0-9]+.[0-9]+)')
time1=re.compile(r'time=([0-9]+)')
try:
    r = requests.post(GIGA_URL, data={"time": int(time.time()*1000)})   #send current time to arduino
    print("POST:", r.status_code, repr(r.text))

    while True:
        r = requests.get(GIGA_URL)  #get 
        print("GET:", r.status_code)
        print(r.text)
        ts = time.time()
        flow_match=re.search(flow,r.text)
        flow1_match=re.search(flow1,r.text)
        ph_match=re.search(ph,r.text)
        ph1_match=re.search(ph1,r.text)
        temp_match=re.search(temp,r.text)
        temp1_match=re.search(temp1,r.text)
        tds_match=re.search(tds,r.text)
        tds1_match=re.search(tds1,r.text)
        time_match=re.search(time1,r.text)
        data=Point("Sensor") #initializes data point objecrt specifying bucket on influx
        for line in r.text.splitlines(): #iterate linre by line down GET response
            line = line.strip()
            if not line or line.startswith("---"):
                continue

            # expects exactly: "<name>=<value> ts=<timestamp>"
            m = re.match(r'^(flow1|flow|ph1|ph|tds1|tds|temp1|temp)=([-+]?\d+(?:\.\d+)?)$', line)
            if m:
                name = m.group(1) #exxtracts tag
                val  = float(m.group(2)) #eextracts field
                data=data.field(name,val) #append data point object with field and tag
                # handle sensor value
            else:
                t = re.match(r'^time=(\d+)$', line)
                if t:
                    ts_us = int(t.group(1)) #extracttimestamp
                    data=data.time(ts_us,WritePrecision.MS) #aappend data point with timestamp
                    write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=data) #api call to send data to db
                    data=Point("Sensor")
        
except requests.exceptions.RequestException as e:
    print("Request failed:", type(e).__name__, e)
