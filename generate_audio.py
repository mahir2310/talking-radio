"""Generate the English talking-radio MP3 library with Google Text-to-Speech.

Run from the project folder after installing gTTS:
    python -m pip install gTTS
    python generate_audio.py

An internet connection is required. Files are written below docs/audio/.
"""

from pathlib import Path
from gtts import gTTS


AUDIO_ROOT = Path(__file__).resolve().parent / "docs" / "audio"

audio_data = {
    "system": {
        "startup.mp3": "Talking radio is ready.",
        # Spoken placeholder; replace with a musical chime if preferred.
        "chime.mp3": "Ding.",
        "the-time-is.mp3": "The time is",
        "oclock.mp3": "o'clock",
        "current-temperature-is.mp3": "The current temperature is",
        "degrees-celsius.mp3": "degrees Celsius",
        "rain-alert.mp3": "Rain is occurring or likely soon. Please prepare.",
        "weather-unavailable.mp3": "Weather information is currently unavailable.",
    },
    "hours": {
        f"{number:02}.mp3": word
        for number, word in enumerate(
            [
                "zero", "one", "two", "three", "four", "five", "six", "seven",
                "eight", "nine", "ten", "eleven", "twelve", "thirteen",
                "fourteen", "fifteen", "sixteen", "seventeen", "eighteen",
                "nineteen", "twenty", "twenty-one", "twenty-two", "twenty-three",
            ]
        )
    },
    "numbers": {
        f"{number:02}.mp3": word
        for number, word in enumerate(
            [
                "zero", "one", "two", "three", "four", "five", "six", "seven",
                "eight", "nine", "ten", "eleven", "twelve", "thirteen",
                "fourteen", "fifteen", "sixteen", "seventeen", "eighteen",
                "nineteen", "twenty", "twenty-one", "twenty-two", "twenty-three",
                "twenty-four", "twenty-five", "twenty-six", "twenty-seven",
                "twenty-eight", "twenty-nine", "thirty", "thirty-one", "thirty-two",
                "thirty-three", "thirty-four", "thirty-five", "thirty-six",
                "thirty-seven", "thirty-eight", "thirty-nine", "forty", "forty-one",
                "forty-two", "forty-three", "forty-four", "forty-five", "forty-six",
                "forty-seven", "forty-eight", "forty-nine", "fifty",
            ]
        )
    },
    "weather": {
        "clear.mp3": "Clear",
        "cloudy.mp3": "Cloudy",
        "fog.mp3": "Fog",
        "rain.mp3": "Rain",
        "thunderstorm.mp3": "Thunderstorm",
    },
}


def generate_audio_files() -> None:
    total = sum(len(files) for files in audio_data.values())
    completed = 0
    for folder, files in audio_data.items():
        destination = AUDIO_ROOT / folder
        destination.mkdir(parents=True, exist_ok=True)
        for filename, text in files.items():
            path = destination / filename
            print(f"Generating {path.relative_to(AUDIO_ROOT)}: {text}", flush=True)
            gTTS(text=text, lang="en", slow=False).save(str(path))
            completed += 1
            print(f"  [{completed}/{total}] saved", flush=True)
    print(f"Created {completed} MP3 files in {AUDIO_ROOT}")


if __name__ == "__main__":
    generate_audio_files()
