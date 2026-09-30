#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Edit these settings for your installation.
// ---------------------------------------------------------------------------

// After enabling GitHub Pages, replace YOUR_USERNAME and YOUR_REPOSITORY.
const char MANIFEST_URL[] =
    "https://mahir2310.github.io/talking-radio/config.json";

// Location used by Open-Meteo. Defaults to central Dhaka, Bangladesh.
constexpr float WEATHER_LATITUDE = 23.8103f;
constexpr float WEATHER_LONGITUDE = 90.4125f;

// Bangladesh is UTC+6 and has no daylight-saving adjustment.
constexpr long UTC_OFFSET_SECONDS = 6L * 60L * 60L;
constexpr int DAYLIGHT_OFFSET_SECONDS = 0;

// Do not make automatic hourly announcements outside these hours.
// With 7 and 22, announcements run from 07:00 through 21:00.
constexpr uint8_t QUIET_HOURS_END = 7;
constexpr uint8_t QUIET_HOURS_START = 22;

constexpr bool ANNOUNCE_WEATHER_AFTER_TIME = true;
constexpr uint8_t RAIN_PROBABILITY_THRESHOLD = 60;
constexpr uint16_t WEATHER_REFRESH_MINUTES = 15;
constexpr uint16_t RAIN_ALERT_REPEAT_MINUTES = 180;
// The device checks config.json again, so adding audio never needs a reflash.
constexpr uint16_t MANIFEST_REFRESH_MINUTES = 10;

// Start low. ESP8266Audio accepts a gain from 0.0 to 1.0.
constexpr float STARTING_VOLUME = 0.15f;
constexpr float VOLUME_STEP = 0.05f;
constexpr float MAXIMUM_VOLUME = 0.55f;

// ESP8266 hardware pins. I2S pins are fixed by the ESP8266Audio library.
constexpr uint8_t IR_RECEIVER_PIN = 14;  // NodeMCU D5 / GPIO14

// ---------------------------------------------------------------------------
// IR remote codes
// ---------------------------------------------------------------------------
// Upload once with these values left at zero. Open Serial Monitor at 115200,
// press each button, then paste the displayed hexadecimal code below.
// A value of zero disables that command.

constexpr uint64_t IR_TIME = 0x0;
constexpr uint64_t IR_WEATHER = 0x0;
constexpr uint64_t IR_MOTIVATION = 0x0;
constexpr uint64_t IR_SURAH = 0x0;
constexpr uint64_t IR_NEXT = 0x0;
constexpr uint64_t IR_STOP = 0x0;
constexpr uint64_t IR_VOLUME_UP = 0x0;
constexpr uint64_t IR_VOLUME_DOWN = 0x0;
constexpr uint64_t IR_TOGGLE_AUTOMATIC = 0x0;
constexpr uint64_t IR_RELOAD_LIBRARY = 0x0;

