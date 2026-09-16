const compatibility = document.querySelector("#compatibility");
const compatibilityText = document.querySelector("#compatibility-text");
const selection = document.querySelector("#firmware-selection");
const selectionHelp = document.querySelector("#selection-help");
const hashElement = document.querySelector("#firmware-hash");
const copyButton = document.querySelector("#copy-hash");
const year = document.querySelector("#year");

const releases = {
  "cyd-1.9.7": {
    target: "ESP32-2432S028R (CYD)",
    version: "1.9.7",
    state: "DERNIÈRE VERSION",
    flash: "4 Mo",
    hash: "3FD8388FD5662B106989FF30C7B25DEED2E55D9D399AECA2C03163EA51C8FBAC",
    notes: "./releases/1.9.7.md",
    changes: ["Sauvegardes .aurora : KDF à 500 000 itérations et mot de passe par consultation.", "Mots : 3 min ; clé privée : 1 min, uniquement depuis un fichier authentifié.", "RNG contrôlé, nettoyage renforcé ; polices et clavier classiques conservés."],
    help: "Dernière version pour l’ESP32-2432S028R.",
    description: "Image ESP32-2432S028R fusionnée : flash 4 Mo, mode DIO, fréquence 40 MHz.",
    releaseDescription: "ESP32-2432S028R · Image fusionnée 4 Mo · DIO 40 MHz",
    warning: "Cette image est réservée à l’ESP32-2432S028R. Une nouvelle installation peut effacer le contenu existant de la mémoire flash."
  },
  "cyd-1.7.5": {
    target: "ESP32-2432S028R (CYD)",
    version: "1.7.5",
    state: "VERSION CONSERVÉE",
    flash: "4 Mo",
    hash: "469A8912CD2A7BA3EF919467D84D6857CF60E726A27865157BD1A04653510218",
    notes: "./releases/1.7.5.md",
    changes: ["Version 1.7.5 conservée, sans modification.", "Pour les corrections récentes, choisissez la 1.9.7."],
    help: "Ancienne version 1.7.5 conservée pour réinstallation volontaire.",
    description: "Image ESP32-2432S028R fusionnée : flash 4 Mo, mode DIO, fréquence 40 MHz.",
    releaseDescription: "ESP32-2432S028R · Archive 1.7.5 · Image fusionnée 4 Mo",
    warning: "Vous avez choisi la version 1.7.5 pour l’ESP32-2432S028R. Elle ne contient pas les protections et corrections ajoutées depuis cette version."
  },
  "p4-rev1-2.0.3": {
    target: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · silicium 1.x",
    version: "2.0.3",
    state: "DÉVELOPPEMENT ACTIF",
    flash: "32 Mo",
    hash: "C4DC3D3A0178402CCFB1F9A5B16246C346A0BDD4AAB5DFEA4FE8CC84E1261594",
    notes: "./releases/2.0.3.md",
    changes: ["KDF à 500 000 itérations, accéléré par blocs sur P4.", "RNG et tampons SD renforcés ; mots 3 min / clé privée 1 min depuis le fichier.", "Interface portrait, 12 mots par page et récupération Umbrel/LND conservées."],
    help: "Réservé au silicium P4 révision 1.x, notamment 1.3.",
    description: "Image Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 1.x fusionnée : flash 32 Mo, bootloader à 0x2000 et application à 0x10000.",
    releaseDescription: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · Silicium 1.x · 2.0.3",
    warning: "DANGER : utilisez cette image uniquement avec un Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 1.x. Ne la flashez jamais sur un ESP32-2432S028R ni sur le silicium 3.x."
  },
  "p4-rev3-2.0.3": {
    target: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · silicium 3.x",
    version: "2.0.3",
    state: "DÉVELOPPEMENT ACTIF",
    flash: "32 Mo",
    hash: "72B909397C61C5731DFDA801C6BB2F503941152811FB13E740AB83CB97CCA6B7",
    notes: "./releases/2.0.3.md",
    changes: ["KDF à 500 000 itérations, accéléré par blocs sur P4.", "RNG et tampons SD renforcés ; mots 3 min / clé privée 1 min depuis le fichier.", "Interface portrait, 12 mots par page et récupération Umbrel/LND conservées."],
    help: "Réservé au silicium P4 révision 3.x. Image contrôlée par logiciel, non testée sur carte 3.x.",
    description: "Image Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 3.x fusionnée : flash 32 Mo, bootloader à 0x2000 et application à 0x10000.",
    releaseDescription: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · Silicium 3.x · 2.0.3",
    warning: "DANGER : utilisez cette image uniquement avec un Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 3.x. Ne la flashez jamais sur un ESP32-2432S028R ni sur le silicium 1.x."
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
  document.querySelector("#detail-target").textContent = release.target;
  document.querySelector("#selected-board").textContent = release.target;
  document.querySelector("#detail-version").textContent = release.version;
  document.querySelector("#detail-flash").textContent = release.flash;
  document.querySelector("#build-description").textContent = release.description;
  document.querySelector("#target-warning").textContent = release.warning;
  document.querySelector("#release-version").textContent = `v${release.version}`;
  document.querySelector("#release-state").textContent = release.state;
  document.querySelector("#release-description").textContent = release.releaseDescription;
  document.querySelector("#release-hash").textContent = release.hash;
  document.querySelector("#release-notes").href = release.notes;
  document.querySelector("#changes-title").textContent = release.version === "1.7.5"
    ? "Version conservée" : "Nouveautés de cette version";
  document.querySelector("#release-changes").replaceChildren(...release.changes.map((change) => {
    const item = document.createElement("li");
    item.textContent = change;
    return item;
  }));
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
