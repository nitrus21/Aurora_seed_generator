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
  "cyd-1.9.9": {
    target: "ESP32-2432S028R (CYD)",
    version: "1.9.9",
    state: "DERNIÈRE VERSION",
    flash: "4 Mo",
    hash: "34A7928985AA88D0E79F2D48EB5143A839E69106B2322DB78D2404EAB03DAD1E",
    applicationHash: "56AEC31DD77CA056E6D9843BA756C70D83765A1522A41078EA943472B3E3EEFD",
    applicationHashDisplayed: false,
    notes: "./releases/1.9.9.md",
    changes: ["Sauvegarde et ouverture .aurora sur microSD, avec mot de passe redemandé pour chaque consultation privée.", "Mots : 3 min ; clé privée : 1 min ; retour au portefeuille public après consultation.", "Boutons publics en vert, actions privées en rouge ; interface compacte et clavier classique."],
    help: "Version finale pour l’ESP32-2432S028R.",
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
    applicationHash: "D0BF637D2C920F2B2C8191DB9D2628F0F5686AE21C1452BB9FF6B767FEFE1656",
    applicationHashDisplayed: false,
    notes: "./releases/1.7.5.md",
    changes: ["Version 1.7.5 conservée, sans modification.", "Pour la version finale de cette carte, choisissez la 1.9.9."],
    help: "Ancienne version 1.7.5 conservée pour réinstallation volontaire.",
    description: "Image ESP32-2432S028R fusionnée : flash 4 Mo, mode DIO, fréquence 40 MHz.",
    releaseDescription: "ESP32-2432S028R · Archive 1.7.5 · Image fusionnée 4 Mo",
    warning: "Vous avez choisi la version 1.7.5 pour l’ESP32-2432S028R. Elle ne contient pas les protections et corrections ajoutées depuis cette version."
  },
  "p4-rev1-2.0.12": {
    target: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · silicium 1.x",
    version: "2.0.12",
    state: "DERNIÈRE VERSION",
    flash: "32 Mo",
    hash: "5C27560D7E021CA38BA9C8FABB59EF217E5E2536F22AF5AB9B4171ABBDB6F90A",
    applicationHash: "80187BFC3C9FCE3CC4D2E27E776A7D8902E514904B098B0393B7BB814F755F58",
    applicationHashDisplayed: true,
    notes: "./releases/2.0.12.md",
    changes: ["Collecte renforcée : 512 mouvements, durée active et couverture de la zone.", "Aperçu figé lorsque le doigt reste immobile, puis validation explicite avec TERMINER.", "Microphone calibré avec seuil adaptatif ; aucune donnée audio brute affichée ou stockée."],
    help: "Dernière version, validée physiquement sur P4 révision 1.3.",
    description: "Image Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 1.x fusionnée : flash 32 Mo, bootloader à 0x2000 et application à 0x10000.",
    releaseDescription: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · Silicium 1.x · 2.0.12",
    warning: "DANGER : utilisez cette image uniquement avec un Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 1.x. Ne la flashez jamais sur un ESP32-2432S028R ni sur le silicium 3.x. Image testée sur P4 révision 1.3."
  },
  "p4-rev3-2.0.12": {
    target: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · silicium 3.x",
    version: "2.0.12",
    state: "DERNIÈRE VERSION",
    flash: "32 Mo",
    hash: "0CD58BA0C18A50A7073BEAB871154AFE622EEB8206F27C7BE5571DC57732F98E",
    applicationHash: "7D07208FD26376130BFAD73E87DE9DA03148A51F7B2B787C531498DE868A3BF5",
    applicationHashDisplayed: true,
    notes: "./releases/2.0.12.md",
    changes: ["Collecte renforcée : 512 mouvements, durée active et couverture de la zone.", "Aperçu figé lorsque le doigt reste immobile, puis validation explicite avec TERMINER.", "Microphone calibré avec seuil adaptatif ; aucune donnée audio brute affichée ou stockée."],
    help: "Dernière version pour le silicium P4 révision 3.x ; contrôlée par logiciel, non testée sur carte 3.x.",
    description: "Image Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 3.x fusionnée : flash 32 Mo, bootloader à 0x2000 et application à 0x10000.",
    releaseDescription: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · Silicium 3.x · 2.0.12",
    warning: "DANGER : utilisez cette image uniquement avec un Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 3.x. Ne la flashez jamais sur un ESP32-2432S028R ni sur le silicium 1.x. Image contrôlée par logiciel, non testée sur carte 3.x."
  },
  "p4-rev1-2.0.11": {
    target: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · silicium 1.x",
    version: "2.0.11",
    state: "VERSION PRÉCÉDENTE",
    flash: "32 Mo",
    hash: "962A04E1FF65D0CE38FD13E3700F74E15852315999C031E972761497D3E0994E",
    applicationHash: "0EFC106031B41BF4ED718C596F6A773E0D2B8CA4CB3237845BEB667D0670F40F",
    applicationHashDisplayed: true,
    notes: "./releases/2.0.11.md",
    changes: ["Journaux de production désactivés, protection de pile forte et builds reproductibles.", "Page À propos : version, cible silicium et SHA-256 de l’application installée.", "Libellés de dérivation explicites et clé étendue de compte à la place de la clé publique enfant."],
    help: "Version précédente, validée physiquement sur P4 révision 1.3.",
    description: "Image Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 1.x fusionnée : flash 32 Mo, bootloader à 0x2000 et application à 0x10000.",
    releaseDescription: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · Silicium 1.x · 2.0.11",
    warning: "DANGER : utilisez cette image uniquement avec un Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 1.x. Ne la flashez jamais sur un ESP32-2432S028R ni sur le silicium 3.x. Image testée sur P4 révision 1.3."
  },
  "p4-rev3-2.0.11": {
    target: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · silicium 3.x",
    version: "2.0.11",
    state: "VERSION PRÉCÉDENTE",
    flash: "32 Mo",
    hash: "7B6F2C69988862233341413677AD866131AABEC82F319A7DBF3F891962F2B0A7",
    applicationHash: "C54EC276907DBA8278A96BF67AF032F91AFECDD4DE0628BBF464CC94CF64F02F",
    applicationHashDisplayed: true,
    notes: "./releases/2.0.11.md",
    changes: ["Journaux de production désactivés, protection de pile forte et builds reproductibles.", "Page À propos : version, cible silicium et SHA-256 de l’application installée.", "Libellés de dérivation explicites et clé étendue de compte à la place de la clé publique enfant."],
    help: "Version précédente pour le silicium P4 révision 3.x ; contrôlée par logiciel, non testée sur carte 3.x.",
    description: "Image Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 3.x fusionnée : flash 32 Mo, bootloader à 0x2000 et application à 0x10000.",
    releaseDescription: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · Silicium 3.x · 2.0.11",
    warning: "DANGER : utilisez cette image uniquement avec un Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 3.x. Ne la flashez jamais sur un ESP32-2432S028R ni sur le silicium 1.x. Image contrôlée par logiciel, non testée sur carte 3.x."
  },
  "p4-rev1-2.0.9": {
    target: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · silicium 1.x",
    version: "2.0.9",
    state: "VERSION PRÉCÉDENTE",
    flash: "32 Mo",
    hash: "F4F10C4D9D9204C949FCF544EFD1469F9225814C1C17B57B48AAC6E971C7B384",
    applicationHash: "5929716877E3585A9B8F6153A67180D7BC4805EE3795DC39BDA8E98D2C0E6FDC",
    applicationHashDisplayed: false,
    notes: "./releases/2.0.9.md",
    changes: ["Navigation publique BIP44/49/84/86 et vingt adresses après un parcours BIP39.", "Navigation publique BIP49/84/86 après récupération Umbrel/LND.", "Les fichiers ouverts restent liés à leur dérivation enregistrée et authentifiée."],
    help: "Dernière version, réservée au silicium P4 révision 1.x, notamment 1.3.",
    description: "Image Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 1.x fusionnée : flash 32 Mo, bootloader à 0x2000 et application à 0x10000.",
    releaseDescription: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · Silicium 1.x · 2.0.9",
    warning: "DANGER : utilisez cette image uniquement avec un Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 1.x. Ne la flashez jamais sur un ESP32-2432S028R ni sur le silicium 3.x. Ce binaire 2.0.9 attend encore sa recette matérielle."
  },
  "p4-rev3-2.0.9": {
    target: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · silicium 3.x",
    version: "2.0.9",
    state: "VERSION PRÉCÉDENTE",
    flash: "32 Mo",
    hash: "128CB229102C4FCD45416574C2DF08B954EE1DE4DF1311FA8D3A67AE1CA94F4D",
    applicationHash: "777D73D05A4E17D31FE9D15E30568A245ED80A1646BD6C97483B5AD389E64334",
    applicationHashDisplayed: false,
    notes: "./releases/2.0.9.md",
    changes: ["Navigation publique BIP44/49/84/86 et vingt adresses après un parcours BIP39.", "Navigation publique BIP49/84/86 après récupération Umbrel/LND.", "Les fichiers ouverts restent liés à leur dérivation enregistrée et authentifiée."],
    help: "Dernière version pour le silicium P4 révision 3.x ; contrôlée par logiciel, non testée sur carte 3.x.",
    description: "Image Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 3.x fusionnée : flash 32 Mo, bootloader à 0x2000 et application à 0x10000.",
    releaseDescription: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · Silicium 3.x · 2.0.9",
    warning: "DANGER : utilisez cette image uniquement avec un Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 3.x. Ne la flashez jamais sur un ESP32-2432S028R ni sur le silicium 1.x. Image contrôlée par logiciel, non testée sur carte 3.x."
  },
  "p4-rev1-2.0.5": {
    target: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · silicium 1.x",
    version: "2.0.5",
    state: "VERSION PRÉCÉDENTE",
    flash: "32 Mo",
    hash: "6D6253867D4962AACDB012F0445444E386C519044543F41DAE20CA04517722CB",
    applicationHash: "F93C6F3635594A6523783F26674F38A6F3627E4DB89E210B947D1D19047F2417",
    applicationHashDisplayed: false,
    notes: "./releases/2.0.5.md",
    changes: ["Umbrel/LND : QR xprv avec décompte de 3 minutes.", "Sauvegarde .aurora chiffrée et réouverture publique du xpub.", "Export Sparrow rouge en clair ; mots AEZEED et passphrase jamais enregistrés."],
    help: "Réservé au silicium P4 révision 1.x, notamment 1.3.",
    description: "Image Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 1.x fusionnée : flash 32 Mo, bootloader à 0x2000 et application à 0x10000.",
    releaseDescription: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · Silicium 1.x · 2.0.5",
    warning: "DANGER : utilisez cette image uniquement avec un Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 1.x. Ne la flashez jamais sur un ESP32-2432S028R ni sur le silicium 3.x."
  },
  "p4-rev3-2.0.5": {
    target: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · silicium 3.x",
    version: "2.0.5",
    state: "VERSION PRÉCÉDENTE",
    flash: "32 Mo",
    hash: "2BC03AAC78FBF9583B6B4918C9A89D807A55D96B554909B21FE486C67CA77884",
    applicationHash: "943765C9C3BAD234719AA5DAAE4CB0C80A5E8B4755BC2BF72484BFCEA3EDE65B",
    applicationHashDisplayed: false,
    notes: "./releases/2.0.5.md",
    changes: ["Umbrel/LND : QR xprv avec décompte de 3 minutes.", "Sauvegarde .aurora chiffrée et réouverture publique du xpub.", "Export Sparrow rouge en clair ; mots AEZEED et passphrase jamais enregistrés."],
    help: "Réservé au silicium P4 révision 3.x. Image contrôlée par logiciel, non testée sur carte 3.x.",
    description: "Image Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 3.x fusionnée : flash 32 Mo, bootloader à 0x2000 et application à 0x10000.",
    releaseDescription: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · Silicium 3.x · 2.0.5",
    warning: "DANGER : utilisez cette image uniquement avec un Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, silicium 3.x. Ne la flashez jamais sur un ESP32-2432S028R ni sur le silicium 1.x."
  },
  "p4-rev1-2.0.3": {
    target: "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 · silicium 1.x",
    version: "2.0.3",
    state: "VERSION CONSERVÉE",
    flash: "32 Mo",
    hash: "C4DC3D3A0178402CCFB1F9A5B16246C346A0BDD4AAB5DFEA4FE8CC84E1261594",
    applicationHash: "E17FB7BF23B0DC95CF9E4D4F383CD118E2C003AB351E8BA2B03B46E77F884AD6",
    applicationHashDisplayed: false,
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
    state: "VERSION CONSERVÉE",
    flash: "32 Mo",
    hash: "72B909397C61C5731DFDA801C6BB2F503941152811FB13E740AB83CB97CCA6B7",
    applicationHash: "B82D846CF5784320F80C743AE30D5CC1B45A11B748B6C73A92C76022C276DE53",
    applicationHashDisplayed: false,
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
  applicationHashElement.textContent = release.applicationHash;
  applicationHashElement.title = release.applicationHash;
  applicationHashNote.textContent = release.applicationHashDisplayed
    ? "C’est la valeur affichée dans À propos d’AURORA sur l’appareil."
    : "Référence extraite du binaire ; cette ancienne version ne l’affiche pas sur l’appareil.";
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
