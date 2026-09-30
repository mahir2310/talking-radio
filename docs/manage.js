const form = document.querySelector("#library-form");
const baseUrlInput = document.querySelector("#base-url");
const versionInput = document.querySelector("#version");
const motivationInput = document.querySelector("#motivation");
const surahInput = document.querySelector("#surah");
const jsonOutput = document.querySelector("#json-output");
const statusElement = document.querySelector("#manager-status");
const uploadMotivation = document.querySelector("#upload-motivation");
const uploadSurah = document.querySelector("#upload-surah");
const editConfig = document.querySelector("#edit-config");

let currentConfig;

function lines(value) {
  return value
    .split("\n")
    .map((line) => line.trim())
    .filter(Boolean);
}

function githubUrl(path) {
  const owner = currentConfig.repository.owner;
  const repository = currentConfig.repository.name;
  return `https://github.com/${owner}/${repository}/${path}`;
}

function updateGithubLinks() {
  uploadMotivation.href = githubUrl("upload/main/docs/audio/motivation");
  uploadSurah.href = githubUrl("upload/main/docs/audio/surah");
  editConfig.href = githubUrl("edit/main/docs/config.json");
  [uploadMotivation, uploadSurah, editConfig].forEach((link) => {
    link.target = "_blank";
    link.rel = "noopener noreferrer";
  });
}

function renderJson(incrementVersion) {
  const nextVersion = Number(versionInput.value || 1) + (incrementVersion ? 1 : 0);
  versionInput.value = String(nextVersion);
  currentConfig.version = nextVersion;
  currentConfig.base_url = baseUrlInput.value.trim();
  currentConfig.motivation = lines(motivationInput.value);
  currentConfig.surah = lines(surahInput.value);
  jsonOutput.value = `${JSON.stringify(currentConfig, null, 2)}\n`;
}

async function loadConfig() {
  try {
    const response = await fetch(`config.json?manager=${Date.now()}`, {
      cache: "no-store",
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    currentConfig = await response.json();

    const owner = currentConfig.repository?.owner;
    const repository = currentConfig.repository?.name;
    if (!owner || !repository || owner.includes("YOUR_")) {
      throw new Error("Set repository.owner and repository.name in config.json first");
    }

    baseUrlInput.value = currentConfig.base_url;
    versionInput.value = String(currentConfig.version || 1);
    motivationInput.value = (currentConfig.motivation ?? []).join("\n");
    surahInput.value = (currentConfig.surah ?? []).join("\n");
    renderJson(false);
    updateGithubLinks();
    statusElement.textContent = "Current library loaded.";
  } catch (error) {
    statusElement.textContent = `Setup needed: ${error.message}`;
    form.querySelectorAll("input, textarea, button").forEach((control) => {
      control.disabled = true;
    });
  }
}

form.addEventListener("submit", (event) => {
  event.preventDefault();
  renderJson(true);
  statusElement.textContent =
    "Update prepared. Copy the JSON, paste it into GitHub's config editor, and commit.";
});

document.querySelector("#copy-json").addEventListener("click", async () => {
  try {
    await navigator.clipboard.writeText(jsonOutput.value);
    statusElement.textContent = "JSON copied. Paste it over config.json on GitHub.";
  } catch {
    jsonOutput.select();
    statusElement.textContent = "Select and copy the highlighted JSON manually.";
  }
});

document.querySelector("#download-json").addEventListener("click", () => {
  const blob = new Blob([jsonOutput.value], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = "config.json";
  anchor.click();
  URL.revokeObjectURL(url);
  statusElement.textContent = "config.json downloaded.";
});

loadConfig();

