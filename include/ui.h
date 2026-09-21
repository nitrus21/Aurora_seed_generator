#pragma once
#include <lvgl.h>
#include "wallet.h"
#include "entropy.h"
#include "aurora_fonts.h"
#include "sd_export.h"
#include "aezeed.h"

#if defined(AURORA_BOARD_P4)
inline constexpr uintptr_t AURORA_ACTION_FINISH_ENTROPY = 152;
#endif

class AuroraUI {
 public:
  void begin();
  void tick();
  void onTouchSample(int16_t x, int16_t y, uint16_t pressure);
#if defined(AURORA_BOARD_P4)
  void refreshDisplayAfterClear();
#endif
  // Caller must quiesce tasks before terminal-failure use. No LVGL calls,
  // allocation, sensor/peripheral access, or release of dynamic buffers.
  void emergencyWipeSecrets() noexcept;

 private:
  enum class Screen : uint8_t {
    Splash, Mode, ImportName, ImportPassword,
#if defined(AURORA_BOARD_P4)
    About,
#endif
    RestoreSetup, RestoreWords, RestorePassphrase, Restoring,
    UmbrelWarning, UmbrelPassphrase, UmbrelProcessing, UmbrelResult, UmbrelQr,
    Setup, Passphrase, Entropy, Generating, FileProcessing, GenerationError,
    SecurityError, Mnemonic, PassphraseReveal, Verify, Info, Qr, Backup, ExportWarning,
    ExportName, ExportPassword, Wipe,
    PrivatePassword,
    SdRequired
  };
  enum class FileOperation : uint8_t { None, Export, Import, PrivateRead };
  enum class QrContent : uint8_t { Address, FirstPublicKey, AccountXpub, PrivateKey };
  void show(Screen screen);
  void clear();
  lv_obj_t *header(const char *title, const char *step = nullptr, bool showBrand = true);
  lv_obj_t *button(lv_obj_t *parent, const char *text, lv_event_cb_t cb, int w = 126);
  lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font = &aurora_font_14);
  lv_obj_t *explanation(lv_obj_t *parent, const char *text,
                        const lv_font_t *font = &aurora_font_10);
  void buildSplash(); void buildMode(); void buildImportName(); void buildImportPassword();
#if defined(AURORA_BOARD_P4)
  void buildAbout();
#endif
  void buildRestoreSetup(); void buildRestoreWords(); void buildRestorePassphrase();
  void buildRestoring(); void updateRestoreSuggestions(); bool acceptRestoreWord(const char *word);
  bool restoreEnteredWallet(); bool rederiveManualWallet(AddressKind kind);
  void buildUmbrelWarning(); void buildUmbrelPassphrase(); void buildUmbrelProcessing();
  void buildUmbrelResult(); void buildUmbrelQr(); bool recoverUmbrel();
  bool deriveUmbrelPublicAccounts(const char *rootXprv);
  bool deriveSelectedUmbrelPublicChild();
  void selectUmbrelScope(uint8_t index); void buildUmbrelScopeSelector();
  bool deriveBip39PublicAccounts(const char *mnemonic, uint8_t words,
                                 const char *passphrase, AddressKind initialKind);
  bool deriveSelectedBip39PublicChild();
  void selectBip39Scope(uint8_t index);
  void buildSetup(); void buildPassphrase(); void buildEntropy();
  void updateEntropyPreview(uint32_t token);
  void buildGenerating(); void buildFileProcessing();
  void buildGenerationError(); void buildSecurityError();
  void buildMnemonic(); void buildPassphraseReveal(); void buildVerify();
  void updateVerifySuggestions(); void acceptVerifySuggestion(uint8_t index);
  void buildInfo(); void buildQr();
  void buildBackup(); void buildExportWarning(); void buildExportName();
  void buildExportPassword(); void buildWipe();
  void buildPassphraseFields(bool restoring);
  bool confirmPassphrase();
  bool needsSd(Screen screen) const;
  bool ensureSd(Screen resume);
  void buildSdRequired();
  Screen afterSd_ = Screen::Mode;
  void wipeSession(); void closeSession();
  enum class Access : uint8_t { None, Words, Passphrase, PrivateQr, UmbrelXprv, Export };
  Access accessFor(Screen screen) const;
  bool authorized(Access access) const;
  static uint32_t accessDurationMs(Access access);
  void updateSecretCountdown();
  void wipeFileCredentials();
  bool hasPassphrase() const;
  void revokeAccess();
  bool generate(); void selectVerifyWords(); bool verifyWords();
  bool renderQr(lv_obj_t *parent, const char *data, int size = 158, int x = 6, int y = 42);
  void performWalletExport(); bool performWalletImport();
  static void event(lv_event_t *e);
  void dropPrivateState();
  void startFileSession(const uint8_t fingerprint[32], const char *baseName);
  bool loadPrivateWallet();
  bool applyUmbrelWalletData(const AuroraWalletData &data, bool keepPrivate);
  void submitPrivatePassword();
  // Public identity only: no password, PIN verifier, capsule or file key.
  uint8_t sessionFingerprint_[32]{};
  bool fileSession_ = false;
  bool privateLoaded_ = false;
  bool sessionHasPassphrase_ = false;
  char sessionBaseName_[25]{};
  // Sticky for the entire workflow, including initial inputs and error pages.
  // Only wiping the session ends it; screen navigation never renews its timer.
  bool sensitiveStateActive_ = false;
#if defined(AURORA_BOARD_P4)
  void buildPortraitEntropy();
  void updatePortraitSensors();
  void updatePortraitEntropyReadyState();
  void finishPortraitEntropy();
  bool displayRefreshPending_ = false;
  bool sensorStopPending_ = false;
  Screen afterSensorStop_ = Screen::Mode;
  uint32_t sensorStopStarted_ = 0;
  uint32_t sensorUiUpdated_ = 0;
  uint32_t cameraPreviewSequence_ = 0;
  lv_obj_t *microphoneStatus_ = nullptr;
  lv_obj_t *microphoneLevel_ = nullptr;
  lv_obj_t *cameraStatus_ = nullptr;
  lv_obj_t *cameraPreview_ = nullptr;
  lv_obj_t *entropyFinishButton_ = nullptr;
  uint16_t *cameraPixels_ = nullptr;
  lv_image_dsc_t cameraImage_{};
#endif

  Screen screen_ = Screen::Splash;
  WalletEngine engine_;
  WalletOutput wallet_{};
  TouchEntropy entropy_;
  bool protectedSession_ = false;
  bool entropyCollected_ = false;
  bool exportSucceeded_ = false;
  Access access_ = Access::None;
  Access requestedAccess_ = Access::None;
  Screen afterAuthentication_ = Screen::Info;
  uint32_t accessGrantedMs_ = 0;
  static constexpr uint32_t SECRET_VISIBLE_MS = 15000;
  static constexpr uint32_t WORDS_VISIBLE_MS = 180000;
  static constexpr uint32_t PRIVATE_KEY_VISIBLE_MS = 60000;
  static constexpr uint32_t UMBREL_XPRV_VISIBLE_MS = 180000;
  static constexpr uint32_t SESSION_IDLE_MS = 120000;
  static constexpr uint32_t EXPORT_AUTH_MS = 120000;
  // Authenticated .aurora views only: one deadline shared by every word page.
  // Public timing metadata only; never a credential or decryption key.
  Access visibleSecret_ = Access::None;
  uint32_t visibleSecretStartedMs_ = 0;
  uint32_t countdownSeconds_ = UINT32_MAX;
  lv_obj_t *secretCountdown_ = nullptr;
  lv_obj_t *passConfirmArea_ = nullptr;
  lv_obj_t *securityStatus_ = nullptr;
  uint8_t mixedEntropy_[32]{};
  uint8_t words_ = 12;
  uint8_t mnemonicPage_ = 0;
  bool entropyReadyPending_ = false;
  bool entropyFailurePending_ = false;
  bool selfTestPending_ = true;
  WalletSelfTest selfTestResult_ = WalletSelfTest::Ok;
  uint32_t selfTestDueMs_ = 0;
  uint32_t generationDueMs_ = 0;
  uint32_t entropyCompleteDueMs_ = 0;
  uint32_t entropyPreviewUpdatedMs_ = 0;
  uint32_t fileOperationDueMs_ = 0;
  AddressKind kind_ = AddressKind::NativeSegwit;
  char passphrase_[64]{};
  char filePassword_[64]{};
  uint8_t verifyIndex_[3]{};
  lv_obj_t *root_ = nullptr;
  lv_obj_t *passArea_ = nullptr;
  lv_obj_t *entropyBar_ = nullptr;
  lv_obj_t *entropyStatus_ = nullptr;
  lv_obj_t *entropyPreview_ = nullptr;
  lv_obj_t *entropyCount_ = nullptr;
  char entropyPreviewText_[36]{};
  lv_obj_t *verifyArea_[3]{};
  lv_obj_t *verifySuggestionButtons_[3]{};
  lv_obj_t *keyboard_ = nullptr;
  lv_obj_t *exportNameArea_ = nullptr;
  lv_obj_t *importFileDropdown_ = nullptr;
  lv_obj_t *restoreWordArea_ = nullptr;
  lv_obj_t *restoreSuggestionButtons_[3]{};
  lv_obj_t *restoreDerivationDropdown_ = nullptr;
  lv_obj_t *umbrelScopeDropdown_ = nullptr;
  lv_obj_t *umbrelIndexDropdown_ = nullptr;
  lv_obj_t *filePasswordArea_ = nullptr;
  lv_obj_t *filePasswordConfirmArea_ = nullptr;
  QrContent qrContent_ = QrContent::Address;
  WalletExportFormat exportFormat_ = WalletExportFormat::AuroraWallet;
  FileOperation fileOperation_ = FileOperation::None;
  bool loadedWallet_ = false;
  bool manualRestore_ = false;
  bool umbrelRecovery_ = false;
  bool umbrelWallet_ = false;
  struct UmbrelPublicAccount {
    char address[96];
    char publicKey[80];
    char accountXpub[128];
    char path[32];
    AddressKind kind;
  } umbrelPublic_[3]{};
  uint8_t umbrelScope_ = 1; // BIP84 is the initial view, never the only scope.
  uint8_t umbrelAddressIndex_ = 0;
  UmbrelPublicAccount bip39Public_[4]{};
  bool bip39PublicReady_ = false;
  uint8_t bip39Scope_ = static_cast<uint8_t>(AddressKind::NativeSegwit);
  uint8_t bip39AddressIndex_ = 0;
  uint8_t restoreWordIndex_ = 0;
  uint8_t restoreSuggestionCount_ = 0;
  uint8_t verifyActiveIndex_ = 0;
  uint8_t verifySuggestionCount_ = 0;
  char restoreWords_[24][WalletEngine::BIP39_WORD_CAPACITY]{};
  char restoreSuggestions_[3][WalletEngine::BIP39_WORD_CAPACITY]{};
  char verifySuggestions_[3][WalletEngine::BIP39_WORD_CAPACITY]{};
  char restoreMnemonic_[256]{};
  char restoreStatus_[96]{};
  char umbrelRootXprv_[128]{};
  uint16_t umbrelBirthdayDays_ = 0;
  AezeedResult umbrelResult_ = AezeedResult::Ok;
  char exportBaseName_[25] = "aurora";
  char importBaseName_[25] = "aurora";
  char auroraFileOptions_[2048]{};
  uint16_t auroraFileCount_ = 0;
  char exportStatus_[80]{};
  char importStatus_[96]{};
  char passwordStatus_[80]{};
};
