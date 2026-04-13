#include <RPC.h>
#include <string>

const int chunkSize = 400;
const int bufferSize = 3000; //once buffer size is exceeded, new data will overwrite values at the bottom of the buffer
const uint32_t dtUs = 2000;   // 500 Hz

volatile float accelArr[bufferSize];
volatile uint64_t timeArr[bufferSize];
volatile uint64_t idArr[bufferSize];

volatile int head = 0;
volatile int tail = 0;
volatile int count = 0;

volatile uint64_t producedCount = 0;
volatile uint64_t overwriteCount = 0;

uint64_t baseEpochMs = 0;
uint64_t sampleIndex = 0;

int availableSamples() {
  return count;
}

uint64_t getOverwriteCount() {
  return overwriteCount;
}

std::string getNextSampleLine() {
  if (count <= 0) {
    return std::string("");
  }

  uint64_t id = idArr[tail];
  uint64_t t = timeArr[tail];
  float accel = accelArr[tail];

  tail = (tail + 1) % bufferSize;
  count--;

  std::string s = "id=";
  s += std::to_string(id);
  s += ", accel=";
  s += std::to_string(accel);
  s += ", time=";
  s += std::to_string(t);

  return s;
}

void setup() {
  RPC.begin();

  RPC.bind("availableSamples", availableSamples);
  RPC.bind("getNextSampleLine", getNextSampleLine);
  RPC.bind("getOverwriteCount", getOverwriteCount);

  while (true) {
    auto r = RPC.call("sendTime"); //get time from m7 core
    uint64_t t = r.get().as<uint64_t>();

    if (t != 0) {
      baseEpochMs = t;
      sampleIndex = 0;
      break;
    }

    delay(50);
  }
}

void loop() {
  static uint32_t nextUs = micros();

  for (int i = 0; i < chunkSize; i++) {
    while ((int32_t)(micros() - nextUs) < 0) { }
    nextUs += dtUs;

    int reading = analogRead(A6); //recording analog accelerometer measurements

    uint64_t t = baseEpochMs + (sampleIndex * dtUs) / 1000ULL;
    uint64_t thisId = producedCount;

    sampleIndex++;
    producedCount++;

    if (count == bufferSize) {
      tail = (tail + 1) % bufferSize;
      count--;
      overwriteCount++;
    }

    accelArr[head] = (float)reading;
    timeArr[head] = t;
    idArr[head] = thisId;

    head = (head + 1) % bufferSize;
    count++;
  }
}