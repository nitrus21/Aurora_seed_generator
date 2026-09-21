const compatibility = document.querySelector("#compatibility");
const compatibilityText = document.querySelector("#compatibility-text");
const selection = document.querySelector("#firmware-selection");
const selectionHelp = document.querySelector("#selection-help");
const hashElement = document.querySelector("#firmware-hash");
const copyButton = document.querySelector("#copy-hash");
const applicationHashElement = document.querySelector("#application-hash");
const applicationHashNote = document.querySelector("#application-hash-note");
const copyApplicationHashButton = document.querySelector("#copy-application-hash");
const shaTableBody = document.querySelector("#sha-table-body");
const year = document.querySelector("#year");

const releases = {
  "p4-rev1-2.0.12": {
    target: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · silicium 1.x",
    version: "2.0.12",
    hash: "5C27560D7E021CA38BA9C8FABB59EF217E5E2536F22AF5AB9B4171ABBDB6F90A",
    applicationHash: "80187BFC3C9FCE3CC4D2E27E776A7D8902E514904B098B0393B7BB814F755F58",
    applicationHashDisplayed: true,
    help: "Dernière version, validée physiquement sur P4 révision 1.3.",
    warning: "DANGER : utilisez cette image uniquement avec un Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 1.x. Ne la flashez jamais sur un ESP32-2432S028R ni sur le silicium 3.x. Image testée sur P4 révision 1.3."
  },
  "p4-rev3-2.0.12": {
    target: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · silicium 3.x",
    version: "2.0.12",
    hash: "0CD58BA0C18A50A7073BEAB871154AFE622EEB8206F27C7BE5571DC57732F98E",
    applicationHash: "7D07208FD26376130BFAD73E87DE9DA03148A51F7B2B787C531498DE868A3BF5",
    applicationHashDisplayed: true,
    help: "Dernière version pour le silicium P4 révision 3.x ; contrôlée par logiciel, non testée sur carte 3.x.",
    warning: "DANGER : utilisez cette image uniquement avec un Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 3.x. Ne la flashez jamais sur un ESP32-2432S028R ni sur le silicium 1.x. Image contrôlée par logiciel, non testée sur carte 3.x."
  },
  "cyd-1.9.9": {
    target: "ESP32-2432S028R (CYD)",
    version: "1.9.9",
    hash: "34A7928985AA88D0E79F2D48EB5143A839E69106B2322DB78D2404EAB03DAD1E",
    applicationHash: "56AEC31DD77CA056E6D9843BA756C70D83765A1522A41078EA943472B3E3EEFD",
    applicationHashDisplayed: false,
    help: "Version finale de la branche CYD.",
    warning: "Cette image est réservée à l’ESP32-2432S028R. Une nouvelle installation peut effacer le contenu existant de la mémoire flash."
  }
};

function updateSelection() {
  const releaseId = selection.value;
  const release = releases[releaseId];
  if (!release) return;

  document.querySelectorAll("esp-web-install-button").forEach((button) => {
    button.hidden = button.dataset.release !== releaseId;
  });
  document.querySelector("#selected-board").textContent = release.target;
  document.querySelector("#target-warning").textContent = release.warning;
  selectionHelp.textContent = release.help;
  hashElement.textContent = release.hash;
  hashElement.title = release.hash;
  copyButton.textContent = "Copier l’empreinte";
  applicationHashElement.textContent = release.applicationHash;
  applicationHashElement.title = release.applicationHash;
  applicationHashNote.textContent = release.applicationHashDisplayed
    ? "C’est la valeur affichée dans À propos d’AURORA sur l’appareil."
    : "Référence extraite du binaire ; la version CYD ne l’affiche pas sur l’appareil.";
  copyApplicationHashButton.textContent = "Copier le SHA appareil";
}

function renderShaTable() {
  const rows = Object.entries(releases).map(([releaseId, release]) => {
    const row = document.createElement("tr");
    const values = [
      release.version,
      release.target,
      release.hash,
      release.applicationHash,
      release.applicationHashDisplayed ? "Affiché dans À propos" : "Non affiché par cette version"
    ];
    const labels = ["Version", "Cible", "SHA image Web Flasher", "SHA application / appareil", "Affichage appareil"];
    row.dataset.release = releaseId;
    row.replaceChildren(...values.map((value, index) => {
      const cell = document.createElement("td");
      cell.dataset.label = labels[index];
      if (index === 2 || index === 3) {
        const code = document.createElement("code");
        code.textContent = value;
        cell.replaceChildren(code);
      } else {
        cell.textContent = value;
      }
      return cell;
    }));
    return row;
  });
  shaTableBody.replaceChildren(...rows);
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
renderShaTable();
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

copyApplicationHashButton.addEventListener("click", async () => {
  const hash = releases[selection.value]?.applicationHash;
  if (!hash) return;
  try {
    await navigator.clipboard.writeText(hash);
    copyApplicationHashButton.textContent = "SHA appareil copié";
    window.setTimeout(() => {
      copyApplicationHashButton.textContent = "Copier le SHA appareil";
    }, 1800);
  } catch {
    applicationHashElement.focus?.();
    copyApplicationHashButton.textContent = "Copie impossible";
  }
});
