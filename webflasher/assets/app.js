const compatibility = document.querySelector("#compatibility");
const compatibilityText = document.querySelector("#compatibility-text");
const hashElement = document.querySelector("#firmware-hash");
const copyButton = document.querySelector("#copy-hash");
const year = document.querySelector("#year");

const firmwareHash = "469A8912CD2A7BA3EF919467D84D6857CF60E726A27865157BD1A04653510218";

if (window.isSecureContext && "serial" in navigator) {
  compatibility.classList.add("ready");
  compatibilityText.textContent = "Navigateur compatible — prêt à détecter l’ESP32";
} else {
  compatibility.classList.add("blocked");
  compatibilityText.textContent = window.isSecureContext
    ? "Web Serial indisponible — utilisez Chrome ou Microsoft Edge sur ordinateur"
    : "Connexion HTTPS requise pour accéder au port USB";
}

hashElement.textContent = firmwareHash;
hashElement.title = firmwareHash;
year.textContent = `© ${new Date().getFullYear()}`;

copyButton.addEventListener("click", async () => {
  try {
    await navigator.clipboard.writeText(firmwareHash);
    copyButton.textContent = "Empreinte copiée";
    window.setTimeout(() => {
      copyButton.textContent = "Copier l’empreinte";
    }, 1800);
  } catch {
    hashElement.focus?.();
    copyButton.textContent = "Copie impossible";
  }
});
