#include <RPC.h>
#include <WiFi.h>
#include <string>
#include <cstdlib>

char ssid[] = "TP-Link_C579";
char pass[] = "72504543";

uint64_t lastId = 0;
bool firstSample = true;
uint64_t receivedCount = 0;

WiFiServer server(80);
uint64_t receivedTime = 0;

uint64_t sendTime() {
  return receivedTime;
} //send time from m7 to m4

uint64_t parseId(const std::string &line) {
  size_t start = line.find("id=");
  if (start == std::string::npos) return 0;

  start += 3;
  size_t end = line.find(',', start);

  std::string val;
  if (end == std::string::npos) {
    val = line.substr(start);
  } else {
    val = line.substr(start, end - start);
  }

  return strtoull(val.c_str(), nullptr, 10);
}

void setup() {
  Serial.begin(115200);

  RPC.begin();
  RPC.bind("sendTime", sendTime);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }

  Serial.print("WiFi connected, IP = ");
  Serial.println(WiFi.localIP());

  server.begin();

  LL_RCC_ForceCM4Boot();
}

void handlePostTime(WiFiClient &client, int contentLength) {
  String body = "";
  unsigned long start = millis();

  while ((int)body.length() < contentLength && (millis() - start) < 2000) {
    while (client.available() && (int)body.length() < contentLength) {
      body += (char)client.read();
    }
  }

  int eq = body.indexOf('=');
  if (eq > 0) {
    uint64_t epoch_ms = strtoull(body.substring(eq + 1).c_str(), nullptr, 10);
    receivedTime = epoch_ms;
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.println("OK");
  client.stop();
}

void handleStream(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain; charset=utf-8");
  client.println("Connection: close");
  client.println();

  int available = RPC.call("availableSamples").get().as<int>();
  int toSend = available;

  for (int i = 0; i < toSend; i++) {
    std::string line = RPC.call("getNextSampleLine").get().as<std::string>();

    if (line.empty()) {
      break;
    }

    uint64_t id = parseId(line);

    if (!firstSample && id != lastId + 1) {
      Serial.print("MISSED SAMPLE(S): expected ");
      Serial.print((unsigned long long)(lastId + 1));
      Serial.print(" but got ");
      Serial.println((unsigned long long)id);
    }

    firstSample = false;
    lastId = id;
    receivedCount++;

    client.println(line.c_str());

    if ((receivedCount % 200) == 0) {
      Serial.print("Last received ID: ");
      Serial.println((unsigned long long)id);
    }
  }

  uint64_t overwrites = RPC.call("getOverwriteCount").get().as<uint64_t>();
  Serial.print("M4 overwriteCount = ");
  Serial.println((unsigned long long)overwrites);

  client.stop();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.end();
    WiFi.begin(ssid, pass);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 5000) {
      delay(250);
    }
  }

  WiFiClient client = server.available();
  if (!client) return;

  String reqLine = client.readStringUntil('\n');
  reqLine.trim();

  int contentLength = 0;
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) break;

    if (line.startsWith("Content-Length:")) {
      contentLength = line.substring(15).toInt();
    }
  }

  if (reqLine.startsWith("POST")) {
    handlePostTime(client, contentLength);
    return;
  }

  if (reqLine.startsWith("GET /data")) {
    handleStream(client);
    return;
  }

  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/plain; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.println("Not found");
  client.stop();
}