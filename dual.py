import os
import re
import time
import requests
import influxdb_client
from dotenv import load_dotenv
from influxdb_client import Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

ARDUINO_IP = "192.168.0.102"
load_dotenv()

post_url = f"http://{ARDUINO_IP}"
data_url = f"http://{ARDUINO_IP}/data"

INFLUX_URL = os.getenv("INFLUX_URL")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")
INFLUX_ORG = os.getenv("INFLUX_ORG")

client = influxdb_client.InfluxDBClient(
    url=INFLUX_URL,
    org=INFLUX_ORG,
    token=INFLUX_TOKEN
)
write_api = client.write_api(write_options=SYNCHRONOUS)
bucket = "giga"

session = requests.Session()
records = []

number = re.compile(r'accel=([-+]?\d+(?:\.\d+)?)')
timeVal = re.compile(r'time=([0-9]+)')

def send_time():
    epoch_ms = int(time.time() * 1000)
    payload = f"time={epoch_ms}"

    r = session.post(post_url, data=payload, timeout=5)
    print("POST response:", r.text)
    print("Sent epoch_ms:", epoch_ms)

def get_data():
    global records

    r = session.get(data_url, stream=True, timeout=10)

    if r.status_code != 200:
        print("Request failed:", r.status_code)
        return

    for raw_line in r.iter_lines():
        if not raw_line:
            continue

        line = raw_line.decode("utf-8").strip()
        print(line)

        time_match = re.search(timeVal, line)
        number_match = re.search(number, line)

        if number_match and time_match:
            accel_val = float(number_match.group(1))
            time_val = int(time_match.group(1))

            point = (
                Point("accel")
                .field("accel", accel_val)
                .time(time_val, WritePrecision.MS)
            )
            records.append(point)

            if len(records) >= 200:
                write_api.write(bucket=bucket, org=INFLUX_ORG, record=records)
                records = []

def main():
    send_time()
    print("Starting data requests...\n")

    try:
        while True:
            get_data()
    finally:
        if records:
            write_api.write(bucket=bucket, org=INFLUX_ORG, record=records)
        session.close()
        client.close()

if __name__ == "__main__":
    main()