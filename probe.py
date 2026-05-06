import serial
from dotenv import load_dotenv
import os

port5=os.getenv("port5")

SER_BAUD = int(os.getenv("SER_BAUD", "9600"))
SER_TIMEOUT = float(os.getenv("SER_TIMEOUT", "2"))

s = serial.Serial(port5, SER_BAUD, timeout=SER_TIMEOUT, write_timeout=2)
while True:
    raw=s.readline()
    print(raw)


