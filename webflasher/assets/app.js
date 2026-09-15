const compatibility = document.querySelector("#compatibility");
const compatibilityText = document.querySelector("#compatibility-text");
const selection = document.querySelector("#firmware-selection");
const selectionHelp = document.querySelector("#selection-help");
const hashElement = document.querySelector("#firmware-hash");
const copyButton = document.querySelector("#copy-hash");
const year = document.querySelector("#year");

const releases = {
  "cyd-1.9.2": {
    target: "CYD ESP32-2432S028R",
    shortTarget: "CYD",
    version: "1.9.2",
    state: "CYD FINAL",
    flash: "4 Mo",
    hash: "E69A9F4D4FCB9E2D733ABC24310183560B6EC0CD47294D9E7F5AD0D0CAC1E95A",
    help: "Version finale recommandée pour le Cheap Yellow Display.",
    description: "Image CYD fusionnée : flash 4 Mo, mode DIO, fréquence 40 MHz.",
    releaseDescription: "ESP32-2432S028R · Image fusionnée 4 Mo · DIO 40 MHz",
    warning: "Cette image est réservée au CYD ESP32-2432S028R. Une nouvelle installation peut effacer le contenu existant de la mémoire flash."
  },
  "cyd-1.7.5": {
    target: "CYD ESP32-2432S028R",
    shortTarget: "CYD",
    version: "1.7.5",
    state: "CYD HISTORIQUE",
    flash: "4 Mo",
    hash: "469A8912CD2A7BA3EF919467D84D6857CF60E726A27865157BD1A04653510218",
    help: "Ancienne version 1.7.5 conservée pour réinstallation volontaire.",
    description: "Image CYD historique fusionnée : flash 4 Mo, mode DIO, fréquence 40 MHz.",
    releaseDescription: "ESP32-2432S028R · Archive 1.7.5 · Image fusionnée 4 Mo",
    warning: "Vous avez choisi l’ancienne version CYD 1.7.5. Elle ne contient pas les protections et corrections ajoutées depuis cette version."
  },
  "p4-rev1-1.9.2": {
    target: "Waveshare ESP32-P4 révision 1.x",
    shortTarget: "P4 1.x",
    version: "1.9.2",
    state: "P4 FINAL",
    flash: "32 Mo",
    hash: "EF0E002C1A2AC80EBCB2C177C3F1204243F50EC5396928D0D0DE6A180F2D6B1A",
    help: "Réservé au silicium P4 révision 1.x, notamment 1.3.",
    description: "Image P4 révision 1.x fusionnée : flash 32 Mo, bootloader à 0x2000 et application à 0x10000.",
    releaseDescription: "Waveshare ESP32-P4 · Silicium 1.x · Version finale",
    warning: "DANGER : utilisez cette image uniquement avec un ESP32-P4 de révision silicium 1.x. Ne la flashez jamais sur un CYD ni sur un P4 3.x."
  },
  "p4-rev3-1.9.2": {
    target: "Waveshare ESP32-P4 révision 3.x",
    shortTarget: "P4 3.x",
    version: "1.9.2",
    state: "P4 FINAL",
    flash: "32 Mo",
    hash: "3B8620A3A43FF185CCCE469A44754265026905C30BDCE0FB773FE2DA2637B910",
    help: "Réservé au silicium P4 révision 3.x.",
    description: "Image P4 révision 3.x fusionnée : flash 32 Mo, bootloader à 0x2000 et application à 0x10000.",
    releaseDescription: "Waveshare ESP32-P4 · Silicium 3.x · Version finale",
    warning: "DANGER : utilisez cette image uniquement avec un ESP32-P4 de révision silicium 3.x. Ne la flashez jamais sur un CYD ni sur un P4 1.x."
  }
};

function updateSelection() {
  const releaseId = selection.value;
  const release = releases[releaseId];
  if (!release) return;

  document.querySelectorAll("esp-web-install-button").forEach((button) => {
    button.hidden = button.dataset.release !== releaseId;
  });
  document.querySelector("#header-target").textContent = release.target;
  document.querySelector("#header-version").textContent = `v${release.version}`;
  document.querySelector("#detail-target").textContent = release.shortTarget;
  document.querySelector("#detail-version").textContent = release.version;
  document.querySelector("#detail-flash").textContent = release.flash;
  document.querySelector("#build-description").textContent = release.description;
  document.querySelector("#target-warning").textContent = release.warning;
  document.querySelector("#release-version").textContent = `v${release.version}`;
  document.querySelector("#release-state").textContent = release.state;
  document.querySelector("#release-description").textContent = release.releaseDescription;
  document.querySelector("#release-hash").textContent = release.hash;
  selectionHelp.textContent = release.help;
  hashElement.textContent = release.hash;
  hashElement.title = release.hash;
  copyButton.textContent = "Copier l’empreinte";
}

if (window.isSecureContext && "serial" in navigator) {
  compatibility.classList.add("ready");
  compatibilityText.textContent = "Navigateur compatible — prêt à détecter l’appareil ESP";
} else {
  compatibility.classList.add("blocked");
  compatibilityText.textContent = window.isSecureContext
    ? "Web Serial indisponible — utilisez Chrome ou Microsoft Edge sur ordinateur"
    : "Connexion HTTPS requise pour accéder au port USB";
}

selection.addEventListener("change", updateSelection);
updateSelection();
year.textContent = `© ${new Date().getFullYear()}`;

copyButton.addEventListener("click", async () => {
  const hash = releases[selection.value]?.hash;
  if (!hash) return;
  try {
    await navigator.clipboard.writeText(hash);
    copyButton.textContent = "Empreinte copiée";
    window.setTimeout(() => {
      copyButton.textContent = "Copier l’empreinte";
    }, 1800);
  } catch {
    hashElement.focus?.();
    copyButton.textContent = "Copie impossible";
  }
});
