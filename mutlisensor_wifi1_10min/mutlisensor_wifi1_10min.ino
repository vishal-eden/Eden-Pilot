#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <stdlib.h>  // strtoull

char ssid[] = "TP-Link_CF6C";
char pass[] = "33518886";

uint64_t receivedTime = 0;     // epoch ms received from Python
uint64_t epoch_arrival = 0;    // millis() when epoch was received

int status = WL_IDLE_STATUS;
WiFiServer server(80);


//data structures to hold series of flow,tds, ph, and temperature analogdata values
struct FlowData { float flow; };
struct TdsData  { float tds;  };
struct PhData   { float ph;   };
struct TempData { float temp; };

//initialize analog pins for data collection
const int tempdig  = 8;
const int tempdig2 = 9;
const int flowdig  = 2;
const int flowdig2 = 3;
const int phdig    = 6;
const int phdig2   = 7;
const int tdsdig   = 4;
const int tdsdig2  = 5;

const int phpin    = A2;
const int phpin2   = A2;
const int flowpin  = A2;
const int flowpin2 = A2;
const int temppin  = A2;
const int temppin2 = A2;
const int tdspin   = A2;
const int tdspin2  = A2;

const int flow_samples = 20;
const int tds_samples  = 20;
const int ph_samples   = 20;
const int temp_samples = 20;

const int PASSES_PER_BURST = 20;
const unsigned long BURST_INTERVAL_MS = 300000UL; // 10 min
unsigned long marker=0;

FlowData flow_log[PASSES_PER_BURST];
FlowData flow_log1[PASSES_PER_BURST];
TdsData  tds_log[PASSES_PER_BURST];
TdsData  tds_log1[PASSES_PER_BURST];
PhData   ph_log[PASSES_PER_BURST];
PhData   ph_log1[PASSES_PER_BURST];
TempData temp_log[PASSES_PER_BURST];
TempData temp_log1[PASSES_PER_BURST];
uint64_t sample_times[PASSES_PER_BURST];

bool schedule_started = false;
bool burst_ready = false;
unsigned long last_burst_start_ms = 0;
unsigned long lastReconnectAttempt = 0;
uint64_t timeElapsed = 0;
uint64_t burst_count = 0;

Adafruit_MPU6050 mpu;

FlowData loop_flow(int pin, int digital);
TdsData loop_tds(int pin, int digital);
PhData loop_ph(int pin, int digital);
TempData loop_temp(int pin, int digital);

uint64_t currentEpochMs() {
  if (receivedTime == 0) return 0;
  uint64_t now_ms = (uint64_t)millis();
  uint64_t delta = now_ms - epoch_arrival;
  return receivedTime + delta;
}

void collect_one_pass(int i) {
  flow_log[i]  = loop_flow(flowpin,  flowdig);
  flow_log1[i] = loop_flow(flowpin2, flowdig2);

  ph_log[i]    = loop_ph(phpin,  phdig);
  ph_log1[i]   = loop_ph(phpin2, phdig2);

  temp_log[i]  = loop_temp(temppin,  tempdig);
  temp_log1[i] = loop_temp(temppin2, tempdig2);

  tds_log[i]   = loop_tds(tdspin,  tdsdig);
  tds_log1[i]  = loop_tds(tdspin2, tdsdig2);

  // Each pass gets its own timestamp
  sample_times[i] = currentEpochMs();
}

void collect_burst() {
  for (int i = 0; i < PASSES_PER_BURST; i++) {
    collect_one_pass(i);
  }
  burst_ready = true;
  burst_count++;

  Serial.print("Completed burst #");
  Serial.println((unsigned long long)burst_count);
}

FlowData loop_flow(int pin, int digital) {
  digitalWrite(digital, HIGH);
  unsigned long start = millis();
  while (millis() - start < 250) {}

  int sum = 0;
  for (int i = 0; i < flow_samples; ++i) {
    sum += analogRead(pin);
  }

  FlowData d;
  d.flow = sum * (1.0f / flow_samples);

  unsigned long endWait = millis();
  while (millis() - endWait < 250) {}

  digitalWrite(digital, LOW);
  return d;
}

TempData loop_temp(int pin, int digital) {
  digitalWrite(digital, HIGH);
  unsigned long start = millis();
  while (millis() - start < 250) {}

  int sum = 0;
  for (int i = 0; i < temp_samples; ++i) {
    sum += analogRead(pin);
  }

  TempData d;
  d.temp = sum * (1.0f / temp_samples);

  unsigned long endWait = millis();
  while (millis() - endWait < 250) {}

  digitalWrite(digital, LOW);
  return d;
}

TdsData loop_tds(int pin, int digital) {
  digitalWrite(digital, HIGH);
  unsigned long start = millis();
  while (millis() - start < 250) {}

  int sum = 0;
  for (int i = 0; i < tds_samples; ++i) {
    sum += analogRead(pin);
  }

  TdsData d;
  d.tds = sum * (1.0f / tds_samples);

  unsigned long endWait = millis();
  while (millis() - endWait < 250) {}

  digitalWrite(digital, LOW);
  return d;
}

PhData loop_ph(int pin, int digital) {
  digitalWrite(digital, HIGH);
  unsigned long start = millis();
  while (millis() - start < 250) {}

  int sum = 0;
  for (int i = 0; i < ph_samples; ++i) {
    sum += analogRead(pin);
  }

  PhData d;
  d.ph = sum * (1.0f / ph_samples);

  unsigned long endWait = millis();
  while (millis() - endWait < 250) {}

  digitalWrite(digital, LOW);
  return d;
}

void setup() {
  pinMode(flowdig, OUTPUT);
  pinMode(flowdig2, OUTPUT);
  pinMode(tdsdig, OUTPUT);
  pinMode(tdsdig2, OUTPUT);
  pinMode(phdig, OUTPUT);
  pinMode(phdig2, OUTPUT);
  pinMode(tempdig, OUTPUT);
  pinMode(tempdig2, OUTPUT);


  //ensure relay switches are closed
  digitalWrite(flowdig, LOW);
  digitalWrite(flowdig2, LOW);
  digitalWrite(tdsdig, LOW);
  digitalWrite(tdsdig2, LOW);
  digitalWrite(phdig, LOW);
  digitalWrite(phdig2, LOW);
  digitalWrite(tempdig, LOW);
  digitalWrite(tempdig2, LOW);

  Serial.begin(9600);
  unsigned long serialStart = millis();
  while (!Serial && millis() - serialStart < 5000) {}

  
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module not found!");
    while (true) {}
  }
  //log into wifi network
  status = WiFi.begin(ssid, pass);
  unsigned long wifiStart = millis();
  while (status != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    status = WiFi.status();
    if (millis() - wifiStart > 20000) {
      Serial.println("\nWiFi connect timed out");
      while (true) {}
    }
  }

  server.begin();
  Wire.begin();

  Serial.println();
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Wi-Fi reconnect
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = millis();
      WiFi.end();
      WiFi.begin(ssid, pass);
    }
  }

  // Every 10 minutes, do a burst of 20 passes, then reset timer
  if (schedule_started) {
    timeElapsed = (uint64_t)(millis() - last_burst_start_ms);

    if (timeElapsed >= BURST_INTERVAL_MS+marker) {
      collect_burst();
      last_burst_start_ms = millis();   // reset timer after burst
      timeElapsed = 0;
    }
  }

  WiFiClient client = server.available();
  if (!client) return;

  String reqLine = client.readStringUntil('\n');
  reqLine.trim();

  int contentLength = 0;

  while (client.connected()) {
    String h = client.readStringUntil('\n');
    h.trim();

    if (h.length() == 0) break;

    if (h.startsWith("Content-Length:")) {
      contentLength = h.substring(15).toInt();
    }
  }

  bool isGetData = reqLine.startsWith("GET /data");
  bool isPostTime = reqLine.startsWith("POST /") || reqLine.startsWith("POST /data");

  if (isGetData) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain; charset=utf-8");
    client.println("Connection: close");
    client.println();

    if (!burst_ready) {
      client.println("No burst ready yet");
      client.stop();
      return;
    }

    client.print("burst_count="); client.println((unsigned long long)burst_count);

    for (int i = 0; i < PASSES_PER_BURST; i++) {
      client.print("sample_index="); client.println(i);

      client.print("flow=");  client.println(flow_log[i].flow);
      client.print("flow1="); client.println(flow_log1[i].flow);

      client.print("ph=");    client.println(ph_log[i].ph);
      client.print("ph1=");   client.println(ph_log1[i].ph);

      client.print("temp=");  client.println(temp_log[i].temp);
      client.print("temp1="); client.println(temp_log1[i].temp);

      client.print("tds=");   client.println(tds_log[i].tds);
      client.print("tds1=");  client.println(tds_log1[i].tds);

      client.print("time=");  client.println((unsigned long long)sample_times[i]);
      client.println("---");
    }

    client.stop();
    return;
  }

  if (isPostTime) {
    String body = "";
    unsigned long start = millis();

    while ((int)body.length() < contentLength && (millis() - start) < 2000) {
      while (client.available() && (int)body.length() < contentLength) {
        body += (char)client.read();
      }
    }

    int eq = body.indexOf('=');
    if (eq > 0) {
      const char* p = body.c_str() + eq + 1;
      receivedTime = strtoull(p, nullptr, 10);
      epoch_arrival = (uint64_t)millis();

      if (!schedule_started) {
        schedule_started = true;
        last_burst_start_ms = millis();  // timer starts now
        timeElapsed = 0;
        burst_ready = false;
      }

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain; charset=utf-8");
      client.println("Connection: close");
      client.println();
      client.println("OK");
      client.stop();
      return;
    }
  }

  client.println("HTTP/1.1 400 Bad Request");
  client.println("Content-Type: text/plain; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.println("Bad request");
  client.stop();
}

