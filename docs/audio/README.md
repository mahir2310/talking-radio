# Required audio files

Place your MP3 recordings in this directory using the exact layout below.
Record them in Bangla, English, or both, but keep every filename unchanged.

```text
audio/
|-- system/
|   |-- startup.mp3
|   |-- chime.mp3
|   |-- the-time-is.mp3
|   |-- oclock.mp3
|   |-- current-temperature-is.mp3
|   |-- degrees-celsius.mp3
|   |-- rain-alert.mp3
|   `-- weather-unavailable.mp3
|-- hours/
|   |-- 00.mp3
|   |-- 01.mp3
|   |-- ...
|   `-- 23.mp3
|-- numbers/
|   |-- 00.mp3
|   |-- 01.mp3
|   |-- ...
|   `-- 50.mp3
|-- weather/
|   |-- clear.mp3
|   |-- cloudy.mp3
|   |-- fog.mp3
|   |-- rain.mp3
|   `-- thunderstorm.mp3
|-- motivation/
|   |-- 001.mp3
|   `-- 002.mp3
`-- surah/
    |-- 001.mp3
    `-- 002.mp3
```

Suggested recording text:

| File | Suggested speech |
|---|---|
| `startup.mp3` | "Talking radio is ready." |
| `the-time-is.mp3` | "The time is" |
| `oclock.mp3` | "o'clock" |
| `current-temperature-is.mp3` | "The current temperature is" |
| `degrees-celsius.mp3` | "degrees Celsius" |
| `rain-alert.mp3` | "Rain is occurring or likely soon. Please prepare." |
| `weather-unavailable.mp3` | "Weather information is currently unavailable." |

For 24-hour announcements, record `hours/00.mp3` through `hours/23.mp3` as
spoken hour values. Record `numbers/00.mp3` through `numbers/50.mp3` for
temperature readings.

Recommended encoding:

```powershell
ffmpeg -i input.wav -ac 1 -ar 22050 -b:a 64k output.mp3
```

Do not upload audio that you do not have permission to redistribute.

