#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <stdlib.h>  // strtoull

char ssid[] = "TP-Link_CF6C";
char pass[] = "33518886";

uint64_t receivedTime=0;
uint64_t epoch_arrival=0;

int status = WL_IDLE_STATUS;
WiFiServer server(80);


// struct RpmData {
//   float rpm;
//   float rps;
//   uint32_t ts;
// };  //

struct FlowData {
  float flow;
};
struct TdsData {
  float tds;
};

struct PhData{
  float ph;
};
struct TempData{
  float temp;
};

const int accel_buffer = 40;
const int number_of_magnets = 10;
//const int output_rpm = 2;

//const int flow_digital=3;
//const int tds_digital=2;
const int tempdig=8;
const int tempdig2=9;
const int flowdig=2;
const int flowdig2=3;
const int phdig=6;
const int phdig2=7;
const int tdsdig=4;
const int tdsdig2=5;

const int phpin = A6;
const int phpin2 = A7; 
const int flowpin = A2;
const int flowpin2 = A3;
const int temppin=A0;
const int temppin2=A1;
const int tdspin=A4;
const int tdspin2=A5;
const int hallpin = 4;
const int max_logs = 100;  // Size of full buffer


Adafruit_MPU6050 mpu;

static bool RTC_SYNCED=false;

FlowData flow_log[max_logs];
FlowData flow_log1[max_logs];
TdsData tds_log[max_logs];
TdsData tds_log1[max_logs];
PhData ph_log[max_logs];
PhData ph_log1[max_logs];
TempData temp_log[max_logs];
TempData temp_log1[max_logs];
//RpmData rpm_log[max_logs];


int sample_index = 0;
const long interval = 1000;
const int num_samples = 50;
const int flow_samples=20;
const int tds_samples=20;
const int ph_samples=20;
const int temp_samples=20;
const int samples = 50;
volatile bool RUN_FLAG = false;


FlowData loop_flow(int pin, int digital);
TdsData loop_tds(int pin,int digital);
PhData loop_ph(int pin, int digital);
TempData loop_temp(int pin, int digital);



 FlowData loop_flow(int pin, int digital) {
  digitalWrite(digital, HIGH); //open relay
  unsigned long start=millis();
  while(millis()-start<250);
  int sum = 0;
  for (int i = 0; i < flow_samples; ++i){ 
  sum += analogRead(pin); 
  }
  float avg_flow = sum * (1.0f / flow_samples);

  FlowData d;
  d.flow = avg_flow;
  unsigned long end=millis();

  while(millis()-end<250);


  Serial.println("time is");
  digitalWrite(digital,LOW); //close relay
  return d;
}

TempData loop_temp(int pin, int digital) {
  digitalWrite(digital, HIGH); //open relay
  unsigned long start=millis();
  while(millis()-start<250);
  int sum = 0;
  for (int i = 0; i < temp_samples; ++i){ 
  sum += analogRead(pin); 
  }
  float avg_temp = sum * (1.0f / temp_samples);
  TempData d;
  d.temp = avg_temp;
  unsigned long end=millis();

  while(millis()-end<250);

  digitalWrite(digital,LOW); //close relay
  return d;
}

TdsData loop_tds(int pin, int digital) {
  digitalWrite(digital,HIGH); //open relay
  unsigned long start=millis();
  while(millis()-start<250);
  int sum = 0;
  for (int i = 0; i < tds_samples; ++i) {
  sum += analogRead(pin);
  }
  float avg_tds = sum * (1.0f / tds_samples);

  TdsData d;
  d.tds = avg_tds;
  unsigned long end=millis();

  while(millis()-end<250);

  digitalWrite(digital,LOW); //close relay

  return d;

}

PhData loop_ph(int pin, int digital) {
  digitalWrite(digital,HIGH); //open relay
  unsigned long start=millis();
  while(millis()-start<250);
  int sum = 0;
  for (int i = 0; i < ph_samples; ++i) {
  sum += analogRead(pin);
  }
  float avg_ph = sum * (1.0f / ph_samples);

  PhData d;
  d.ph = avg_ph;
  unsigned long end=millis();

  while(millis()-end<250);
  digitalWrite(digital,LOW); //close relay

  return d;

}
// RpmData loop_rpm(int pin, int num_magnets) {
//   //File file = SD.open("/testing_multisensor/error.txt",FILE_WRITE);
  

//   int val = digitalRead(pin);
//   unsigned long t0u = millis();
//   unsigned long lastEdgeMs = millis();
//   int old = digitalRead(pin);
//   int cnt = 0;

//   while (cnt < num_magnets) {
//     if (millis() - lastEdgeMs >= 20) {
//      // file.println("Timeout: No magnet detected in 500 milliseconds.");
//       //Serial.println("Timeout: No magnet detected in 500 milliseconds.");
//       RpmData d;
//       //d.rpm = (seconds > 0) ? (cnt / seconds * 60.0f) : 0.0f;
//       d.rpm = 0;
//       d.ts = rtc.now().unixtime();  // <-- epoch seconds
//       return d;
//     }
//     unsigned long w0 = millis();
//     while (millis() - w0 < 1) { /* small wait window */
//     }

//     int val = digitalRead(pin);
//     if (!val && val != old) {
//       cnt++;
//       lastEdgeMs = millis();
//     }
//     old = val;
//   }

//   float seconds = (micros() - t0u) / 1000000.0f;
//   RpmData d;
//   //d.rpm = (seconds > 0) ? (cnt / seconds * 60.0f) : 0.0f;
//   d.rpm = (cnt / seconds * 60)/3;
//   d.ts = rtc.now().unixtime();  // <-- epoch seconds

//   //Serial.print("rpm is ");
//   //Serial.print(d.rpm);
//   return d;
// }



void setup() {

pinMode(flowdig, OUTPUT);
pinMode(flowdig2, OUTPUT);
pinMode(tdsdig, OUTPUT);
pinMode(tdsdig2, OUTPUT);
pinMode(phdig, OUTPUT);
pinMode(phdig2, OUTPUT);
pinMode(tempdig, OUTPUT);
pinMode(tempdig2, OUTPUT);

digitalWrite(flowdig, LOW);
digitalWrite(flowdig2, LOW);
digitalWrite(tdsdig, LOW);
digitalWrite(tdsdig2, LOW);

  status = WiFi.begin(ssid,pass);
  unsigned long t0 = millis();
  unsigned long start2 = millis();
  while (millis() - start2 < 2000);

  Serial.begin(9600); //serial begin (relatively low baud rate)
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module not found!");
    while (true) {}
  }
  while (status != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    status = WiFi.status();
    if (millis() - t0 > 20000) { // 20s timeout
      Serial.println("\nWiFi connect timed out");
      while (true) {}
    }
  }
server.begin();  

  Wire.begin(); //for i2c (clock)
  unsigned long start = millis();
  while (!Serial && millis() - start < 5000);

  delay(500);

}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print(WiFi.localIP());

  if (WiFi.status() != WL_CONNECTED) {
  WiFi.end();
  WiFi.begin(ssid, pass);
}

  WiFiClient client = server.available();
  if (!client) return;
  if (receivedTime!=0.0){
  String reqLine = client.readStringUntil('\n');
  reqLine.trim(); // removes \r too
  // --- Read and discard headers ---
  while (client.connected()) {
    String h = client.readStringUntil('\n');
  
    h.trim();
    if (h.length() == 0) break;
  }
  bool isGetData = reqLine.startsWith("GET /data");

  if (!isGetData) {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain; charset=utf-8");
    client.println("Connection: close");
    client.println();
    client.println("Not found");
    client.stop();
    return;

  }

    for (int i = 0; i < 1; ++i) {
  // collect
flow_log[sample_index]  = loop_flow(flowpin,  flowdig);
flow_log1[sample_index] = loop_flow(flowpin2, flowdig2);

ph_log[sample_index]    = loop_ph(phpin,  phdig);
ph_log1[sample_index]   = loop_ph(phpin2, phdig2);

temp_log[sample_index]  = loop_temp(temppin,  tempdig);
temp_log1[sample_index] = loop_temp(temppin2, tempdig2);

tds_log[sample_index]   = loop_tds(tdspin,  tdsdig);
tds_log1[sample_index]  = loop_tds(tdspin2, tdsdig2);
// print value + timestamp for each
    client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain; charset=utf-8");
  client.println("Connection: close");
  client.println();
Serial.println("HI");
client.print("flow=");
client.print(flow_log[sample_index].flow);


Serial.print("raw flow=");
Serial.println(flow_log[sample_index].flow);

client.print("\n");
Serial.println();
float flow_map1 = (flow_log1[sample_index].flow - 204.6f) / 136.4f;
client.print("flow1=");
client.print(flow_log1[sample_index].flow);

Serial.print("raw flow1=");
Serial.println(flow_log1[sample_index].flow);
client.print("\n");
float ph_map  = (ph_log[sample_index].ph  - 204.6f) / 58.457f;
client.print("ph=");
client.print(ph_log[sample_index].ph);
client.print("\n");
float ph_map1 = (ph_log1[sample_index].ph - 204.6f) / 58.457f;
client.print("ph1=");
client.print(ph_log1[sample_index].ph);

client.print("\n");

client.print("temp=");
float temp_map  = (temp_log[sample_index].temp  - 204.6f) / 3.860f;
client.print(temp_log[sample_index].temp);

Serial.print("temp is ");
Serial.println(temp_map);

Serial.print("raw temp is");
Serial.println(temp_log[sample_index].temp);


client.print("\n");

client.print("temp1=");
float temp_map1 = (temp_log1[sample_index].temp - 204.6f) / 3.860f;
Serial.print("raw temp1 is");
Serial.println(temp_log1[sample_index].temp);


client.print(temp_log1[sample_index].temp);

Serial.print("temp 1 is ");
Serial.println(temp_map1);




client.print("\n");

client.print("tds=");
float tds_map  = (tds_log[sample_index].tds  - 204.6f) / 0.0016368f;
client.print(tds_log[sample_index].tds);
Serial.print("tds is ");
Serial.println(tds_map);

Serial.println("raw tds is");
Serial.println(tds_log[sample_index].tds);


client.print("\n");

client.print("tds1=");
float tds_map1 = (tds_log1[sample_index].tds - 204.6f) / 0.0016368f;
client.print(tds_log1[sample_index].tds);
Serial.print("tds 1 is ");
Serial.println(tds_map1);

Serial.println("raw tds1 is");
Serial.println(tds_log1[sample_index].tds);

client.print("\n");

client.print("time=");
uint64_t now = (uint64_t) millis();
uint64_t delta = now - epoch_arrival;   // wrap-safe!
uint64_t ts = receivedTime + (uint64_t)delta;
client.print(ts);
client.print("\n");

client.print("---\n");

  client.println();
  client.println("OK");
client.stop();

      sample_index++;
    }

    sample_index=0;
 

  }
  else{
        int contentLength = 0;
    // 1) Read headers until blank line
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      line.trim(); // removes \r and whitespace

      if (line.length() == 0) break; // end of headers

      if (line.startsWith("Content-Length:")) {
        contentLength = line.substring(15).toInt();
      }
    }

    // 2) Read body exactly contentLength bytes
    String body = "";
    unsigned long start = millis();
    while ((int)body.length() < contentLength && (millis() - start) < 2000) {
      while (client.available() && (int)body.length() < contentLength) {
        body += (char)client.read();
      }
    }

    // 3) Parse: "time=1735956934.12345"
    int eq = body.indexOf('=');
    if (eq > 0) {
  const char* p = body.c_str() + eq + 1;
  receivedTime = strtoull(p, nullptr, 10);   // <-- integer parse
      epoch_arrival=(uint64_t)millis();
    }
  }

  // Always respond to the HTTP client

client.println("HTTP/1.1 200 OK");
client.println("Content-Type: text/plain; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.println("OK");
client.stop();

  }

