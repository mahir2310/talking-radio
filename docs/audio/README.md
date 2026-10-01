# Required audio files

This NodeMCU ESP8266 build uses PCM WAV files to reduce decoder memory use.
Keep the filenames below and place the files in these folders. Use standard
PCM WAV: mono, 22,050 Hz, signed 16-bit samples.

```text
audio/
|-- system/
|   |-- startup.wav
|   |-- chime.wav
|   |-- the-time-is.wav
|   |-- oclock.wav
|   |-- current-temperature-is.wav
|   |-- degrees-celsius.wav
|   |-- rain-alert.wav
|   `-- weather-unavailable.wav
|-- hours/
|   |-- 00.wav
|   |-- 01.wav
|   |-- ...
|   `-- 23.wav
|-- numbers/
|   |-- 00.wav
|   |-- 01.wav
|   |-- ...
|   `-- 50.wav
|-- weather/
|   |-- clear.wav
|   |-- cloudy.wav
|   |-- fog.wav
|   |-- rain.wav
|   `-- thunderstorm.wav
|-- motivation/
|   |-- 001.wav
|   `-- 002.wav
`-- surah/
    |-- 001.wav
    `-- 002.wav
```

Suggested speech:

| File | Suggested speech |
|---|---|
| `startup.wav` | Talking radio is ready. |
| `the-time-is.wav` | The time is |
| `oclock.wav` | o'clock |
| `current-temperature-is.wav` | The current temperature is |
| `degrees-celsius.wav` | degrees Celsius |
| `rain-alert.wav` | Rain is occurring or likely soon. Please prepare. |
| `weather-unavailable.wav` | Weather information is currently unavailable. |

Record `hours/00.wav` through `hours/23.wav` as spoken hour values, and
`numbers/00.wav` through `numbers/50.wav` as spoken number values. The weather
files should say `Clear`, `Cloudy`, `Fog`, `Rain`, and `Thunderstorm`.

To convert the generated MP3 recordings, install FFmpeg and run
`convert_mp3_to_wav.ps1` from the project folder. It creates WAV versions next
to the MP3s; the firmware and GitHub Pages player use the WAV versions.

WAV files are larger than MP3 files. Keep long recordings as short as practical,
and only upload audio that you have permission to redistribute.
