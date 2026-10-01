/*
  ESP8266 Talking Radio

  Board: NodeMCU 1.0 (ESP-12E Module)
  DAC: PCM5102
  Amplifier: PAM8403, one speaker connected to L+ and L- only

  Required Arduino libraries:
    - ESP8266Audio by Earle F. Philhower
    - IRremoteESP8266 by David Conran
    - ArduinoJson by Benoit Blanchon

  PCM5102 wiring for the photographed purple GY-PCM5102 board:
    GPIO2 / D4  -> LCK / LRCK
    GPIO15 / D8 -> BCK / BCLK
    GPIO3 / RX  -> DIN
    VU / USB 5V -> VIN
    3V3         -> XMT/XSMT
    GND         -> GND, FLT, DEMP, FMT and SCK
    A3V3        -> leave unconnected

  IR receiver:
    GPIO14 / D5 -> OUT
    3V3         -> VCC
    GND         -> GND
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <time.h>
#include <AudioLogger.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>

#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>

#include "AudioFileSourceHTTPSStream.h"
#include "config.h"
#include "secrets.h"

namespace {

constexpr size_t URL_QUEUE_SIZE = 20;
constexpr size_t MAX_TRACKS_PER_CATEGORY = 24;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 15000;

IRrecv irReceiver(IR_RECEIVER_PIN);
decode_results irResult;

AudioGeneratorWAV *wav = nullptr;
AudioFileSourceHTTPSStream *audioSource = nullptr;
AudioOutputI2S *audioOutput = nullptr;

String playbackQueue[URL_QUEUE_SIZE];
size_t queueHead = 0;
size_t queueCount = 0;

String baseAudioUrl;
String manifestVersion = "1";
String startupTrack;
String chimeTrack;
String timePrefixTrack;
String oclockTrack;
String weatherPrefixTrack;
String degreesTrack;
String rainAlertTrack;
String weatherUnavailableTrack;
String motivationTracks[MAX_TRACKS_PER_CATEGORY];
String surahTracks[MAX_TRACKS_PER_CATEGORY];
size_t motivationCount = 0;
size_t surahCount = 0;
size_t nextMotivation = 0;
size_t nextSurah = 0;

float volume = STARTING_VOLUME;
float currentTemperatureC = NAN;
float currentRainMm = 0.0f;
int currentWeatherCode = -1;
int maximumUpcomingRainProbability = 0;

bool manifestLoaded = false;
bool weatherAvailable = false;
bool automaticAnnouncementsEnabled = true;
bool rainWasActive = false;

int lastAnnouncedHour = -1;
uint32_t lastWiFiAttemptMs = 0;
uint32_t lastManifestAttemptMs = 0;
uint32_t lastWeatherUpdateMs = 0;
uint32_t lastRainAlertMs = 0;

bool elapsed(uint32_t now, uint32_t previous, uint32_t interval) {
  return previous == 0 || static_cast<uint32_t>(now - previous) >= interval;
}

String joinUrl(const String &path) {
  if (path.startsWith("http://") || path.startsWith("https://")) {
    return path;
  }
  String url = baseAudioUrl + path;
  // Incrementing config.json's version immediately bypasses an old CDN copy.
  url += url.indexOf('?') >= 0 ? '&' : '?';
  url += F("v=");
  url += manifestVersion;
  return url;
}

void clearQueue() {
  queueHead = 0;
  queueCount = 0;
}

bool enqueue(const String &urlOrPath) {
  if (urlOrPath.length() == 0 || queueCount >= URL_QUEUE_SIZE) {
    return false;
  }
  size_t tail = (queueHead + queueCount) % URL_QUEUE_SIZE;
  playbackQueue[tail] = joinUrl(urlOrPath);
  queueCount++;
  return true;
}

String dequeue() {
  if (queueCount == 0) return String();
  String value = playbackQueue[queueHead];
  playbackQueue[queueHead] = String();
  queueHead = (queueHead + 1) % URL_QUEUE_SIZE;
  queueCount--;
  return value;
}

bool audioIsRunning() {
  return wav != nullptr && wav->isRunning();
}

void stopCurrentAudio() {
  if (wav != nullptr) {
    if (wav->isRunning()) wav->stop();
    delete wav;
    wav = nullptr;
  }
  delete audioSource;
  audioSource = nullptr;
}

bool startAudio(const String &url) {
  stopCurrentAudio();
  if (WiFi.status() != WL_CONNECTED || url.length() == 0) return false;

  Serial.print(F("Playing: "));
  Serial.println(url);

  audioSource = new AudioFileSourceHTTPSStream(url.c_str());
  wav = new AudioGeneratorWAV();

  Serial.printf(
      "Audio source open: %d, free heap: %u, largest free block: %u, fragmentation: %u%%\n",
      audioSource->isOpen(), ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(),
      ESP.getHeapFragmentation());
  if (!wav->begin(audioSource, audioOutput)) {
    Serial.println(F("Could not start WAV stream."));
    stopCurrentAudio();
    return false;
  }
  return true;
}

void serviceAudio() {
  if (audioIsRunning()) {
    if (!wav->loop()) stopCurrentAudio();
    return;
  }

  if (wav != nullptr) stopCurrentAudio();

  if (queueCount > 0 && WiFi.status() == WL_CONNECTED) {
    String nextUrl = dequeue();
    startAudio(nextUrl);
  }
}

void interruptWith(const String &track) {
  stopCurrentAudio();
  clearQueue();
  enqueue(track);
}

void setVolume(float newVolume) {
  volume = constrain(newVolume, 0.0f, MAXIMUM_VOLUME);
  audioOutput->SetGain(volume);
  Serial.printf("Volume: %d%%\n", static_cast<int>(volume * 100.0f));
}

void connectWiFi() {
  if (strcmp(WIFI_SSID, "YOUR_WIFI_NAME") == 0) {
    Serial.println(F("Edit firmware/secrets.h with your Wi-Fi details."));
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print(F("Connecting to Wi-Fi"));

  for (uint8_t attempt = 0; attempt < 40 && WiFi.status() != WL_CONNECTED;
       attempt++) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("Wi-Fi connected. IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("Wi-Fi connection timed out; background retry enabled."));
  }
}

void serviceWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  uint32_t now = millis();
  if (!elapsed(now, lastWiFiAttemptMs, WIFI_RETRY_INTERVAL_MS)) return;
  lastWiFiAttemptMs = now;
  Serial.println(F("Retrying Wi-Fi..."));
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

bool secureGet(const String &url, String &payload) {
  if (WiFi.status() != WL_CONNECTED) return false;

  // These URLs contain public audio/weather data and no private information.
  // setInsecure avoids storing a large CA bundle on the ESP8266.
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(12000);

  HTTPClient http;
  http.setTimeout(12000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, url)) return false;

  int status = http.GET();
  if (status != HTTP_CODE_OK) {
    Serial.printf("HTTP %d for %s\n", status, url.c_str());
    http.end();
    return false;
  }

  payload = http.getString();
  http.end();
  return true;
}

void copyTrackArray(JsonVariantConst source, String *destination,
                    size_t &destinationCount) {
  destinationCount = 0;
  if (!source.is<JsonArrayConst>()) return;
  for (JsonVariantConst value : source.as<JsonArrayConst>()) {
    if (destinationCount >= MAX_TRACKS_PER_CATEGORY) break;
    const char *track = value.as<const char *>();
    if (track != nullptr && track[0] != '\0') {
      destination[destinationCount++] = track;
    }
  }
}

bool loadManifest() {
  lastManifestAttemptMs = millis();
  String payload;
  String requestUrl = MANIFEST_URL;
  requestUrl += requestUrl.indexOf('?') >= 0 ? '&' : '?';
  requestUrl += F("device_request=");
  requestUrl += String(lastManifestAttemptMs);
  if (!secureGet(requestUrl, payload)) return false;

  DynamicJsonDocument document(12288);
  DeserializationError error = deserializeJson(document, payload);
  if (error) {
    Serial.print(F("Manifest JSON error: "));
    Serial.println(error.c_str());
    return false;
  }

  baseAudioUrl = document["base_url"].as<String>();
  if (!baseAudioUrl.endsWith("/")) baseAudioUrl += '/';
  manifestVersion = document["version"].as<String>();
  if (manifestVersion.length() == 0) manifestVersion = "1";

  startupTrack = document["system"]["startup"].as<String>();
  chimeTrack = document["system"]["chime"].as<String>();
  timePrefixTrack = document["system"]["time_prefix"].as<String>();
  oclockTrack = document["system"]["oclock"].as<String>();
  weatherPrefixTrack = document["system"]["weather_prefix"].as<String>();
  degreesTrack = document["system"]["degrees_celsius"].as<String>();
  rainAlertTrack = document["system"]["rain_alert"].as<String>();
  weatherUnavailableTrack =
      document["system"]["weather_unavailable"].as<String>();

  copyTrackArray(document["motivation"], motivationTracks, motivationCount);
  copyTrackArray(document["surah"], surahTracks, surahCount);

  manifestLoaded = baseAudioUrl.startsWith("http") && baseAudioUrl.indexOf(
      "YOUR_USERNAME") < 0;

  if (!manifestLoaded) {
    Serial.println(F("Edit docs/config.json and firmware/config.h URLs first."));
    return false;
  }

  Serial.printf("Manifest loaded: %u motivation, %u surah tracks.\n",
                static_cast<unsigned>(motivationCount),
                static_cast<unsigned>(surahCount));
  return true;
}

String twoDigitPath(const char *folder, int number) {
  char path[40];
  snprintf(path, sizeof(path), "%s/%02d.wav", folder, number);
  return String(path);
}

String weatherConditionTrack(int code) {
  if (code == 0 || code == 1) return F("weather/clear.wav");
  if (code == 2 || code == 3) return F("weather/cloudy.wav");
  if (code == 45 || code == 48) return F("weather/fog.wav");
  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) {
    return F("weather/rain.wav");
  }
  if (code >= 95) return F("weather/thunderstorm.wav");
  return String();
}

bool refreshWeather() {
  if (WiFi.status() != WL_CONNECTED || audioIsRunning()) return false;

  String url = F("https://api.open-meteo.com/v1/forecast?latitude=");
  url += String(WEATHER_LATITUDE, 4);
  url += F("&longitude=");
  url += String(WEATHER_LONGITUDE, 4);
  url += F("&current=temperature_2m,rain,weather_code");
  url += F("&hourly=precipitation_probability&forecast_hours=3&timezone=auto");

  String payload;
  lastWeatherUpdateMs = millis();
  if (!secureGet(url, payload)) {
    weatherAvailable = false;
    return false;
  }

  DynamicJsonDocument document(6144);
  DeserializationError error = deserializeJson(document, payload);
  if (error) {
    Serial.print(F("Weather JSON error: "));
    Serial.println(error.c_str());
    weatherAvailable = false;
    return false;
  }

  currentTemperatureC = document["current"]["temperature_2m"] | NAN;
  currentRainMm = document["current"]["rain"] | 0.0f;
  currentWeatherCode = document["current"]["weather_code"] | -1;
  maximumUpcomingRainProbability = 0;

  JsonArrayConst probabilities =
      document["hourly"]["precipitation_probability"].as<JsonArrayConst>();
  for (int value : probabilities) {
    if (value > maximumUpcomingRainProbability) {
      maximumUpcomingRainProbability = value;
    }
  }

  weatherAvailable = !isnan(currentTemperatureC);
  Serial.printf("Weather: %.1f C, rain %.1f mm, code %d, probability %d%%\n",
                currentTemperatureC, currentRainMm, currentWeatherCode,
                maximumUpcomingRainProbability);
  return weatherAvailable;
}

void enqueueWeatherAnnouncement() {
  if (!manifestLoaded) return;
  if (!weatherAvailable) {
    enqueue(weatherUnavailableTrack);
    return;
  }

  enqueue(weatherPrefixTrack);
  int roundedTemperature = static_cast<int>(roundf(currentTemperatureC));
  if (roundedTemperature >= 0 && roundedTemperature <= 50) {
    enqueue(twoDigitPath("numbers", roundedTemperature));
    enqueue(degreesTrack);
  }
  enqueue(weatherConditionTrack(currentWeatherCode));
}

void enqueueTimeAnnouncement(const tm &timeInfo) {
  if (!manifestLoaded) return;
  enqueue(chimeTrack);
  enqueue(timePrefixTrack);
  enqueue(twoDigitPath("hours", timeInfo.tm_hour));
  enqueue(oclockTrack);
  if (ANNOUNCE_WEATHER_AFTER_TIME) enqueueWeatherAnnouncement();
}

bool isQuietHour(int hour) {
  if (QUIET_HOURS_START > QUIET_HOURS_END) {
    return hour >= QUIET_HOURS_START || hour < QUIET_HOURS_END;
  }
  return hour >= QUIET_HOURS_START && hour < QUIET_HOURS_END;
}

bool readLocalTime(tm &timeInfo, uint32_t timeoutMs) {
  uint32_t started = millis();
  do {
    time_t now = time(nullptr);
    if (now > 1600000000) {
      localtime_r(&now, &timeInfo);
      return true;
    }
    yield();
  } while (static_cast<uint32_t>(millis() - started) < timeoutMs);
  return false;
}

void serviceHourlyAnnouncement() {
  if (!automaticAnnouncementsEnabled || !manifestLoaded) return;

  tm timeInfo;
  if (!readLocalTime(timeInfo, 10)) return;

  // Permit a short two-minute window in case a network operation overlaps 00:00.
  if (timeInfo.tm_min <= 1 && timeInfo.tm_hour != lastAnnouncedHour &&
      !isQuietHour(timeInfo.tm_hour)) {
    lastAnnouncedHour = timeInfo.tm_hour;
    enqueueTimeAnnouncement(timeInfo);
  }
}

void serviceWeather() {
  uint32_t now = millis();
  uint32_t interval = WEATHER_REFRESH_MINUTES * 60UL * 1000UL;
  if (!elapsed(now, lastWeatherUpdateMs, interval) || audioIsRunning()) return;

  if (!refreshWeather()) return;

  bool rainActive = currentRainMm > 0.0f ||
                    maximumUpcomingRainProbability >= RAIN_PROBABILITY_THRESHOLD;
  uint32_t repeatInterval = RAIN_ALERT_REPEAT_MINUTES * 60UL * 1000UL;
  bool repeatDue = elapsed(now, lastRainAlertMs, repeatInterval);

  if (rainActive && (!rainWasActive || repeatDue)) {
    lastRainAlertMs = now;
    interruptWith(rainAlertTrack);
  }
  rainWasActive = rainActive;
}

void serviceManifest() {
  if (WiFi.status() != WL_CONNECTED || audioIsRunning()) return;
  uint32_t refreshInterval = MANIFEST_REFRESH_MINUTES * 60UL * 1000UL;
  if (elapsed(millis(), lastManifestAttemptMs, refreshInterval)) {
    loadManifest();
  }
}

void playNextMotivation() {
  if (motivationCount == 0) return;
  interruptWith(motivationTracks[nextMotivation]);
  nextMotivation = (nextMotivation + 1) % motivationCount;
}

void playNextSurah() {
  if (surahCount == 0) return;
  interruptWith(surahTracks[nextSurah]);
  nextSurah = (nextSurah + 1) % surahCount;
}

void announceTimeNow() {
  tm timeInfo;
  if (!readLocalTime(timeInfo, 100)) return;
  stopCurrentAudio();
  clearQueue();
  enqueueTimeAnnouncement(timeInfo);
}

void announceWeatherNow() {
  stopCurrentAudio();
  clearQueue();
  if (elapsed(millis(), lastWeatherUpdateMs, 5UL * 60UL * 1000UL)) {
    refreshWeather();
  }
  enqueueWeatherAnnouncement();
}

void handleIrCode(uint64_t code) {
  if (code == 0) return;

  if (IR_TIME != 0 && code == IR_TIME) {
    announceTimeNow();
  } else if (IR_WEATHER != 0 && code == IR_WEATHER) {
    announceWeatherNow();
  } else if (IR_MOTIVATION != 0 && code == IR_MOTIVATION) {
    playNextMotivation();
  } else if ((IR_SURAH != 0 && code == IR_SURAH) ||
             (IR_NEXT != 0 && code == IR_NEXT)) {
    playNextSurah();
  } else if (IR_STOP != 0 && code == IR_STOP) {
    stopCurrentAudio();
    clearQueue();
  } else if (IR_VOLUME_UP != 0 && code == IR_VOLUME_UP) {
    setVolume(volume + VOLUME_STEP);
  } else if (IR_VOLUME_DOWN != 0 && code == IR_VOLUME_DOWN) {
    setVolume(volume - VOLUME_STEP);
  } else if (IR_TOGGLE_AUTOMATIC != 0 && code == IR_TOGGLE_AUTOMATIC) {
    automaticAnnouncementsEnabled = !automaticAnnouncementsEnabled;
    Serial.printf("Automatic announcements: %s\n",
                  automaticAnnouncementsEnabled ? "ON" : "OFF");
  } else if (IR_RELOAD_LIBRARY != 0 && code == IR_RELOAD_LIBRARY) {
    stopCurrentAudio();
    clearQueue();
    if (loadManifest()) Serial.println(F("Audio library reloaded."));
  }
}

void serviceIrReceiver() {
  if (!irReceiver.decode(&irResult)) return;

  if (!irResult.repeat) {
    String codeText = uint64ToString(irResult.value, 16);
    codeText.toUpperCase();
    Serial.print(F("IR code: 0x"));
    Serial.println(codeText);
    handleIrCode(irResult.value);
  }
  irReceiver.resume();
}

}  // namespace

void setup() {
  // Transmit-only prevents incoming USB serial traffic from fighting GPIO3,
  // which becomes the PCM5102 I2S data output after audio starts.
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  audioLogger = &Serial;
  delay(200);
  Serial.println();
  Serial.println(F("ESP8266 Talking Radio starting..."));

  irReceiver.enableIRIn();

  audioOutput = new AudioOutputI2S();
  audioOutput->SetGain(volume);

  connectWiFi();
  configTime(UTC_OFFSET_SECONDS, DAYLIGHT_OFFSET_SECONDS, "pool.ntp.org",
             "time.google.com", "time.cloudflare.com");

  if (WiFi.status() == WL_CONNECTED) {
    manifestLoaded = loadManifest();
    // Fetch weather from loop() after startup audio has had a chance to begin.
    if (manifestLoaded) enqueue(startupTrack);
  }
}

void loop() {
  serviceAudio();
  serviceIrReceiver();
  serviceWiFi();

  serviceManifest();
  serviceWeather();
  serviceHourlyAnnouncement();
  yield();
}

