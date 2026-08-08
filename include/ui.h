#pragma once
#include <lvgl.h>
#include "wallet.h"
#include "entropy.h"
#include "aurora_fonts.h"
#include "sd_export.h"

class AuroraUI {
 public:
  void begin();
  void tick();
  void onTouchSample(int16_t x, int16_t y, uint16_t pressure);

 private:
  enum class Screen : uint8_t {
    Splash, Mode, ImportName, ImportPassword,
    RestoreSetup, RestoreWords, RestorePassphrase, Restoring,
    Setup, Passphrase, Entropy, Generating, FileProcessing, GenerationError,
    SecurityError, Mnemonic, PassphraseReveal, Verify, Info, Qr, Backup, ExportWarning,
    ExportName, ExportPassword, Wipe
  };
  enum class FileOperation : uint8_t { None, Export, Import };
  enum class QrContent : uint8_t { Address, AccountXpub, PrivateKey };
  void show(Screen screen);
  void clear();
  lv_obj_t *header(const char *title, const char *step = nullptr);
  lv_obj_t *button(lv_obj_t *parent, const char *text, lv_event_cb_t cb, int w = 126);
  lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font = &aurora_font_14);
  void buildSplash(); void buildMode(); void buildImportName(); void buildImportPassword();
  void buildRestoreSetup(); void buildRestoreWords(); void buildRestorePassphrase();
  void buildRestoring(); void updateRestoreSuggestions(); bool acceptRestoreWord(const char *word);
  bool restoreEnteredWallet(); bool rederiveManualWallet(AddressKind kind);
  void buildSetup(); void buildPassphrase(); void buildEntropy();
  void buildGenerating(); void buildFileProcessing();
  void buildGenerationError(); void buildSecurityError();
  void buildMnemonic(); void buildPassphraseReveal(); void buildVerify();
  void updateVerifySuggestions(); void acceptVerifySuggestion(uint8_t index);
  void buildInfo(); void buildQr();
  void buildBackup(); void buildExportWarning(); void buildExportName();
  void buildExportPassword(); void buildWipe();
  bool generate(); void selectVerifyWords(); bool verifyWords();
  bool renderQr(lv_obj_t *parent, const char *data, int size = 158, int x = 6, int y = 42);
  void performWalletExport(); bool performWalletImport();
  static void event(lv_event_t *e);

  Screen screen_ = Screen::Splash;
  WalletEngine engine_;
  WalletOutput wallet_{};
  TouchEntropy entropy_;
  uint8_t mixedEntropy_[32]{};
  uint8_t words_ = 12;
  uint8_t mnemonicPage_ = 0;
  bool entropyReadyPending_ = false;
  bool entropyFailurePending_ = false;
  bool selfTestPending_ = true;
  WalletSelfTest selfTestResult_ = WalletSelfTest::Ok;
  uint32_t selfTestDueMs_ = 0;
  uint32_t generationDueMs_ = 0;
  uint32_t fileOperationDueMs_ = 0;
  AddressKind kind_ = AddressKind::NativeSegwit;
  char passphrase_[64]{};
  char filePassword_[64]{};
  uint8_t verifyIndex_[3]{};
  lv_obj_t *root_ = nullptr;
  lv_obj_t *passArea_ = nullptr;
  lv_obj_t *entropyBar_ = nullptr;
  lv_obj_t *entropyStatus_ = nullptr;
  lv_obj_t *verifyArea_[3]{};
  lv_obj_t *verifySuggestionButtons_[3]{};
  lv_obj_t *keyboard_ = nullptr;
  lv_obj_t *exportNameArea_ = nullptr;
  lv_obj_t *importFileDropdown_ = nullptr;
  lv_obj_t *restoreWordArea_ = nullptr;
  lv_obj_t *restoreSuggestionButtons_[3]{};
  lv_obj_t *restoreDerivationDropdown_ = nullptr;
  lv_obj_t *filePasswordArea_ = nullptr;
  lv_obj_t *filePasswordConfirmArea_ = nullptr;
  QrContent qrContent_ = QrContent::Address;
  WalletExportFormat exportFormat_ = WalletExportFormat::AuroraWallet;
  FileOperation fileOperation_ = FileOperation::None;
  bool loadedWallet_ = false;
  bool manualRestore_ = false;
  uint8_t restoreWordIndex_ = 0;
  uint8_t restoreSuggestionCount_ = 0;
  uint8_t verifyActiveIndex_ = 0;
  uint8_t verifySuggestionCount_ = 0;
  char restoreWords_[24][WalletEngine::BIP39_WORD_CAPACITY]{};
  char restoreSuggestions_[3][WalletEngine::BIP39_WORD_CAPACITY]{};
  char verifySuggestions_[3][WalletEngine::BIP39_WORD_CAPACITY]{};
  char restoreMnemonic_[256]{};
  char restoreStatus_[96]{};
  char exportBaseName_[25] = "aurora";
  char importBaseName_[25] = "aurora";
  char auroraFileOptions_[2048]{};
  uint16_t auroraFileCount_ = 0;
  char exportStatus_[80]{};
  char importStatus_[96]{};
  char passwordStatus_[80]{};
};
