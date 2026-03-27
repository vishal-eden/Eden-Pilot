#include "WiFiS3.h"

char ssid[] = "TP-Link_CF6C";
char pass[] = "33518886";
byte mac[6];

int status = WL_IDLE_STATUS;
WiFiServer server(80);

int count = 0;
int pulseDelay = 45;

// Digital pins powering each Wheatstone bridge
int bridgePins[12] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

static void printWifiStatus() {
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
}

void setup() {
  Serial.begin(115200);


  while (!Serial) {}

  for (int i = 0; i < 12; i++) {
    pinMode(bridgePins[i], OUTPUT);
    digitalWrite(bridgePins[i], LOW);
  }

  int n = WiFi.scanNetworks();
for (int i = 0; i < n; i++) {
  Serial.print(i);
  Serial.print(": ");
  Serial.print(WiFi.SSID(i));
  Serial.print(" (RSSI ");
  Serial.print(WiFi.RSSI(i));
  Serial.println(")");
}

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module not found!");
    while (true) {}
  }

  Serial.print("Connecting to WiFi SSID: ");
  Serial.println(ssid);

  // Connect to existing Wi-Fi (router)
  status = WiFi.begin(ssid,pass);

  // Wait for connection
  unsigned long t0 = millis();
  while (status != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    status = WiFi.status();
    if (millis() - t0 > 20000) { // 20s timeout
      Serial.println("\nWiFi connect timed out");
      while (true) {}
    }
  }

  Serial.println("\nWiFi connected!");
  printWifiStatus();

  server.begin();
  Serial.println("HTTP server started on port 80");
}

void loop() {
if (WiFi.status() != WL_CONNECTED) {

  status = WiFi.begin(ssid,pass);

  // Wait for connection
  unsigned long t0 = millis();
  while (status != WL_CONNECTED) {
      status = WiFi.begin(ssid,pass);

    delay(500);
    Serial.print(".");
    status = WiFi.status();
    if (millis() - t0 > 20000) { // 20s timeout
      Serial.println("\nWiFi connect timed out");
      while (true) {}

    }
  }
    server.begin();

  }

  Serial.println(WiFi.localIP());

  WiFiClient client = server.available();
  if (!client) return;

  // --- Read request line ---
  String reqLine = client.readStringUntil('\n');
  reqLine.trim(); // removes \r too
  Serial.println(reqLine);
  // --- Read and discard headers ---
  while (client.connected()) {
    String h = client.readStringUntil('\n');
  
    h.trim();
    if (h.length() == 0) break;
  }

  // Only respond to GET /data
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

  count++;

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain; charset=utf-8");
  client.println("Connection: close");
  client.println();

  client.print("count=");
  client.println(count);

  for (int i = 0; i < 12; i++) {
    digitalWrite(bridgePins[i], HIGH);
    delay(pulseDelay);

    int reading = analogRead(A1);

    client.print("bridge_");
    client.print(i + 1);
    client.print("=");
    client.println(reading);

    digitalWrite(bridgePins[i], LOW);
    delay(pulseDelay);
  }

  client.stop();
}
