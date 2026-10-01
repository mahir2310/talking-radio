# ESP8266 Talking Radio

This project turns a NodeMCU ESP8266, PCM5102 DAC, PAM8403 amplifier, one
speaker, and an IR receiver into an hourly talking announcer.

Features:

- Hourly time announcement with quiet hours.
- Current temperature and weather condition.
- Rain warning based on current rain or forecast probability.
- IR buttons for time, weather, motivation, surah, volume, and stop.
- PCM WAV streaming from a GitHub Pages audio library.
- Browser page for testing every hosted audio file.
- Zero-cost browser manager for changing the library without reflashing.

## Zero-cost design

The project uses only free services and local software:

- GitHub repository for WAV storage.
- GitHub Pages for HTTPS audio delivery.
- Open-Meteo for weather data without an API key.
- NTP for time synchronization.
- Arduino IDE and free open-source libraries.

No credit card, subscription, paid text-to-speech service, database, or private
web server is required. You provide the recordings; the ESP8266 streams them.

## 1. Safe hardware connection

Use a regulated, isolated 5 V, 2-3 A supply. Do not connect this project to the
old radio's mains wiring. Reuse only the enclosure and speaker unless you are
qualified to inspect and isolate the original mains circuitry.

### Power

| Connection | Destination |
|---|---|
| Supply +5 V | NodeMCU `VIN/5V` and PAM8403 `5V/VCC` |
| Supply GND | NodeMCU, PCM5102 and PAM8403 GND |
| 1000 uF capacitor + | PAM8403 5 V terminal |
| 1000 uF capacitor - | PAM8403 GND terminal |

Add a 100 nF ceramic capacitor beside the IR receiver between 3.3 V and GND.

For a connection-only, photograph-specific checklist, use
`CONNECTION_WIRING.txt`.

### PCM5102 to NodeMCU

| PCM5102 | NodeMCU |
|---|---|
| `LCK`, `LRCK` or `WS` | D4 / GPIO2 |
| `BCK` or `BCLK` | D8 / GPIO15 |
| `DIN` | RX / GPIO3 |
| `VIN` | VU/USB 5 V on the photographed purple GY-PCM5102 board |
| `XMT/XSMT` | NodeMCU 3V3 |
| `GND`, `FLT`, `DEMP`, `FMT`, `SCK` | NodeMCU GND |
| `A3V3/3V3` | Leave unconnected when VIN is powered |

Breakout boards vary. Check the labels printed on both sides of your board
before applying power.

### PCM5102 to PAM8403

| PCM5102 | PAM8403 |
|---|---|
| `LOUT` | `L` or `LIN` |
| `ROUT` | `R` or `RIN` (optional with one speaker) |
| Audio GND | Input GND |

### One speaker

Connect the speaker's two wires only to PAM8403 `L+` and `L-`. Leave `R+` and
`R-` disconnected. Never connect either speaker terminal to ground and never
join the negative amplifier outputs.

If the speaker is labelled 4 ohms but measures about 2.7 ohms with a multimeter,
it is normally safe because the measurement is DC resistance. If it is actually
labelled 2.7 or 3 ohms, add a 1-1.5 ohm, 2 W series resistor or use a 4-ohm
speaker.

### IR receiver

| IR receiver | NodeMCU |
|---|---|
| OUT | D5 / GPIO14 |
| VCC | 3V3 |
| GND | GND |

The pin order differs between receiver models. Look up the exact receiver model
instead of assuming the left-to-right order.

## 2. Prepare the audio library

1. Record or obtain permitted WAV announcements, or convert the generated MP3 files to WAV.
2. Follow the exact file list in `docs/audio/README.md`.
3. Use standard PCM WAV: mono, 22.05 kHz, 16-bit samples.
4. Put every file below `docs/audio/`.
5. Add or remove motivation and surah filenames in `docs/config.json`.

WAV files are larger than MP3 files, especially for long surah recordings. Test
long recordings carefully and keep an eye on GitHub Pages storage and transfer
limits. Wi-Fi interruption during a stream stops that track; the next request
starts it from the beginning.

## 3. Configure GitHub Pages

1. Create a GitHub repository, for example `talking-radio`.
2. Upload this entire project to its `main` branch.
3. In `docs/config.json`, replace the repository and URL placeholders. Example:

   ```json
   "repository": { "owner": "rahim", "name": "talking-radio" },
   "base_url": "https://rahim.github.io/talking-radio/audio/"
   ```

4. In `esp8266_talking_radio/config.h`, make the same replacement:

   ```cpp
   const char MANIFEST_URL[] =
       "https://rahim.github.io/talking-radio/config.json";
   ```

5. Commit and push those changes.
6. Open GitHub repository **Settings > Pages**.
7. Choose **Deploy from a branch**.
8. Select branch `main`, directory `/docs`, and save.
9. Wait a few minutes, then open:

   ```text
   https://YOUR_USERNAME.github.io/YOUR_REPOSITORY/
   ```

10. Use the web page to play and verify each configured track.

## Change audio later without uploading firmware

After the first setup, open the **Manage audio library** link on your GitHub
Pages site. It provides direct buttons for the correct GitHub upload folders and
prepares an updated `config.json` for you.

The safe, free update process is:

1. Open `https://YOUR_USERNAME.github.io/YOUR_REPOSITORY/manage.html`.
2. Press **Upload motivation WAV** or **Upload surah WAV**.
3. Sign in to GitHub if requested, choose the new WAV, and commit it.
4. Return to the manager and add the path, for example
   `motivation/morning-003.wav`, on a new line.
5. Press **Prepare update**. This automatically increases the library version
   so GitHub's old cached audio is not reused.
6. Press **Copy JSON**, then **Open config.json on GitHub**.
7. Replace the GitHub editor content with the copied JSON and commit.
8. Press the remote button assigned to `IR_RELOAD_LIBRARY`, or simply wait up
   to ten minutes.

The ESP8266 reloads `config.json` every ten minutes. Therefore WAV additions,
removals, reordered playlists, and replacement recordings require no USB cable
and no firmware reflash.

The manager deliberately does not request a GitHub token. Putting a repository
token into a public static page would allow other people to copy it. GitHub's
normal signed-in upload and edit screens provide the secure zero-cost option.

If you change a WAV but reuse its filename, GitHub's CDN may temporarily cache
the old copy. Rename the file and update `config.json`, or wait for the cache to
expire.

## 4. Configure Arduino IDE

1. Open **File > Preferences**.
2. Add this Boards Manager URL:

   ```text
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```

3. Open **Tools > Board > Boards Manager**, search for `esp8266`, and install
   **esp8266 by ESP8266 Community**.
4. Open **Sketch > Include Library > Manage Libraries** and install:
   - `ESP8266Audio`
   - `IRremoteESP8266`
   - `ArduinoJson`
5. Select **Tools > Board > ESP8266 Boards > NodeMCU 1.0 (ESP-12E Module)**.
6. Select **CPU Frequency > 160 MHz**.
7. Select **lwIP Variant > v2 Higher Bandwidth** if that menu is available.
8. Select the correct COM port.

## 5. Configure and upload the firmware

Open `esp8266_talking_radio/esp8266_talking_radio.ino` in Arduino IDE. Its
filename already matches its containing folder, as required by Arduino. Then:

1. Edit `esp8266_talking_radio/secrets.h` with your 2.4 GHz Wi-Fi name and password.
2. Edit `esp8266_talking_radio/config.h` with your GitHub Pages URL.
3. Change latitude, longitude, UTC offset, and quiet hours if necessary.
4. Connect the NodeMCU by USB.
5. Click **Verify**.
6. Fix any library installation error before connecting the amplifier.
7. Click **Upload**.
8. Open **Tools > Serial Monitor** at 115200 baud.

Expected output includes:

```text
ESP8266 Talking Radio starting...
Wi-Fi connected. IP: ...
Manifest loaded: ...
Weather: ...
Playing: .../audio/system/startup.wav
```

## 6. Learn and configure the IR remote

The initial IR command values in `esp8266_talking_radio/config.h` are zero, so commands are
disabled while the receiver learns button values.

1. Keep Serial Monitor open at 115200 baud.
2. Press one remote button briefly.
3. Copy the printed value, for example `IR code: 0xFF30CF`.
4. Repeat for all desired buttons.
5. Paste the values into `config.h`:

   ```cpp
   constexpr uint64_t IR_TIME = 0xFF30CF;
   constexpr uint64_t IR_WEATHER = 0xFF18E7;
   constexpr uint64_t IR_MOTIVATION = 0xFF7A85;
   constexpr uint64_t IR_RELOAD_LIBRARY = 0xFF10EF;
   ```

6. Verify and upload again.

If no code appears, confirm the receiver pin order and that OUT is connected to
D5. A phone camera can usually show the remote's IR LED flashing while a button
is pressed.

## 7. First powered audio test

1. Disconnect power.
2. Turn the PAM8403 volume control fully down if it has one.
3. Connect only one speaker to `L+` and `L-`.
4. Inspect every power and ground wire again.
5. Apply 5 V power.
6. Listen for the startup message.
7. Raise the amplifier volume slowly.

Do not begin at full volume. The firmware starts at 15% digital gain and limits
IR volume adjustment to 55%.

## 8. How automatic operation works

- NTP obtains local time after Wi-Fi connects.
- At minute 00 or 01, the device queues the chime, hour, and weather tracks.
- Weather refreshes every 15 minutes.
- Rain is announced when current rain is above zero or the highest next-three-
  hour probability reaches the configured threshold.
- The same continuing rain condition is repeated no more than once every three
  hours.
- IR requests interrupt the current track and start the requested category.
- The system does not announce automatically during quiet hours.

## 9. Troubleshooting

### No sound

- Test the WAV from the GitHub Pages browser page.
- Confirm PCM5102 LOUT goes to PAM8403 left input, not its speaker output.
- Confirm the speaker is on `L+` and `L-`, not GND.
- Verify PCM5102 `XMT/XSMT` is high.
- Check Serial Monitor for HTTP errors.

### Buzzing or Wi-Fi resets

- Use a 5 V supply rated for at least 2 A.
- Keep the 1000 uF capacitor close to the PAM8403.
- Keep DAC-to-amplifier analog wires short.
- Twist each pair of speaker wires.
- Use one common low-voltage ground point near the power input.

### Upload fails

- Disconnect external power and use USB only during upload.
- Temporarily disconnect PCM5102 DIN from NodeMCU RX/GPIO3 if necessary.
- Confirm NodeMCU 1.0 and the correct COM port are selected.

### Audio stops during a long surah

- Improve Wi-Fi signal.
- Re-encode at 64 kbit/s mono.
- Avoid very large bit rates and variable-rate files.
- For highly reliable long playback, add an SD-card module or move to ESP32.

