const statusElement = document.querySelector("#status");
const libraryElement = document.querySelector("#library");
const player = document.querySelector("#audio-player");
const nowPlaying = document.querySelector("#now-playing");

function absoluteTrackUrl(baseUrl, track) {
  return new URL(track, baseUrl).href;
}

function titleFromPath(path) {
  return path
    .split("/")
    .pop()
    .replace(/\.wav$/i, "")
    .replaceAll("-", " ")
    .replace(/^\d+$/, (value) => `Track ${value}`);
}

function playTrack(url, path, button) {
  document
    .querySelectorAll(".track-button[aria-current='true']")
    .forEach((item) => item.removeAttribute("aria-current"));

  button.setAttribute("aria-current", "true");
  nowPlaying.textContent = path;
  player.src = url;
  player.play().catch(() => {
    statusElement.textContent =
      "Playback did not start. Check that the WAV file exists, then press the audio play button.";
  });
}

function createGroup(title, tracks, baseUrl) {
  const section = document.createElement("section");
  section.className = "group";

  const heading = document.createElement("h2");
  heading.textContent = title;
  section.append(heading);

  const list = document.createElement("ul");
  list.className = "track-list";

  tracks.forEach((track) => {
    const item = document.createElement("li");
    const button = document.createElement("button");
    button.className = "track-button";
    button.type = "button";
    button.textContent = titleFromPath(track);
    button.addEventListener("click", () => {
      playTrack(absoluteTrackUrl(baseUrl, track), track, button);
    });
    item.append(button);
    list.append(item);
  });

  section.append(list);
  libraryElement.append(section);
}

async function loadLibrary() {
  try {
    const response = await fetch(`config.json?v=${Date.now()}`, {
      cache: "no-store",
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);

    const config = await response.json();
    if (config.base_url.includes("YOUR_USERNAME")) {
      throw new Error("Replace YOUR_USERNAME and YOUR_REPOSITORY in config.json");
    }

    const systemTracks = Object.values(config.system);
    const hourTracks = Array.from(
      { length: 24 },
      (_, hour) => `hours/${String(hour).padStart(2, "0")}.wav`,
    );
    const numberTracks = Array.from(
      { length: 51 },
      (_, number) => `numbers/${String(number).padStart(2, "0")}.wav`,
    );
    const weatherTracks = [
      "weather/clear.wav",
      "weather/cloudy.wav",
      "weather/fog.wav",
      "weather/rain.wav",
      "weather/thunderstorm.wav",
    ];

    createGroup("System announcements", systemTracks, config.base_url);
    createGroup("Hours", hourTracks, config.base_url);
    createGroup("Temperature numbers", numberTracks, config.base_url);
    createGroup("Weather conditions", weatherTracks, config.base_url);
    createGroup("Motivation", config.motivation ?? [], config.base_url);
    createGroup("Surah", config.surah ?? [], config.base_url);

    statusElement.textContent = `${
      systemTracks.length +
      hourTracks.length +
      numberTracks.length +
      weatherTracks.length +
      (config.motivation?.length ?? 0) +
      (config.surah?.length ?? 0)
    } configured tracks`;
  } catch (error) {
    statusElement.textContent = `Configuration error: ${error.message}`;
  }
}

player.addEventListener("error", () => {
  statusElement.textContent =
    "This audio file could not be loaded. Confirm its name and location in the GitHub repository.";
});

loadLibrary();

