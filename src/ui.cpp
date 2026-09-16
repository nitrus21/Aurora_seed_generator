#include "ui.h"
#include "hardware_rng.h"
#include "bitcoin_logo.h"
#include "sd_export.h"
#include "secure_memory.h"
#include "splash_img.h"
#include "version.h"
#include "platform/ui_layout.h"
#if defined(AURORA_BOARD_P4)
#include "sensors.h"
#endif

namespace {
AuroraUI *g_ui = nullptr;
constexpr lv_color_t ORANGE = AuroraLayout::color(0xF7, 0x93, 0x1A);
constexpr lv_color_t DANGER = AuroraLayout::color(0xFF, 0x3B, 0x30);
constexpr lv_color_t SUCCESS = AuroraLayout::color(0x39, 0xD3, 0x83);
constexpr lv_color_t BLACK = AuroraLayout::color(0x08, 0x09, 0x0B);
constexpr lv_color_t PANEL = AuroraLayout::color(0x16, 0x18, 0x1D);
constexpr lv_color_t MUTED = AuroraLayout::color(0x9A, 0xA0, 0xAA);
constexpr lv_color_t KEYBOARD_BG = AuroraLayout::color(0x0A, 0x0C, 0x10);
constexpr lv_color_t KEY_BG = AuroraLayout::color(0x1A, 0x1D, 0x23);
constexpr lv_color_t KEY_BORDER = AuroraLayout::color(0x2D, 0x32, 0x3B);
constexpr lv_color_t KEY_PRESSED = AuroraLayout::color(0x3A, 0x2A, 0x18);
constexpr lv_color_t KEY_TEXT = AuroraLayout::color(0xD8, 0xDC, 0xE2);
constexpr lv_color_t INPUT_BG = AuroraLayout::color(0xFF, 0xFF, 0xFF);
constexpr lv_color_t INPUT_BORDER = AuroraLayout::color(0xC7, 0xCC, 0xD4);
constexpr lv_color_t INPUT_PLACEHOLDER = AuroraLayout::color(0x5E, 0x64, 0x70);
constexpr lv_style_selector_t INPUT_FOCUSED =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) |
    static_cast<lv_style_selector_t>(LV_STATE_FOCUSED);
constexpr lv_style_selector_t KEY_PRESSED_SELECTOR =
    static_cast<lv_style_selector_t>(LV_PART_ITEMS) |
    static_cast<lv_style_selector_t>(LV_STATE_PRESSED);
constexpr lv_style_selector_t KEY_CHECKED_SELECTOR =
    static_cast<lv_style_selector_t>(LV_PART_ITEMS) |
    static_cast<lv_style_selector_t>(LV_STATE_CHECKED);
constexpr char PASSPHRASE_ASCII[] =
    " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
constexpr char FILE_NAME_CHARS[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";

const char *addressKindName(AddressKind kind) {
  switch (kind) {
    case AddressKind::Legacy: return "legacy-p2pkh";
    case AddressKind::NestedSegwit: return "nested-segwit-p2sh-p2wpkh";
    case AddressKind::NativeSegwit: return "native-segwit-p2wpkh";
    case AddressKind::Taproot: return "taproot-p2tr";
  }
  return "unknown";
}

enum Action : uint8_t { START, OPEN_WALLET, NEW_WALLET, RESTORE_WALLET, RECOVER_UMBREL,
                        TO_IMPORT_PASSWORD,
                        REFRESH_AURORA_FILES, UNLOCK_WALLET,
                        TO_PASSPHRASE, TO_ENTROPY, GENERATE,
                        NEXT_VERIFY, CHECK_VERIFY, TO_INFO, TO_QR_ADDRESS,
                        TO_QR_PUBLIC, REVEAL_PRIVATE, TO_BACKUP, EXPORT_ELECTRUM,
                        EXPORT_AURORA, CONFIRM_PRIVATE, SAVE_EXPORT,
                        SAVE_EXPORT_PASSWORD, DO_WIPE,
                        WORD_12 = 30, WORD_15, WORD_18, WORD_21, WORD_24,
                        TYPE_LEGACY = 40, TYPE_NESTED, TYPE_NATIVE, TYPE_TAPROOT,
                        BACK_MODE = 50, BACK_SETUP, BACK_PASSPHRASE, BACK_ENTROPY,
                        BACK_MNEMONIC, BACK_VERIFY, BACK_INFO, BACK_BACKUP,
                        BACK_IMPORT_NAME, BACK_EXPORT_NAME, BACK_MODE_WIPE,
                        MNEMONIC_PREVIOUS = 70, MNEMONIC_NEXT, RETRY_ENTROPY,
                        RESTORE_SETUP_CONTINUE = 80, RESTORE_WORD_CHANGED,
                        RESTORE_WORD_READY, RESTORE_SUGGESTION_0,
                        RESTORE_SUGGESTION_1, RESTORE_SUGGESTION_2,
                        RESTORE_DERIVE, RESTORE_DERIVATION_CHANGED,
                        RESTORE_WORD_BACK, BACK_RESTORE_SETUP,
                        BACK_RESTORE_WORDS, SHOW_LOADED_PASSPHRASE,
                        VERIFY_WORD_CHANGED = 100, VERIFY_SUGGESTION_0,
                        VERIFY_SUGGESTION_1, VERIFY_SUGGESTION_2,
                        LOCK_SESSION = 113,
                        SHOW_WORDS, RETRY_SD,
                        UMBREL_CONTINUE = 120, UMBREL_DECODE, UMBREL_SHOW_XPRV,
                        UMBREL_RESULT_BACK, PRIVATE_PASSWORD_CHECK, PRIVATE_PASSWORD_CANCEL };

void styleRoot(lv_obj_t *o) {
  lv_obj_set_style_bg_color(o, BLACK, 0); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(o, lv_color_white(), 0); lv_obj_set_style_border_width(o, 0, 0);
}

lv_obj_t *createInput(lv_obj_t *parent) {
  lv_obj_t *input = lv_textarea_create(parent);
  lv_obj_set_style_bg_color(input, INPUT_BG, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(input, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(input, INPUT_BORDER, LV_PART_MAIN);
  lv_obj_set_style_border_width(input, 1, LV_PART_MAIN);
  lv_obj_set_style_text_color(input, BLACK, LV_PART_MAIN);
  lv_obj_set_style_text_color(input, INPUT_PLACEHOLDER, LV_PART_TEXTAREA_PLACEHOLDER);
  lv_obj_set_style_border_color(input, ORANGE, INPUT_FOCUSED);
  lv_obj_set_style_border_width(input, 2, INPUT_FOCUSED);
  lv_obj_set_style_border_color(input, BLACK, LV_PART_CURSOR);
#if defined(AURORA_BOARD_P4)
  // One-line textareas otherwise shrink back to the theme's content height.
  // A 60-pixel minimum is 1.25x the former rendered height on the P4.
  lv_obj_set_style_min_height(input, 60, LV_PART_MAIN);
#endif
  return input;
}

lv_obj_t *createKeyboard(lv_obj_t *parent) {
  lv_obj_t *keyboard = lv_keyboard_create(parent);
#if defined(AURORA_BOARD_P4)
  // The CYD keeps the native LVGL 8 keyboard palette used in 1.7.5.
  // Only the P4 uses the custom dark keyboard.
  lv_obj_set_style_bg_color(keyboard, KEYBOARD_BG, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(keyboard, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(keyboard, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(keyboard, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(keyboard, KEY_BG, LV_PART_ITEMS);
  lv_obj_set_style_bg_opa(keyboard, LV_OPA_COVER, LV_PART_ITEMS);
  lv_obj_set_style_border_color(keyboard, KEY_BORDER, LV_PART_ITEMS);
  lv_obj_set_style_border_width(keyboard, 1, LV_PART_ITEMS);
  lv_obj_set_style_shadow_width(keyboard, 0, LV_PART_ITEMS);
  lv_obj_set_style_text_color(keyboard, KEY_TEXT, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(keyboard, KEY_PRESSED, KEY_PRESSED_SELECTOR);
  lv_obj_set_style_border_color(keyboard, KEY_PRESSED, KEY_PRESSED_SELECTOR);
  lv_obj_set_style_text_color(keyboard, KEY_TEXT, KEY_PRESSED_SELECTOR);
  lv_obj_set_style_bg_color(keyboard, KEY_PRESSED, KEY_CHECKED_SELECTOR);
  lv_obj_set_style_border_color(keyboard, KEY_PRESSED, KEY_CHECKED_SELECTOR);
  lv_obj_set_style_text_color(keyboard, KEY_TEXT, KEY_CHECKED_SELECTOR);
#endif
  return keyboard;
}


void wipeObjectTree(lv_obj_t *object) {
  if (!object) return;
  const uint32_t childCount = lv_obj_get_child_cnt(object);
  for (uint32_t i = 0; i < childCount; ++i) wipeObjectTree(lv_obj_get_child(object, i));

  if (lv_obj_check_type(object, &lv_label_class)) {
    char *text = const_cast<char *>(lv_label_get_text(object));
    if (text) secureZero(text, strlen(text));
  }
  if (lv_obj_check_type(object, &lv_textarea_class)) {
    char *text = const_cast<char *>(lv_textarea_get_text(object));
    if (text) secureZero(text, strlen(text));
  }
  if (lv_obj_has_class(object, &lv_canvas_class)) {
#if LVGL_VERSION_MAJOR >= 9
    lv_image_dsc_t *image = lv_canvas_get_image(object);
#else
    lv_img_dsc_t *image = lv_canvas_get_img(object);
#endif
    if (image && image->data && image->data_size) {
      secureZero(const_cast<uint8_t *>(image->data), image->data_size);
    }
  }
}
}

void AuroraUI::begin() {
  g_ui = this;
  show(Screen::Splash);
  selfTestPending_ = true;
  selfTestDueMs_ = millis() + 80;
}

void AuroraUI::tick() {
  // AEZEED is not an authenticated .aurora file view: retain its original
  // one-shot 15-second lifetime, without the file-view countdown.
  if(umbrelRecovery_ && screen_==Screen::UmbrelQr &&
      millis()-accessGrantedMs_>=SECRET_VISIBLE_MS) {
    closeSession(); return;
  }
  // Display time excludes decryption/rendering. It is absolute: neither touch
  // nor pagination renews it. Expiration closes the entire wallet.
  if(visibleSecret_!=Access::None &&
      millis()-visibleSecretStartedMs_>=accessDurationMs(visibleSecret_)) {
    closeSession(); return;
  }
  updateSecretCountdown();
  // Check before the sensor-shutdown interlock: a failed sensor stop must not
  // keep a partially entered/generated wallet or deferred operation alive.
  if (visibleSecret_==Access::None &&
      (sensitiveStateActive_ || protectedSession_ || entropyCollected_) &&
      lv_disp_get_inactive_time(nullptr) >= SESSION_IDLE_MS) {
    closeSession();
    return;
  }
 #if defined(AURORA_BOARD_P4)
  if (sensorStopPending_) {
    if (AuroraSensors::stopped()) {
      sensorStopPending_ = false;
      show(afterSensorStop_);
      if (afterSensorStop_ == Screen::Generating) generationDueMs_ = millis() + 100;
    } else if (millis() - sensorStopStarted_ > 5000 && entropyStatus_) {
      lv_label_set_text(entropyStatus_, "Arrêt capteurs non confirmé. Redémarrez.");
      lv_obj_set_style_text_color(entropyStatus_, DANGER, 0);
    }
    return; // Never show wallet secrets while a sensor can still be recording.
  }
  if (screen_ == Screen::Entropy && !entropyReadyPending_) {
    AuroraSensors::drain(entropy_);
    updatePortraitSensors();
  }
#endif
  if (protectedSession_ && access_ != Access::None && !authorized(access_)) {
    revokeAccess();
    wipeFileCredentials();
    fileOperation_ = FileOperation::None;
    fileOperationDueMs_ = 0;
    qrContent_ = QrContent::Address;
    show(umbrelRecovery_ ? Screen::UmbrelResult : Screen::Info);
    return;
  }
  if (selfTestPending_ && static_cast<int32_t>(millis() - selfTestDueMs_) >= 0) {
    selfTestPending_ = false;
    const uint32_t selfTestStarted = millis();
    selfTestResult_ = engine_.selfTest();
    if (selfTestResult_ == WalletSelfTest::Ok && !auroraWalletCryptoSelfTest()) {
      selfTestResult_ = WalletSelfTest::AuroraWalletCrypto;
    }
    Serial.printf("AURORA autotest : E%02u\n", static_cast<unsigned>(selfTestResult_));
    Serial.printf("AURORA autotest durée : %lu ms\n",
                  static_cast<unsigned long>(millis() - selfTestStarted));
    if (selfTestResult_ != WalletSelfTest::Ok) {
      show(Screen::SecurityError);
      return;
    }
  }
  if (screen_ == Screen::FileProcessing && fileOperationDueMs_ &&
      static_cast<int32_t>(millis() - fileOperationDueMs_) >= 0) {
    fileOperationDueMs_ = 0;
    const FileOperation operation = fileOperation_;
    fileOperation_ = FileOperation::None;
    const uint32_t started = millis();
    if (operation == FileOperation::Export) {
      performWalletExport();
      wipeFileCredentials();
      Serial.printf("AURORA export SD : %lu ms\n", static_cast<unsigned long>(millis() - started));
      revokeAccess();
      show(exportSucceeded_ && protectedSession_ ? Screen::Info : Screen::Backup);
    } else if (operation == FileOperation::Import) {
      const bool imported = performWalletImport();
      secureZero(filePassword_, sizeof(filePassword_));
      Serial.printf("AURORA lecture SD : %lu ms\n", static_cast<unsigned long>(millis() - started));
      if (imported) {
        show(Screen::Info);
      } else show(Screen::ImportPassword);
    }
    else if (operation == FileOperation::PrivateRead) {
      const uint32_t attemptStarted = started;
      const Screen destination = afterAuthentication_;
      const Access requested = requestedAccess_;
      const bool ok = loadPrivateWallet();
      if (lv_disp_get_inactive_time(nullptr) >= SESSION_IDLE_MS) {
        closeSession(); return;
      }
      if (ok) {
        access_ = requested;
        accessGrantedMs_ = (requested==Access::Words || requested==Access::PrivateQr)
            ? millis() : attemptStarted;
        if (!authorized(requested)) { revokeAccess(); show(Screen::Info); return; }
        show(destination);
        if(visibleSecret_==requested && fileSession_ && privateLoaded_) {
          // Only a fresh successful authentication starts a fresh display
          // grant. Navigation between pages/categories cannot renew it.
          accessGrantedMs_=visibleSecretStartedMs_=millis();
          updateSecretCountdown();
        }
      } else {
        access_ = Access::None; accessGrantedMs_ = 0;
        show(Screen::PrivatePassword);
      }
    }
    return;
  }
  if (entropyFailurePending_) {
    entropyFailurePending_ = false;
    show(Screen::GenerationError);
    return;
  }
  if (entropyReadyPending_ && screen_ == Screen::Entropy &&
      static_cast<int32_t>(millis() - entropyCompleteDueMs_) >= 0) {
    entropyReadyPending_ = false;
    show(Screen::Passphrase);
    return;
  }
  if (screen_ == Screen::Generating && generationDueMs_ &&
      static_cast<int32_t>(millis() - generationDueMs_) >= 0) {
    generationDueMs_ = 0;
    if (generate()) {
      mnemonicPage_ = 0;
      show(Screen::Mnemonic);
    } else {
      show(Screen::GenerationError);
    }
  }
  if (screen_ == Screen::Restoring && generationDueMs_ &&
      static_cast<int32_t>(millis() - generationDueMs_) >= 0) {
    generationDueMs_ = 0;
    if (restoreEnteredWallet()) {
      qrContent_ = QrContent::Address;
      protectedSession_=false;
      show(Screen::Info);
    } else {
      restoreWordIndex_ = words_ ? words_ - 1 : 0;
      show(Screen::RestoreWords);
    }
  }
  if (screen_ == Screen::UmbrelProcessing && generationDueMs_ &&
      static_cast<int32_t>(millis() - generationDueMs_) >= 0) {
    generationDueMs_ = 0;
    if (recoverUmbrel()) {
      protectedSession_=false;
      show(Screen::UmbrelResult);
    } else if (umbrelResult_ == AezeedResult::InvalidPassphrase ||
               umbrelResult_ == AezeedResult::MemoryFailed ||
               umbrelResult_ == AezeedResult::CryptoFailed) {
      show(Screen::UmbrelPassphrase);
    } else {
      restoreWordIndex_ = 23;
      show(Screen::RestoreWords);
    }
  }
}

void AuroraUI::clear() {
  if (root_) {
    wipeObjectTree(root_);
    lv_obj_del(root_);
  }
  root_ = lv_obj_create(lv_scr_act()); AuroraLayout::size(root_, 320, 240); AuroraLayout::pos(root_, 0, 0);
  lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE); styleRoot(root_);
  // Coordinates already include the screen margins; LVGL theme padding would
  // shift the CYD's rightmost controls beyond the physical 320-pixel display.
  lv_obj_set_style_pad_all(root_, 0, 0);
  // Suggestions belong only to the previous input screen. Each input builder
  // recomputes them, so retaining their word copies serves no navigation need.
  secureZero(restoreSuggestions_, sizeof(restoreSuggestions_));
  secureZero(verifySuggestions_, sizeof(verifySuggestions_));
  restoreSuggestionCount_ = verifySuggestionCount_ = 0;
#if defined(AURORA_BOARD_P4)
  lv_obj_set_style_radius(root_, 0, 0);
  // Service after the new screen is built, outside the current input event.
  // All replacements qualify, including password/PIN forms and secret QRs.
  displayRefreshPending_ = true;
  if (cameraPixels_) {
    secureZero(cameraPixels_, AuroraSensors::PREVIEW_WIDTH * AuroraSensors::PREVIEW_HEIGHT * sizeof(uint16_t));
    free(cameraPixels_); cameraPixels_ = nullptr;
  }
  microphoneStatus_ = microphoneLevel_ = cameraStatus_ = cameraPreview_ = nullptr;
  cameraImage_ = {}; cameraPreviewSequence_ = 0;
#endif
  secureZero(entropyPreviewText_, sizeof(entropyPreviewText_));
  passArea_ = entropyBar_ = entropyStatus_ = entropyPreview_ = entropyCount_ = keyboard_ = exportNameArea_ =
      importFileDropdown_ = restoreWordArea_ = restoreDerivationDropdown_ =
      filePasswordArea_ = filePasswordConfirmArea_ = nullptr;
  memset(verifyArea_, 0, sizeof(verifyArea_));
  memset(verifySuggestionButtons_, 0, sizeof(verifySuggestionButtons_));
  memset(restoreSuggestionButtons_, 0, sizeof(restoreSuggestionButtons_));
  passConfirmArea_ = securityStatus_ = nullptr;
  secretCountdown_ = nullptr;
  countdownSeconds_ = UINT32_MAX;
}

lv_obj_t *AuroraUI::label(lv_obj_t *p, const char *text, const lv_font_t *font) {
  lv_obj_t *l = lv_label_create(p); lv_label_set_text(l, text); AuroraLayout::font(l, font, 0);
  lv_obj_set_style_text_color(l, lv_color_white(), 0); return l;
}

lv_obj_t *AuroraUI::explanation(lv_obj_t *p, const char *text, const lv_font_t *font) {
  return label(p,text,font);
}

lv_obj_t *AuroraUI::button(lv_obj_t *p, const char *text, lv_event_cb_t cb, int w) {
  lv_obj_t *b = lv_btn_create(p);
#if defined(AURORA_BOARD_P4)
  // Standard P4 actions, especially bottom navigation, share the compact
  // 64-pixel height used by the main menu. Explicit special controls may
  // still override this size after creation.
  lv_obj_set_size(b, AuroraLayout::x(w), 64);
#else
  AuroraLayout::size(b, w, 34);
#endif
  lv_obj_set_style_radius(b, 8, 0);
  lv_obj_set_style_bg_color(b, ORANGE, 0); lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *l = label(b, text, &aurora_font_12); lv_obj_set_style_text_color(l, BLACK, 0); lv_obj_center(l);
  return b;
}

lv_obj_t *AuroraUI::header(const char *title, const char *step, bool showBrand) {
  lv_obj_t *titleLabel = nullptr;
  if (step) {
    lv_obj_t *badge = lv_obj_create(root_); AuroraLayout::pos(badge, 9, 3); AuroraLayout::size(badge, 27, 27);
    lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0); lv_obj_set_style_bg_color(badge, ORANGE, 0);
    lv_obj_set_style_border_width(badge, 0, 0); lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    char n[2] = {step[0], 0}; lv_obj_t *num = label(badge, n, &aurora_font_14);
    lv_obj_set_style_text_color(num, BLACK, 0); lv_obj_center(num);
    titleLabel = label(root_, title, &aurora_font_14); AuroraLayout::pos(titleLabel, 44, 9);
    lv_obj_t *s = label(root_, step, &aurora_font_10); lv_obj_set_style_text_color(s, MUTED, 0); AuroraLayout::pos(s, 247, 10);
    Action backAction = BACK_MODE;
    switch(screen_) {
      case Screen::Setup: backAction=BACK_MODE; break;
      case Screen::Passphrase: backAction=BACK_ENTROPY; break;
      case Screen::Entropy: backAction=BACK_SETUP; break;
      case Screen::Mnemonic: backAction=loadedWallet_ ? BACK_MODE_WIPE : BACK_ENTROPY; break;
      case Screen::PassphraseReveal: backAction=BACK_MNEMONIC; break;
      case Screen::Verify: backAction=BACK_MNEMONIC; break;
      case Screen::Info:
        backAction=(loadedWallet_ && !manualRestore_ && passphrase_[0]) ?
            SHOW_LOADED_PASSPHRASE : (loadedWallet_ ? BACK_MNEMONIC : BACK_VERIFY);
        break;
      case Screen::Backup: backAction=BACK_INFO; break;
      case Screen::ExportWarning: case Screen::ExportName: backAction=BACK_BACKUP; break;
      case Screen::ExportPassword: backAction=BACK_EXPORT_NAME; break;
      default: break;
    }
    lv_obj_t *back=lv_btn_create(root_); AuroraLayout::pos(back,276,3); AuroraLayout::size(back,28,28);
    lv_obj_set_style_radius(back,7,0); lv_obj_set_style_bg_color(back,PANEL,0);
    lv_obj_set_style_border_color(back,ORANGE,0); lv_obj_set_style_border_width(back,1,0);
    lv_obj_set_style_shadow_width(back,0,0); lv_obj_set_user_data(back,(void*)backAction);
    lv_obj_add_event_cb(back,event,LV_EVENT_CLICKED,nullptr);
    lv_obj_t *arrow=label(back,"<",&aurora_font_16); lv_obj_set_style_text_color(arrow,ORANGE,0); lv_obj_center(arrow);
  } else {
    if(showBrand) {
      lv_obj_t *brand = label(root_, "AURORA", &aurora_font_14); AuroraLayout::pos(brand, 10, 9);
      lv_obj_set_style_text_color(brand, ORANGE, 0);
    }
    titleLabel = label(root_, title, &aurora_font_14);
    if(showBrand) AuroraLayout::pos(titleLabel, 105, 9);
    else AuroraLayout::align(titleLabel, LV_ALIGN_TOP_MID, 0, 9);
  }
  lv_obj_t *line = lv_obj_create(root_); AuroraLayout::pos(line, 10, 34); AuroraLayout::size(line, 300, 1);
  lv_obj_set_style_bg_color(line, AuroraLayout::color(0x38,0x3B,0x42),0); lv_obj_set_style_border_width(line,0,0);
  return titleLabel;
}

void AuroraUI::show(Screen s) {
  // Navigation at the deadline must not revive an expired view before tick().
  if(s!=Screen::Splash && s!=Screen::Mode && s!=Screen::Wipe && s!=Screen::SecurityError &&
      visibleSecret_!=Access::None &&
      millis()-visibleSecretStartedMs_>=accessDurationMs(visibleSecret_)) s=Screen::Mode;
  if(umbrelRecovery_ && screen_==Screen::UmbrelQr && s!=Screen::UmbrelQr &&
      s!=Screen::SecurityError) s=Screen::Mode;
  // A blocking crypto/SD call may consume the idle deadline while tick() is
  // unable to run. Discard its result before constructing any next screen;
  // completing work is not user activity. Keep fatal security errors blocked.
  if (s != Screen::SecurityError && visibleSecret_==Access::None && sensitiveStateActive_ &&
      lv_disp_get_inactive_time(nullptr) >= SESSION_IDLE_MS) s = Screen::Mode;
  const bool clearedScreen = s == Screen::Splash || s == Screen::Mode ||
      s == Screen::Wipe || s == Screen::SecurityError;
  if (clearedScreen) {
    wipeSession();
#if defined(AURORA_BOARD_P4)
    // A close always supersedes a deferred destination containing secrets.
    // Keep input blocked until the sensor worker really acknowledges stop.
    if (sensorStopPending_) afterSensorStop_ = s;
#endif
  }
#if defined(AURORA_BOARD_P4)
  if (screen_ == Screen::Entropy && !AuroraSensors::stopped()) {
    if (!sensorStopPending_) {
      afterSensorStop_ = s; sensorStopPending_ = true; sensorStopStarted_ = millis();
      AuroraSensors::requestStop();
      entropy_.cancel();
      if (s != Screen::Generating && !((s == Screen::Passphrase || s == Screen::SdRequired) && entropyCollected_)) {
        secureZero(mixedEntropy_, sizeof(mixedEntropy_)); entropyCollected_ = false;
      }
      if (entropyStatus_) lv_label_set_text(entropyStatus_, "Arrêt des capteurs...");
    }
    // Erase the old text/pixels immediately on a close even if a sensor is
    // stuck. Safe screens do not expose secrets; their input remains blocked.
    if (!clearedScreen) return;
  }
#endif
  if (screen_ == Screen::Entropy && s != Screen::Entropy) {
    entropy_.cancel();
    entropyReadyPending_ = entropyFailurePending_ = false;
    entropyCompleteDueMs_ = 0;
    if (s != Screen::Generating && !((s == Screen::Passphrase || s == Screen::SdRequired) && entropyCollected_)) {
      secureZero(mixedEntropy_, sizeof(mixedEntropy_)); entropyCollected_ = false;
    }
  }
  if (s == Screen::Entropy) entropyCollected_ = false;
  // Require media only in the save/export workflow, after sensor shutdown.
  const bool sdChecked=needsSd(s);
  if(sdChecked && !ensureSd(s)) return;
  // Every file-backed private category is independently password-authorized.
  // A public screen retains only public metadata; cancellation never keeps a key.
  const bool authenticating = s==Screen::PrivatePassword ||
      (s==Screen::FileProcessing && fileOperation_==FileOperation::PrivateRead);
  if(s==Screen::PrivatePassword) {
    dropPrivateState();
    secureZero(filePassword_,sizeof(filePassword_));
    access_=Access::None; accessGrantedMs_=0;
  }
  if(fileSession_ && !clearedScreen && s!=Screen::SdRequired && !authenticating) {
    const Access required=accessFor(s);
    if(required!=Access::None && !authorized(required)) {
      if(s==Screen::FileProcessing)
        s=exportFormat_==WalletExportFormat::AuroraWallet?Screen::ExportPassword:Screen::ExportWarning;
      revokeAccess();
      secureZero(importStatus_,sizeof(importStatus_));
      fileOperation_=FileOperation::None; fileOperationDueMs_=0;
      afterAuthentication_=s; requestedAccess_=required;
      s=Screen::PrivatePassword;
    } else if(required==Access::None) {
      revokeAccess();
      fileOperation_=FileOperation::None; fileOperationDueMs_=0;
    }
  }
  if (!clearedScreen && !sensitiveStateActive_) {
    // Conservative by design: every workflow is sensitive from entry, before
    // its first character/sample. A future screen inherits expiry by default.
    sensitiveStateActive_ = true;
    lv_disp_trig_activity(nullptr);
  }
  if(umbrelRecovery_ && s==Screen::UmbrelQr && screen_!=s) accessGrantedMs_=millis();
  screen_ = s; clear();
  switch (s) {
    case Screen::Splash: buildSplash(); break; case Screen::Mode: buildMode(); break;
    case Screen::ImportName: buildImportName(); break;
    case Screen::ImportPassword: buildImportPassword(); break;
    case Screen::RestoreSetup: buildRestoreSetup(); break;
    case Screen::RestoreWords: buildRestoreWords(); break;
    case Screen::RestorePassphrase: buildRestorePassphrase(); break;
    case Screen::Restoring: buildRestoring(); break;
    case Screen::UmbrelWarning: buildUmbrelWarning(); break;
    case Screen::UmbrelPassphrase: buildUmbrelPassphrase(); break;
    case Screen::UmbrelProcessing: buildUmbrelProcessing(); break;
    case Screen::UmbrelResult: buildUmbrelResult(); break;
    case Screen::UmbrelQr: buildUmbrelQr(); break;
    case Screen::Setup: buildSetup(); break;
    case Screen::Passphrase: buildPassphrase(); break; case Screen::Entropy: buildEntropy(); break;
    case Screen::Generating: buildGenerating(); break;
    case Screen::FileProcessing: buildFileProcessing(); break;
    case Screen::GenerationError: buildGenerationError(); break;
    case Screen::SecurityError: buildSecurityError(); break;
    case Screen::Mnemonic: buildMnemonic(); break;
    case Screen::PassphraseReveal: buildPassphraseReveal(); break;
    case Screen::Verify: buildVerify(); break;
    case Screen::Info: buildInfo(); break; case Screen::Qr: buildQr(); break;
    case Screen::Backup: buildBackup(); break; case Screen::ExportWarning: buildExportWarning(); break;
    case Screen::ExportName: buildExportName(); break;
    case Screen::ExportPassword: buildExportPassword(); break;
    case Screen::Wipe: buildWipe(); break;
    case Screen::SdRequired: buildSdRequired(); break;
    case Screen::PrivatePassword: buildImportPassword(); break;
  }
  // Only reauthenticated .aurora consultations get the extended display
  // deadlines. Creation, manual restoration and AEZEED keep their own policy.
  const Access displayed = fileSession_ && protectedSession_ && privateLoaded_ ?
      (screen_==Screen::Mnemonic ? Access::Words :
       (screen_==Screen::Qr && qrContent_==QrContent::PrivateKey ? Access::PrivateQr : Access::None)) :
      Access::None;
  if(displayed!=visibleSecret_) {
    visibleSecret_=displayed;
    visibleSecretStartedMs_=displayed==Access::None ? 0 : millis();
    if(fileSession_ && displayed!=Access::None && access_==displayed)
      visibleSecretStartedMs_=accessGrantedMs_;
  }
  if(displayed!=Access::None) {
#if defined(AURORA_BOARD_P4)
    secretCountdown_=label(root_,"",&aurora_font_18);
    lv_obj_set_pos(secretCountdown_,15,696);
#else
    secretCountdown_=label(root_,"",&aurora_font_10);
    lv_obj_set_pos(secretCountdown_,10,227);
#endif
    lv_obj_set_style_text_color(secretCountdown_,MUTED,0);
    updateSecretCountdown();
  }
}

uint32_t AuroraUI::accessDurationMs(Access access) {
  switch(access) {
    case Access::Words: return WORDS_VISIBLE_MS;
    case Access::PrivateQr: return PRIVATE_KEY_VISIBLE_MS;
    case Access::Export: return EXPORT_AUTH_MS;
    default: return SECRET_VISIBLE_MS;
  }
}

void AuroraUI::updateSecretCountdown() {
  if(!secretCountdown_ || visibleSecret_==Access::None) return;
  const uint32_t duration=accessDurationMs(visibleSecret_);
  const uint32_t elapsed=millis()-visibleSecretStartedMs_;
  const uint32_t seconds=elapsed>=duration ? 0 : (duration-elapsed+999)/1000;
  if(seconds==countdownSeconds_) return;
  countdownSeconds_=seconds;
  // This buffer contains only public timing metadata, never wallet material.
  char text[8];
  snprintf(text,sizeof(text),"%02lu:%02lu",static_cast<unsigned long>(seconds/60),
           static_cast<unsigned long>(seconds%60));
  lv_label_set_text(secretCountdown_,text);
}

bool AuroraUI::needsSd(Screen screen) const {
  if(screen==Screen::ImportPassword || screen==Screen::PrivatePassword) return true;
  if(screen==Screen::FileProcessing && fileOperation_==FileOperation::PrivateRead) return true;
  if(fileSession_ && accessFor(screen)!=Access::None) return true;
  if(screen==Screen::Backup) return true;
  return accessFor(screen)==Access::Export;
}

bool AuroraUI::ensureSd(Screen resume) {
  // Shared submit handlers must not mount SD for an offline action.
  if(!needsSd(resume)) return true;
  if(auroraSdReady()) return true;
  // Interrupted operations must request their credentials again.
  // Never resume an operation with credentials erased by this failure.
  if(resume==Screen::PrivatePassword ||
     (resume==Screen::FileProcessing && fileOperation_==FileOperation::PrivateRead))
    resume=afterAuthentication_;
  if(resume==Screen::FileProcessing && fileOperation_==FileOperation::Export) {
    resume=exportFormat_==WalletExportFormat::AuroraWallet?Screen::ExportPassword:Screen::Backup;
  } else if(resume==Screen::FileProcessing) resume=Screen::ImportPassword;
  afterSd_=resume;
  wipeFileCredentials();
  fileOperation_=FileOperation::None; fileOperationDueMs_=0;
  revokeAccess(); // Also preserves the CYD's accumulated failure count.
  show(Screen::SdRequired); // clear() securely destroys typed UI text/copies.
  return false;
}

void AuroraUI::buildSdRequired() {
  lv_obj_set_style_pad_all(root_,0,0);
  header("microSD requise");
  lv_obj_t *title=label(root_,"CARTE ABSENTE OU ILLISIBLE",&aurora_font_14);
  lv_obj_set_style_text_color(title,DANGER,0); AuroraLayout::align(title,LV_ALIGN_TOP_MID,0,53);
  const char *hint="Insérez une carte microSD FAT32, puis appuyez sur RÉESSAYER.\nL'enregistrement sur la carte est bloqué.";
  lv_obj_t *message=explanation(root_,hint,&aurora_font_12);
  lv_label_set_long_mode(message,LV_LABEL_LONG_WRAP); AuroraLayout::pos(message,16,82);
  AuroraLayout::size(message,288,68); lv_obj_set_style_text_align(message,LV_TEXT_ALIGN_CENTER,0);
  lv_obj_t *note=explanation(root_,"FERMER efface la session en mémoire.",&aurora_font_10);
  lv_obj_set_style_text_color(note,MUTED,0); AuroraLayout::align(note,LV_ALIGN_TOP_MID,0,158);
  lv_obj_t *cancel=button(root_,"FERMER",event,136);
  lv_obj_set_user_data(cancel,(void*)LOCK_SESSION); AuroraLayout::pos(cancel,16,190);
  lv_obj_t *retry=button(root_,"RÉESSAYER",event,136);
  lv_obj_set_user_data(retry,(void*)RETRY_SD); AuroraLayout::pos(retry,168,190);
}

void AuroraUI::buildSplash() {
#if defined(AURORA_BOARD_P4)
  lv_obj_t *bg = lv_image_create(root_); lv_image_set_src(bg, &aurora_splash);
  lv_image_set_scale(bg, 384); lv_image_set_pivot(bg, 0, 0);
  lv_obj_set_pos(bg, 0, 280);
  lv_obj_t *name = label(root_, "A U R O R A", &aurora_font_72);
  lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 130);
  lv_obj_t *sub = label(root_, "SEED GENERATOR", &aurora_font_24);
  lv_obj_set_style_text_color(sub, ORANGE, 0); lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 204);
  lv_obj_t *b = button(root_, "CONTINUER", event, 320);
  lv_obj_set_size(b, 320, 64); lv_obj_set_user_data(b, (void *)START);
  lv_obj_align(b, LV_ALIGN_BOTTOM_MID, 0, -64);
  lv_obj_t *version = label(root_, "v" AURORA_FIRMWARE_VERSION, &aurora_font_10);
  lv_obj_set_style_text_color(version, MUTED, 0); lv_obj_align(version, LV_ALIGN_BOTTOM_MID, 0, -24);
#else
  lv_obj_t *bg = lv_img_create(root_); lv_img_set_src(bg, &aurora_splash); AuroraLayout::pos(bg, 0, 0);
  lv_obj_t *shade = lv_obj_create(root_); AuroraLayout::pos(shade, 0, 142); AuroraLayout::size(shade, 320, 98);
  lv_obj_set_style_bg_color(shade, BLACK, 0); lv_obj_set_style_bg_opa(shade, LV_OPA_50, 0);
  lv_obj_set_style_border_width(shade, 0, 0); lv_obj_clear_flag(shade, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *b = button(root_, "CONTINUER", event, 140); lv_obj_set_user_data(b, (void *)START);
  AuroraLayout::align(b, LV_ALIGN_BOTTOM_MID, 0, -51);
  lv_obj_t *name = label(root_, "A U R O R A", &aurora_font_20);
  AuroraLayout::align(name, LV_ALIGN_BOTTOM_MID, 0, -19);
  lv_obj_t *sub = label(root_, "SEED GENERATOR", &aurora_font_10);
  lv_obj_set_style_text_color(sub, ORANGE, 0); AuroraLayout::align(sub, LV_ALIGN_BOTTOM_MID, 0, -4);
  lv_obj_t *version = label(root_, "v" AURORA_FIRMWARE_VERSION, &aurora_font_10);
  lv_obj_set_style_text_color(version, lv_color_white(), 0);
  AuroraLayout::align(version, LV_ALIGN_BOTTOM_RIGHT, -4, -3);
#endif
}

void AuroraUI::buildMode() {
  header("Choisissez une action");
#if defined(AURORA_BOARD_P4)
  lv_obj_t *logo=lv_img_create(root_); lv_img_set_src(logo,&aurora_bitcoin_logo);
  // Keep the artwork entirely below the header divider (y=113..115 on P4).
  lv_obj_align(logo,LV_ALIGN_TOP_MID,0,136);
  const char *names[]={"NOUVEAU PORTEFEUILLE","OUVRIR AURORA WALLET",
                       "RESTAURER UNE SEED BIP39","RÉCUPÉRER UMBREL / LND"};
  const Action actions[]={NEW_WALLET,OPEN_WALLET,RESTORE_WALLET,RECOVER_UMBREL};
  for(unsigned i=0;i<4;++i) {
    lv_obj_t *choice=button(root_,names[i],event,256);
    lv_obj_set_size(choice,384,64);
    lv_obj_align(choice,LV_ALIGN_TOP_MID,0,316+i*88);
    lv_obj_set_user_data(choice,(void*)actions[i]);
  }
#else
  lv_obj_t *create=button(root_,"NOUVEAU PORTEFEUILLE",event,178);
  lv_obj_set_user_data(create,(void*)NEW_WALLET); AuroraLayout::pos(create,12,48);
  AuroraLayout::size(create,178,44);
  lv_obj_t *open=button(root_,"OUVRIR AURORA WALLET",event,178);
  lv_obj_set_user_data(open,(void*)OPEN_WALLET); AuroraLayout::pos(open,12,108);
  AuroraLayout::size(open,178,44);
  lv_obj_t *restore=button(root_,"RESTAURER UNE SEED",event,178);
  lv_obj_set_user_data(restore,(void*)RESTORE_WALLET); AuroraLayout::pos(restore,12,168);
  AuroraLayout::size(restore,178,44);
  lv_obj_t *logo=lv_img_create(root_); lv_img_set_src(logo,&aurora_bitcoin_logo);
  AuroraLayout::pos(logo,200,43);
#endif
}

void AuroraUI::buildImportName() {
  header("Ouvrir Aurora Wallet");
  lv_obj_t *back=button(root_,"<",event,28); AuroraLayout::size(back,28,28);
  lv_obj_set_user_data(back,(void*)BACK_MODE); AuroraLayout::pos(back,276,3);
  lv_obj_t *hint=explanation(root_,"Sélectionnez un fichier .aurora sur la carte microSD",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); AuroraLayout::pos(hint,12,41);
  const AuroraWalletListResult listResult=listAuroraWalletFiles(
      auroraFileOptions_,sizeof(auroraFileOptions_),auroraFileCount_);
  importFileDropdown_=lv_dropdown_create(root_); AuroraLayout::pos(importFileDropdown_,12,59);
  AuroraLayout::size(importFileDropdown_,296,40);
  AuroraLayout::font(importFileDropdown_,&aurora_font_12,0);
#if defined(AURORA_BOARD_P4)
  // Compact, centered selector instead of the former 444 x 133 surface.
  lv_obj_set_pos(importFileDropdown_,48,196); lv_obj_set_size(importFileDropdown_,384,64);
#endif
  lv_dropdown_set_symbol(importFileDropdown_,LV_SYMBOL_DOWN);
  if(auroraFileCount_) lv_dropdown_set_options(importFileDropdown_,auroraFileOptions_);
  else lv_dropdown_set_options(importFileDropdown_,"Aucun fichier .aurora");

  const char *statusText=nullptr; lv_color_t statusColor=MUTED;
  if(importStatus_[0]) { statusText=importStatus_; statusColor=DANGER; }
  else switch(listResult) {
    case AuroraWalletListResult::Ok: {
      static char countText[48];
      snprintf(countText,sizeof(countText),"%u fichier%s trouvé%s",
               static_cast<unsigned>(auroraFileCount_),
               auroraFileCount_>1?"s":"",auroraFileCount_>1?"s":"");
      statusText=countText; statusColor=ORANGE; break;
    }
    case AuroraWalletListResult::NoCard:
      statusText="Carte microSD absente ou illisible."; statusColor=DANGER; break;
    case AuroraWalletListResult::OpenFailed:
      statusText="Impossible de lire le contenu de la carte."; statusColor=DANGER; break;
    case AuroraWalletListResult::NoFiles:
      statusText="Aucun fichier .aurora trouvé à la racine."; statusColor=DANGER; break;
    case AuroraWalletListResult::BufferTooSmall:
      statusText="Trop de fichiers .aurora pour afficher la liste complète."; statusColor=DANGER; break;
  }
  lv_obj_t *status=explanation(root_,statusText?statusText:"",&aurora_font_10);
  lv_label_set_long_mode(status,LV_LABEL_LONG_WRAP); AuroraLayout::size(status,296,30);
  lv_obj_set_style_text_color(status,statusColor,0); AuroraLayout::pos(status,12,108);

  lv_obj_t *refresh=button(root_,"ACTUALISER",event,130);
  lv_obj_set_user_data(refresh,(void*)REFRESH_AURORA_FILES); AuroraLayout::pos(refresh,12,164);
  lv_obj_t *open=button(root_,"OUVRIR",event,130);
  lv_obj_set_user_data(open,(void*)TO_IMPORT_PASSWORD); AuroraLayout::pos(open,178,164);
  if(listResult!=AuroraWalletListResult::Ok || !auroraFileCount_) {
    lv_obj_add_state(open,LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(open,PANEL,LV_STATE_DISABLED);
  }
}

void AuroraUI::buildImportPassword() {
#if !defined(AURORA_BOARD_P4)
  lv_obj_set_style_pad_all(root_,0,0);
#endif
  lv_obj_t *title=header("Mot de passe Aurora Wallet");
#if !defined(AURORA_BOARD_P4)
  // CYD 1.7.5 used a compact title here to fit beside the brand/back button.
  lv_obj_set_style_text_font(title,&aurora_font_12,0);
#endif
  AuroraLayout::pos(title,90,10);
  lv_obj_t *back=button(root_,"<",event,28); AuroraLayout::size(back,28,28);
  const bool consulting=screen_==Screen::PrivatePassword;
  lv_obj_set_user_data(back,(void*)(consulting?PRIVATE_PASSWORD_CANCEL:BACK_IMPORT_NAME));
  AuroraLayout::pos(back,276,3);
  const char *message=importStatus_[0] ? importStatus_ :
      "Saisissez le mot de passe du fichier (12 caractères minimum).";
  lv_obj_t *hint=explanation(root_,message,&aurora_font_10);
  lv_label_set_long_mode(hint,LV_LABEL_LONG_WRAP); AuroraLayout::size(hint,296,26);
  lv_obj_set_style_text_color(hint,importStatus_[0]?DANGER:MUTED,0); AuroraLayout::pos(hint,12,40);
  filePasswordArea_=createInput(root_); AuroraLayout::pos(filePasswordArea_,12,69);
  AuroraLayout::size(filePasswordArea_,296,38); AuroraLayout::font(filePasswordArea_,&aurora_font_12,0);
  lv_textarea_set_one_line(filePasswordArea_,true); lv_textarea_set_password_mode(filePasswordArea_,true);
  lv_textarea_set_password_show_time(filePasswordArea_,0);
  lv_textarea_set_max_length(filePasswordArea_,63); lv_textarea_set_accepted_chars(filePasswordArea_,PASSPHRASE_ASCII);
  lv_textarea_set_placeholder_text(filePasswordArea_,"Mot de passe du fichier");
  keyboard_=createKeyboard(root_); AuroraLayout::size(keyboard_,320,112);
  AuroraLayout::align(keyboard_,LV_ALIGN_BOTTOM_MID,0,0); lv_keyboard_set_textarea(keyboard_,filePasswordArea_);
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_READY,
      (void*)(consulting?PRIVATE_PASSWORD_CHECK:UNLOCK_WALLET));
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_CANCEL,
      (void*)(consulting?PRIVATE_PASSWORD_CANCEL:BACK_IMPORT_NAME));
}

void AuroraUI::buildRestoreSetup() {
  header("Restaurer une seed");
  lv_obj_t *back=button(root_,"<",event,28); AuroraLayout::size(back,28,28);
  lv_obj_set_user_data(back,(void*)BACK_MODE); AuroraLayout::pos(back,276,3);
  lv_obj_t *hint=explanation(root_,"Choisissez le nombre de mots de la phrase BIP39.",&aurora_font_10);
  lv_obj_set_style_text_color(hint,ORANGE,0); AuroraLayout::pos(hint,12,43);
#if defined(AURORA_BOARD_P4)
  lv_obj_set_style_text_font(hint,&aurora_font_18,0);
#endif

  const uint8_t counts[5]={12,15,18,21,24};
  for(uint8_t i=0;i<5;++i) {
    lv_obj_t *choice=button(root_,"",event,54);
#if defined(AURORA_BOARD_P4)
    const int x=i<3 ? 72+i*120 : 132+(i-3)*120;
    const int y=i<3 ? 210 : 330;
    lv_obj_set_pos(choice,x,y); lv_obj_set_size(choice,96,96);
#else
    AuroraLayout::pos(choice,10+i*61,70); AuroraLayout::size(choice,54,40);
#endif
    lv_obj_set_user_data(choice,reinterpret_cast<void *>(static_cast<uintptr_t>(WORD_12+i)));
    const bool selected=words_==counts[i];
    lv_obj_set_style_bg_color(choice,selected?ORANGE:PANEL,0);
    lv_obj_set_style_border_color(choice,ORANGE,0);
    lv_obj_set_style_border_width(choice,selected?2:1,0);
    char text[3]; snprintf(text,sizeof(text),"%u",counts[i]);
    lv_obj_t *value=lv_obj_get_child(choice,0); lv_label_set_text(value,text);
#if defined(AURORA_BOARD_P4)
    lv_obj_set_style_text_font(value,&aurora_font_30,0);
#endif
    lv_obj_set_style_text_color(value,selected?BLACK:lv_color_white(),0);
  }

  lv_obj_t *info=explanation(root_,
      "Les mots sont vérifiés avec la liste anglaise officielle.\n"
      "Le checksum BIP39 sera contrôlé avant toute dérivation.",&aurora_font_10);
  lv_label_set_long_mode(info,LV_LABEL_LONG_WRAP); AuroraLayout::size(info,296,46);
  lv_obj_set_style_text_color(info,MUTED,0); AuroraLayout::pos(info,12,127);
#if defined(AURORA_BOARD_P4)
  lv_obj_set_pos(info,24,450); lv_obj_set_size(info,432,120);
#endif
  lv_obj_t *next=button(root_,"SAISIR LES MOTS",event,170);
  lv_obj_set_user_data(next,(void*)RESTORE_SETUP_CONTINUE);
  AuroraLayout::align(next,LV_ALIGN_BOTTOM_MID,0,-20);
}

void AuroraUI::updateRestoreSuggestions() {
  secureZero(restoreSuggestions_,sizeof(restoreSuggestions_));
  const char *prefix=restoreWordArea_?lv_textarea_get_text(restoreWordArea_):nullptr;
  restoreSuggestionCount_=WalletEngine::bip39Suggestions(
      prefix,reinterpret_cast<char *>(restoreSuggestions_),
      WalletEngine::BIP39_WORD_CAPACITY,3);
  for(uint8_t i=0;i<3;++i) {
    if(!restoreSuggestionButtons_[i]) continue;
    if(i<restoreSuggestionCount_) {
      lv_obj_t *text=lv_obj_get_child(restoreSuggestionButtons_[i],0);
      lv_label_set_text(text,restoreSuggestions_[i]);
      lv_obj_clear_flag(restoreSuggestionButtons_[i],LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(restoreSuggestionButtons_[i],LV_OBJ_FLAG_HIDDEN);
    }
  }
}

void AuroraUI::buildRestoreWords() {
  char title[40]; snprintf(title,sizeof(title),umbrelRecovery_?"Mot AEZEED %u / %u":"Mot BIP39 %u / %u",
                           static_cast<unsigned>(restoreWordIndex_+1),
                           static_cast<unsigned>(words_));
  header(title);
  lv_obj_t *back=button(root_,"<",event,28); AuroraLayout::size(back,28,28);
  lv_obj_set_user_data(back,(void*)(restoreWordIndex_?RESTORE_WORD_BACK:
                                   (umbrelRecovery_?UMBREL_RESULT_BACK:BACK_RESTORE_SETUP)));
  AuroraLayout::pos(back,276,3);

  const int16_t contentOffset=restoreStatus_[0]?14:0;
  if(restoreStatus_[0]) {
    lv_obj_t *hint=explanation(root_,restoreStatus_,&aurora_font_10);
    lv_label_set_long_mode(hint,LV_LABEL_LONG_WRAP); AuroraLayout::size(hint,296,14);
    lv_obj_set_style_text_color(hint,DANGER,0); AuroraLayout::pos(hint,12,38);
  }

  restoreWordArea_=createInput(root_); AuroraLayout::pos(restoreWordArea_,12,40+contentOffset);
  AuroraLayout::size(restoreWordArea_,296,36); AuroraLayout::font(restoreWordArea_,&aurora_font_14,0);
  lv_textarea_set_one_line(restoreWordArea_,true);
  lv_textarea_set_max_length(restoreWordArea_,WalletEngine::BIP39_WORD_CAPACITY-1);
  lv_textarea_set_accepted_chars(restoreWordArea_,"abcdefghijklmnopqrstuvwxyz");
  lv_textarea_set_placeholder_text(restoreWordArea_,"Mot anglais BIP39");
  lv_obj_set_user_data(restoreWordArea_,(void*)RESTORE_WORD_CHANGED);
  lv_obj_add_event_cb(restoreWordArea_,event,LV_EVENT_VALUE_CHANGED,nullptr);
  if(restoreWords_[restoreWordIndex_][0])
    lv_textarea_set_text(restoreWordArea_,restoreWords_[restoreWordIndex_]);

  for(uint8_t i=0;i<3;++i) {
    restoreSuggestionButtons_[i]=button(root_,"",event,94);
    AuroraLayout::size(restoreSuggestionButtons_[i],94,26);
    AuroraLayout::pos(restoreSuggestionButtons_[i],8+i*104,80+contentOffset);
    lv_obj_set_user_data(restoreSuggestionButtons_[i],
                         reinterpret_cast<void *>(static_cast<uintptr_t>(RESTORE_SUGGESTION_0+i)));
  }

  updateRestoreSuggestions();

  keyboard_=createKeyboard(root_); AuroraLayout::size(keyboard_,320,108);
  AuroraLayout::align(keyboard_,LV_ALIGN_BOTTOM_MID,0,0);
  lv_keyboard_set_mode(keyboard_,LV_KEYBOARD_MODE_TEXT_LOWER);
  lv_keyboard_set_textarea(keyboard_,restoreWordArea_);
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_READY,(void*)RESTORE_WORD_READY);
}

bool AuroraUI::acceptRestoreWord(const char *word) {
  if(!ensureSd(Screen::RestoreWords)) return false;
  const char *accepted=word;
  if(!accepted || !WalletEngine::bip39Word(accepted)) {
    char suggestions[3][WalletEngine::BIP39_WORD_CAPACITY]{};
    const uint8_t count=WalletEngine::bip39Suggestions(
        accepted,reinterpret_cast<char *>(suggestions),
        WalletEngine::BIP39_WORD_CAPACITY,3);
    if(count==1) accepted=suggestions[0];
    else {
      strlcpy(restoreStatus_,"Mot incomplet : choisissez une suggestion BIP39.",sizeof(restoreStatus_));
      secureZero(suggestions,sizeof(suggestions));
      show(Screen::RestoreWords);
      return false;
    }
    strlcpy(restoreWords_[restoreWordIndex_],accepted,
            sizeof(restoreWords_[restoreWordIndex_]));
    secureZero(suggestions,sizeof(suggestions));
  } else {
    strlcpy(restoreWords_[restoreWordIndex_],accepted,
            sizeof(restoreWords_[restoreWordIndex_]));
  }
  secureZero(restoreStatus_,sizeof(restoreStatus_));

  if(restoreWordIndex_+1<words_) {
    ++restoreWordIndex_;
    show(Screen::RestoreWords);
    return true;
  }

  secureZero(restoreMnemonic_,sizeof(restoreMnemonic_));
  for(uint8_t i=0;i<words_;++i) {
    if(i && strlcat(restoreMnemonic_," ",sizeof(restoreMnemonic_))>=sizeof(restoreMnemonic_)) return false;
    if(strlcat(restoreMnemonic_,restoreWords_[i],sizeof(restoreMnemonic_))>=sizeof(restoreMnemonic_)) return false;
  }
  show(umbrelRecovery_ ? Screen::UmbrelPassphrase : Screen::RestorePassphrase);
  return true;
}

void AuroraUI::buildRestorePassphrase() {
  buildPassphraseFields(true);
}

void AuroraUI::buildRestoring() {
  lv_obj_t *spinner=AuroraLayout::spinner(root_,900,70); AuroraLayout::size(spinner,58,58);
  lv_obj_set_style_arc_color(spinner,PANEL,LV_PART_MAIN);
  lv_obj_set_style_arc_color(spinner,ORANGE,LV_PART_INDICATOR);
  AuroraLayout::align(spinner,LV_ALIGN_CENTER,0,-40);
  lv_obj_t *title=label(root_,"Restauration du portefeuille...",&aurora_font_16);
  AuroraLayout::align(title,LV_ALIGN_CENTER,0,15);
  lv_obj_t *hint=explanation(root_,"Validation BIP39 et dérivation BIP32 en cours.",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); AuroraLayout::align(hint,LV_ALIGN_CENTER,0,48);
}

bool AuroraUI::restoreEnteredWallet() {
  const bool ok=engine_.restore(restoreMnemonic_,words_,kind_,passphrase_,wallet_);
  if(!ok) {
    strlcpy(restoreStatus_,
            "Phrase ou checksum BIP39 invalide. Vérifiez les mots.",
            sizeof(restoreStatus_));
    return false;
  }
  loadedWallet_=true;
  manualRestore_=true;
  secureZero(restoreWords_,sizeof(restoreWords_));
  secureZero(restoreMnemonic_,sizeof(restoreMnemonic_));
#if defined(AURORA_BOARD_P4)
  secureZero(restoreSuggestions_,sizeof(restoreSuggestions_));
  restoreSuggestionCount_=0;
#endif
  secureZero(restoreStatus_,sizeof(restoreStatus_));
  return true;
}

bool AuroraUI::rederiveManualWallet(AddressKind kind) {
  if(!manualRestore_ || !wallet_.valid) return false;
  char mnemonic[sizeof(wallet_.mnemonic)]{};
  strlcpy(mnemonic,wallet_.mnemonic,sizeof(mnemonic));
  const bool ok=engine_.restore(mnemonic,words_,kind,passphrase_,wallet_);
  secureZero(mnemonic,sizeof(mnemonic));
  if(ok) kind_=kind;
  return ok;
}

void AuroraUI::buildUmbrelWarning() {
  header("Récupération Umbrel / LND");
  lv_obj_t *back=button(root_,"<",event,28); AuroraLayout::size(back,28,28);
  lv_obj_set_user_data(back,(void*)BACK_MODE); AuroraLayout::pos(back,276,3);
  lv_obj_t *danger=label(root_,"FONDS ON-CHAIN UNIQUEMENT",&aurora_font_16);
  lv_obj_set_style_text_color(danger,DANGER,0); AuroraLayout::align(danger,LV_ALIGN_TOP_MID,0,48);
  lv_obj_t *message=explanation(root_,
      "Cette opération déchiffre les 24 mots AEZEED de LND et produit la clé maître BIP32 (xprv) pour Sparrow.\n\n"
      "Elle ne restaure pas les canaux Lightning. Pour eux, utilisez aussi le fichier channel.backup / SCB.",
      &aurora_font_10);
  lv_label_set_long_mode(message,LV_LABEL_LONG_WRAP); AuroraLayout::size(message,292,102);
  lv_obj_set_style_text_align(message,LV_TEXT_ALIGN_CENTER,0); AuroraLayout::pos(message,14,76);
  lv_obj_t *next=button(root_,"SAISIR LES 24 MOTS",event,190);
  lv_obj_set_user_data(next,(void*)UMBREL_CONTINUE); AuroraLayout::align(next,LV_ALIGN_BOTTOM_MID,0,-18);
}

void AuroraUI::buildUmbrelPassphrase() {
  lv_obj_set_style_pad_all(root_,0,0); header("Passphrase AEZEED");
  lv_obj_t *back=button(root_,"<",event,28); AuroraLayout::size(back,28,28);
  lv_obj_set_user_data(back,(void*)BACK_RESTORE_WORDS); AuroraLayout::pos(back,276,3);
  const char *message=restoreStatus_[0]?restoreStatus_:
      "Optionnelle. Laissez vide si aucune passphrase AEZEED n'a été définie dans LND.";
  securityStatus_=explanation(root_,message,&aurora_font_10);
  AuroraLayout::pos(securityStatus_,12,39); AuroraLayout::size(securityStatus_,296,30);
  lv_label_set_long_mode(securityStatus_,LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(securityStatus_,restoreStatus_[0]?DANGER:ORANGE,0);
  passArea_=createInput(root_); AuroraLayout::pos(passArea_,12,71);
  AuroraLayout::size(passArea_,296,36); AuroraLayout::font(passArea_,&aurora_font_12,0);
  lv_textarea_set_one_line(passArea_,true); lv_textarea_set_password_mode(passArea_,true);
  lv_textarea_set_password_show_time(passArea_,0); lv_textarea_set_max_length(passArea_,63);
  lv_textarea_set_accepted_chars(passArea_,PASSPHRASE_ASCII);
  lv_textarea_set_placeholder_text(passArea_,"Passphrase AEZEED (vide par défaut)");
  keyboard_=createKeyboard(root_); AuroraLayout::size(keyboard_,320,112);
  AuroraLayout::align(keyboard_,LV_ALIGN_BOTTOM_MID,0,0); lv_keyboard_set_textarea(keyboard_,passArea_);
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_READY,(void*)UMBREL_DECODE);
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_CANCEL,(void*)BACK_RESTORE_WORDS);
}

void AuroraUI::buildUmbrelProcessing() {
  lv_obj_t *spinner=AuroraLayout::spinner(root_,900,70); AuroraLayout::size(spinner,58,58);
  lv_obj_set_style_arc_color(spinner,PANEL,LV_PART_MAIN);
  lv_obj_set_style_arc_color(spinner,ORANGE,LV_PART_INDICATOR);
  AuroraLayout::align(spinner,LV_ALIGN_CENTER,0,-42);
  lv_obj_t *title=label(root_,"Déchiffrement AEZEED...",&aurora_font_16);
  AuroraLayout::align(title,LV_ALIGN_CENTER,0,14);
  lv_obj_t *hint=explanation(root_,"Paramètres scrypt LND officiels. Patientez...",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); AuroraLayout::align(hint,LV_ALIGN_CENTER,0,47);
}

bool AuroraUI::recoverUmbrel() {
  AezeedDecoded decoded{};
  umbrelResult_=AezeedEngine::decode(restoreMnemonic_,passphrase_,decoded);
  secureZero(passphrase_,sizeof(passphrase_));
  if(umbrelResult_!=AezeedResult::Ok) {
    const char *message="Erreur cryptographique AEZEED.";
    switch(umbrelResult_) {
      case AezeedResult::InvalidFormat: message="La phrase doit contenir exactement 24 mots."; break;
      case AezeedResult::UnknownWord: message="Un mot n'appartient pas à la liste AEZEED anglaise."; break;
      case AezeedResult::UnsupportedVersion: message="Version AEZEED non prise en charge."; break;
      case AezeedResult::InvalidChecksum: message="Checksum AEZEED invalide. Vérifiez les 24 mots."; break;
      case AezeedResult::InvalidPassphrase: message="Passphrase AEZEED incorrecte."; break;
      case AezeedResult::MemoryFailed: message="Mémoire PSRAM insuffisante pour le déchiffrement."; break;
      default: break;
    }
    strlcpy(restoreStatus_,message,sizeof(restoreStatus_));
    AezeedEngine::wipe(decoded); secureZero(umbrelRootXprv_,sizeof(umbrelRootXprv_));
    return false;
  }
  const bool ok=engine_.rootXprvFromSeed(decoded.entropy,sizeof(decoded.entropy),
                                         umbrelRootXprv_,sizeof(umbrelRootXprv_));
  umbrelBirthdayDays_=decoded.birthdayDays; AezeedEngine::wipe(decoded);
  if(!ok) {
    umbrelResult_=AezeedResult::CryptoFailed;
    strlcpy(restoreStatus_,"Impossible de dériver la clé maître BIP32.",sizeof(restoreStatus_));
    secureZero(umbrelRootXprv_,sizeof(umbrelRootXprv_)); return false;
  }
  secureZero(restoreWords_,sizeof(restoreWords_)); secureZero(restoreMnemonic_,sizeof(restoreMnemonic_));
  secureZero(restoreSuggestions_,sizeof(restoreSuggestions_)); secureZero(restoreStatus_,sizeof(restoreStatus_));
  return true;
}

void AuroraUI::buildUmbrelResult() {
  header("Clé Umbrel prête");
  lv_obj_t *panel=lv_obj_create(root_); AuroraLayout::pos(panel,12,43); AuroraLayout::size(panel,296,126);
  lv_obj_set_style_bg_color(panel,PANEL,0); lv_obj_set_style_border_color(panel,ORANGE,0);
  lv_obj_set_style_border_width(panel,1,0); lv_obj_set_style_radius(panel,7,0);
  lv_obj_clear_flag(panel,LV_OBJ_FLAG_SCROLLABLE);
  char text[360];snprintf(text,sizeof(text),
      "AEZEED authentifiée\nAnniversaire LND : jour %u depuis Genesis\n\n"
      "Dans Sparrow : nouveau portefeuille > Software Wallet > Master Private Key (BIP32).\n"
      "Recherchez ensuite les comptes 49', 84' et 86'.",
      static_cast<unsigned>(umbrelBirthdayDays_));
  lv_obj_t *info=explanation(panel,text,&aurora_font_10);lv_label_set_long_mode(info,LV_LABEL_LONG_WRAP);
  AuroraLayout::size(info,282,116);AuroraLayout::pos(info,4,3);
  lv_obj_t *show=button(root_,"AFFICHER LE XPRV",event,160);
  lv_obj_set_user_data(show,(void*)UMBREL_SHOW_XPRV);AuroraLayout::pos(show,12,185);
  lv_obj_set_style_bg_color(show,DANGER,0);
  lv_obj_t *lock=button(root_,"EFFACER",event,116);
  lv_obj_set_user_data(lock,(void*)LOCK_SESSION);AuroraLayout::pos(lock,192,185);
}

void AuroraUI::buildUmbrelQr() {
  lv_obj_t *title=header("XPRV maître - DANGER");lv_obj_set_style_text_color(title,DANGER,0);
#if defined(AURORA_BOARD_P4)
  if(!renderQr(root_,umbrelRootXprv_,224,48,39)) {
    lv_obj_t *error=label(root_,"QR impossible",&aurora_font_20);
    lv_obj_set_style_text_color(error,DANGER,0);lv_obj_set_pos(error,24,260);
  }
  lv_obj_t *value=label(root_,umbrelRootXprv_,&aurora_font_20);
  lv_label_set_long_mode(value,LV_LABEL_LONG_WRAP);lv_obj_set_size(value,432,180);lv_obj_set_pos(value,24,480);
  lv_obj_t *warning=explanation(root_,"Quiconque possède ce xprv peut dépenser tous les fonds.",&aurora_font_10);
  lv_label_set_long_mode(warning,LV_LABEL_LONG_WRAP);lv_obj_set_size(warning,432,46);
  lv_obj_set_style_text_color(warning,DANGER,0);lv_obj_set_pos(warning,24,665);
  lv_obj_t *back=button(root_,"RETOUR",event,116);lv_obj_set_user_data(back,(void*)UMBREL_RESULT_BACK);
  lv_obj_set_pos(back,153,720);lv_obj_set_size(back,174,64);
#else
  if(!renderQr(root_,umbrelRootXprv_,128,6,48)) {
    lv_obj_t *error=label(root_,"QR impossible",&aurora_font_14);
    lv_obj_set_style_text_color(error,DANGER,0);AuroraLayout::pos(error,18,94);
  }
  lv_obj_t *value=label(root_,umbrelRootXprv_,&aurora_font_10);
  lv_label_set_long_mode(value,LV_LABEL_LONG_WRAP);AuroraLayout::size(value,168,112);AuroraLayout::pos(value,142,49);
  lv_obj_t *warning=explanation(root_,"Quiconque possède ce xprv peut dépenser tous les fonds.",&aurora_font_10);
  lv_label_set_long_mode(warning,LV_LABEL_LONG_WRAP);AuroraLayout::size(warning,168,34);
  lv_obj_set_style_text_color(warning,DANGER,0);AuroraLayout::pos(warning,142,151);
  lv_obj_t *back=button(root_,"RETOUR",event,116);lv_obj_set_user_data(back,(void*)UMBREL_RESULT_BACK);
  AuroraLayout::pos(back,192,194);
#endif
}

void AuroraUI::buildSetup() {
  header("Configuration du portefeuille", "1 / 7");
#if defined(AURORA_BOARD_P4)
  const lv_font_t *sectionFont=&aurora_font_12, *countFont=&aurora_font_20, *typeFont=&aurora_font_14;
#else
  // Original 320x240 typography (1.7.5); P4 enlargement is not shared.
  const lv_font_t *sectionFont=&aurora_font_10, *countFont=&aurora_font_10, *typeFont=&aurora_font_10;
#endif
  lv_obj_t *l1 = label(root_, "Nombre de mots", sectionFont); AuroraLayout::pos(l1, 10, 41);
  const uint8_t counts[5] = {12,15,18,21,24};
  for (int i=0;i<5;++i) {
    lv_obj_t *b=lv_btn_create(root_); AuroraLayout::pos(b,10+i*61,56); AuroraLayout::size(b,55,27);
    bool selected=words_==counts[i]; lv_obj_set_style_radius(b,6,0);
    lv_obj_set_style_bg_color(b,selected?ORANGE:PANEL,0); lv_obj_set_style_border_color(b,ORANGE,0);
    lv_obj_set_style_border_width(b,selected?1:0,0); lv_obj_set_style_shadow_width(b,0,0);
    lv_obj_set_user_data(b,reinterpret_cast<void *>(static_cast<uintptr_t>(WORD_12+i))); lv_obj_add_event_cb(b,event,LV_EVENT_CLICKED,nullptr);
    char text[3]; snprintf(text,sizeof(text),"%u",counts[i]); lv_obj_t *v=label(b,text,countFont);
    lv_obj_set_style_text_color(v,selected?BLACK:lv_color_white(),0); lv_obj_center(v);
  }
  lv_obj_t *l2 = label(root_, "Type d'adresse", sectionFont); AuroraLayout::pos(l2, 10, 86);
  const char *names[4]={"Legacy\nm/44'/0'/0'/0/0","Nested SegWit\nm/49'/0'/0'/0/0","Native SegWit\nm/84'/0'/0'/0/0","Taproot\nm/86'/0'/0'/0/0"};
  for(int i=0;i<4;++i){
    lv_obj_t *b=lv_btn_create(root_); int x=10+(i%2)*155, y=99+(i/2)*36; AuroraLayout::pos(b,x,y); AuroraLayout::size(b,145,32);
    bool selected=(uint8_t)kind_==i; lv_obj_set_style_radius(b,6,0); lv_obj_set_style_bg_color(b,PANEL,0);
    lv_obj_set_style_border_color(b,ORANGE,0); lv_obj_set_style_border_width(b,selected?2:1,0); lv_obj_set_style_shadow_width(b,0,0);
    lv_obj_set_user_data(b,reinterpret_cast<void *>(static_cast<uintptr_t>(TYPE_LEGACY+i))); lv_obj_add_event_cb(b,event,LV_EVENT_CLICKED,nullptr);
    lv_obj_t *v=label(b,names[i],typeFont); lv_obj_set_style_text_color(v,selected?ORANGE:lv_color_white(),0); lv_obj_center(v);
  }
  lv_obj_t *safe=label(root_,"Bitcoin Mainnet • hors ligne",&aurora_font_10); lv_obj_set_style_text_color(safe,MUTED,0); AuroraLayout::pos(safe,10,181);
  lv_obj_t *b = button(root_, "CONTINUER", event, 105); lv_obj_set_user_data(b,(void*)TO_PASSPHRASE); AuroraLayout::pos(b,205,198);
}

void AuroraUI::buildPassphrase() {
  buildPassphraseFields(false);
}

void AuroraUI::buildPassphraseFields(bool restoring) {
  lv_obj_set_style_pad_all(root_,0,0);
  header("Passphrase BIP39", restoring ? nullptr : "3 / 7");
  if (restoring) {
    lv_obj_t *back=button(root_,"<",event,28); AuroraLayout::size(back,28,28);
    lv_obj_set_user_data(back,(void*)BACK_RESTORE_WORDS); AuroraLayout::pos(back,276,3);
  }
  securityStatus_=explanation(root_,"Optionnelle : saisissez-la deux fois, ou laissez les deux vides.",&aurora_font_10);
  AuroraLayout::pos(securityStatus_,12,39); AuroraLayout::size(securityStatus_,296,28);
  lv_label_set_long_mode(securityStatus_,LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(securityStatus_,ORANGE,0);
  lv_obj_t **fields[2]={&passArea_,&passConfirmArea_};
  for(unsigned i=0;i<2;++i) {
    lv_obj_t *field=*fields[i]=createInput(root_);
    AuroraLayout::pos(field,i?165:12,70); AuroraLayout::size(field,143,35);
    AuroraLayout::font(field,&aurora_font_10,0);
#if !defined(AURORA_BOARD_P4)
    lv_obj_set_style_text_font(field,&aurora_font_12,0);
#endif
    lv_textarea_set_one_line(field,true); lv_textarea_set_password_mode(field,true);
    lv_textarea_set_password_show_time(field,0);
    lv_textarea_set_max_length(field,63); lv_textarea_set_accepted_chars(field,PASSPHRASE_ASCII);
    lv_textarea_set_placeholder_text(field,i?"Confirmation":"Passphrase");
    lv_obj_add_event_cb(field,event,LV_EVENT_FOCUSED,nullptr);
  }
  keyboard_=createKeyboard(root_); AuroraLayout::size(keyboard_,320,112);
  AuroraLayout::align(keyboard_,LV_ALIGN_BOTTOM_MID,0,0); lv_keyboard_set_textarea(keyboard_,passArea_);
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_READY,(void*)(restoring?RESTORE_DERIVE:TO_ENTROPY));
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_CANCEL,(void*)(restoring?BACK_RESTORE_WORDS:BACK_ENTROPY));
}

bool AuroraUI::confirmPassphrase() {
  if(!passArea_ || !passConfirmArea_) return false;
  if(!ensureSd(screen_)) return false;
  const char *first=lv_textarea_get_text(passArea_);
  const char *second=lv_textarea_get_text(passConfirmArea_);
  const bool ok=first && second && strlen(first)<sizeof(passphrase_) && strcmp(first,second)==0;
  secureZero(passphrase_,sizeof(passphrase_));
  if(ok) strlcpy(passphrase_,first,sizeof(passphrase_));
  wipeObjectTree(passArea_); wipeObjectTree(passConfirmArea_);
  lv_textarea_set_text(passArea_,""); lv_textarea_set_text(passConfirmArea_,"");
  if(!ok) {
    lv_label_set_text(securityStatus_,"Les deux passphrases sont différentes. Recommencez.");
    lv_obj_set_style_text_color(securityStatus_,DANGER,0);
    lv_keyboard_set_textarea(keyboard_,passArea_);
  }
  return ok;
}

void AuroraUI::buildEntropy() {
#if defined(AURORA_BOARD_P4)
  buildPortraitEntropy();
#else
  lv_obj_set_style_pad_all(root_,0,0);
  header("Collecte d'entropie", "2 / 7");
  secureZero(mixedEntropy_, sizeof(mixedEntropy_));
  entropyReadyPending_ = entropyFailurePending_ = false;
  entropyCompleteDueMs_ = entropyPreviewUpdatedMs_ = 0;
  entropy_.begin();

  lv_obj_t *title = explanation(root_, "Bougez votre doigt dans le cadre", &aurora_font_14);
  AuroraLayout::pos(title,18,43);
  lv_obj_t *pad = lv_obj_create(root_); AuroraLayout::pos(pad,18,62); AuroraLayout::size(pad,284,72);
  lv_obj_clear_flag(pad,LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(pad,PANEL,0); lv_obj_set_style_border_color(pad,ORANGE,0);
  lv_obj_set_style_border_width(pad,1,0); lv_obj_set_style_radius(pad,8,0);
  lv_obj_t *hint = explanation(pad,"Mouvement + pression + temps\nLuminosité + RNG matériel",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_CENTER,0); lv_obj_center(hint);

  lv_obj_t *caption = explanation(root_,"Aperçu du mélange",&aurora_font_10);
  lv_obj_set_style_text_color(caption,MUTED,0); AuroraLayout::pos(caption,18,140);
  lv_obj_t *strip = lv_obj_create(root_); AuroraLayout::pos(strip,18,155); AuroraLayout::size(strip,284,26);
  lv_obj_clear_flag(strip,LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(strip,PANEL,0); lv_obj_set_style_border_width(strip,0,0);
  lv_obj_set_style_radius(strip,4,0); lv_obj_set_style_pad_all(strip,4,0);
  strlcpy(entropyPreviewText_,"-------- -------- -------- --------",sizeof(entropyPreviewText_));
  entropyPreview_ = label(strip,"",&aurora_font_12);
  lv_label_set_text_static(entropyPreview_,entropyPreviewText_); lv_obj_center(entropyPreview_);

  entropyStatus_ = label(root_,"Collecte insuffisante - 0 %",&aurora_font_10);
  AuroraLayout::pos(entropyStatus_,18,187); lv_obj_set_style_text_color(entropyStatus_,DANGER,0);
  entropyBar_ = lv_bar_create(root_); AuroraLayout::pos(entropyBar_,18,207); AuroraLayout::size(entropyBar_,284,12);
  lv_bar_set_range(entropyBar_,0,100); lv_bar_set_value(entropyBar_,0,LV_ANIM_OFF);
  lv_obj_set_style_bg_color(entropyBar_,lv_color_darken(DANGER,LV_OPA_70),LV_PART_MAIN);
  lv_obj_set_style_bg_opa(entropyBar_,LV_OPA_COVER,LV_PART_MAIN);
  lv_obj_set_style_bg_color(entropyBar_,DANGER,LV_PART_INDICATOR);
  entropyCount_ = label(root_,"",&aurora_font_10); AuroraLayout::pos(entropyCount_,18,224);
  lv_obj_set_style_text_color(entropyCount_,MUTED,0);
  lv_label_set_text_fmt(entropyCount_,"0 / %u échantillons",static_cast<unsigned>(TouchEntropy::REQUIRED_SAMPLES));
#endif
}

void AuroraUI::updateEntropyPreview(uint32_t token) {
  // Shift one group to the left; show only the keyed preview from the collector.
  memmove(entropyPreviewText_,entropyPreviewText_+9,26);
  entropyPreviewText_[26]=' ';
  snprintf(entropyPreviewText_+27,9,"%08lX",static_cast<unsigned long>(token));
  lv_label_set_text_static(entropyPreview_,entropyPreviewText_);
}

void AuroraUI::onTouchSample(int16_t x, int16_t y, uint16_t pressure) {
  if (screen_ != Screen::Entropy || !entropyBar_ || entropyReadyPending_) return;
#if defined(AURORA_BOARD_P4)
  if (sensorStopPending_ || x < 24 || x >= 456 || y < 152 || y >= 340) return;
#else
  if (x < 18 || x >= 302 || y < 62 || y >= 134) return;
#endif
  entropy_.add(x,y,pressure);
  const uint8_t p = entropy_.progress();
  const uint16_t count = entropy_.sampleCount();
  const lv_color_t color = p == 100 ? SUCCESS : (p >= 50 ? ORANGE : DANGER);
  lv_bar_set_value(entropyBar_,p,LV_ANIM_OFF);
  lv_obj_set_style_bg_color(entropyBar_,color,LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(entropyBar_,lv_color_darken(color,LV_OPA_70),LV_PART_MAIN);
  lv_obj_set_style_text_color(entropyStatus_,color,0);
  lv_label_set_text_fmt(entropyStatus_,"%s - %u %%",
      p == 100 ? "Collecte terminée" : (p >= 50 ? "Collecte en cours" : "Collecte insuffisante"),
      static_cast<unsigned>(p));
  lv_label_set_text_fmt(entropyCount_,"%u / %u échantillons",
      static_cast<unsigned>(count),static_cast<unsigned>(TouchEntropy::REQUIRED_SAMPLES));
  const uint32_t now = millis();
  if (count == 1 || p == 100 || now - entropyPreviewUpdatedMs_ >= 100) {
    updateEntropyPreview(entropy_.previewToken());
    entropyPreviewUpdatedMs_ = now;
  }
  if (entropy_.ready()) {
    if (entropy_.finish(mixedEntropy_)) {
      entropyCollected_ = true;
      lv_disp_trig_activity(nullptr);
      entropyReadyPending_ = true;
      entropyCompleteDueMs_ = millis() + 1000; // Leave the green state visible.
    }
    else entropyFailurePending_ = true;
  }
}

void AuroraUI::buildGenerating() {
  lv_obj_t *spinner=AuroraLayout::spinner(root_,900,70); AuroraLayout::size(spinner,58,58);
  lv_obj_set_style_arc_color(spinner,PANEL,LV_PART_MAIN);
  lv_obj_set_style_arc_color(spinner,ORANGE,LV_PART_INDICATOR);
  AuroraLayout::align(spinner,LV_ALIGN_CENTER,0,-42);
  lv_obj_t *title=label(root_,"Génération du portefeuille...",&aurora_font_16);
  AuroraLayout::align(title,LV_ALIGN_CENTER,0,13);
  lv_obj_t *hint=explanation(root_,"Calcul BIP39 / BIP32 en cours\nVeuillez patienter quelques secondes.",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_CENTER,0);
  AuroraLayout::align(hint,LV_ALIGN_CENTER,0,48);
}

void AuroraUI::buildFileProcessing() {
  lv_obj_t *spinner=AuroraLayout::spinner(root_,900,70); AuroraLayout::size(spinner,58,58);
  lv_obj_set_style_arc_color(spinner,PANEL,LV_PART_MAIN);
  lv_obj_set_style_arc_color(spinner,ORANGE,LV_PART_INDICATOR);
  AuroraLayout::align(spinner,LV_ALIGN_CENTER,0,-42);
  const bool importing=fileOperation_==FileOperation::Import ||
      fileOperation_==FileOperation::PrivateRead;
  lv_obj_t *title=label(root_,importing?"Déchiffrement en cours...":"Chiffrement en cours...",&aurora_font_16);
  AuroraLayout::align(title,LV_ALIGN_CENTER,0,13);
  lv_obj_t *hint=explanation(root_,"PBKDF2-HMAC-SHA-256 + AES-256-GCM\nVeuillez patienter.",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_CENTER,0);
  AuroraLayout::align(hint,LV_ALIGN_CENTER,0,48);
}

void AuroraUI::buildGenerationError() {
  header("Erreur de génération");
  lv_obj_t *title=label(root_,"La génération n'a pas abouti.",&aurora_font_16);
  AuroraLayout::align(title,LV_ALIGN_CENTER,0,-30);
  lv_obj_t *hint=explanation(root_,"Aucune phrase ni clé n'a été conservée.",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); AuroraLayout::align(hint,LV_ALIGN_CENTER,0,2);
  lv_obj_t *retry=button(root_,"RECOMMENCER",event,150); lv_obj_set_user_data(retry,(void*)RETRY_ENTROPY);
  AuroraLayout::align(retry,LV_ALIGN_BOTTOM_MID,0,-24);
}

void AuroraUI::buildSecurityError() {
  header("ÉCHEC DE SÉCURITÉ");
  lv_obj_t *title=label(root_,"Autotest cryptographique échoué",&aurora_font_16);
  lv_obj_set_style_text_color(title,DANGER,0); AuroraLayout::align(title,LV_ALIGN_CENTER,0,-35);
  lv_obj_t *hint=explanation(root_,"Aucune seed ne peut être générée.\nReflashez un firmware vérifié.",&aurora_font_12);
  lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_CENTER,0); AuroraLayout::align(hint,LV_ALIGN_CENTER,0,10);
  char code[28];
  snprintf(code,sizeof(code),"Code diagnostic : E%02u",static_cast<unsigned>(selfTestResult_));
  lv_obj_t *detail=label(root_,code,&aurora_font_10);
  lv_obj_set_style_text_color(detail,MUTED,0); AuroraLayout::align(detail,LV_ALIGN_BOTTOM_MID,0,-22);
}

bool AuroraUI::generate() {
  const bool ok=entropyCollected_ && engine_.create(words_,kind_,passphrase_,mixedEntropy_,wallet_);
  secureZero(mixedEntropy_,sizeof(mixedEntropy_)); entropyCollected_=false;
  return ok;
}

void AuroraUI::buildMnemonic() {
  if(protectedSession_) header("Phrase de récupération", nullptr, false);
  else
  header("Phrase de récupération",
         loadedWallet_ ? (passphrase_[0] ? "1 / 4" : "1 / 3") : "4 / 7");
  const char *warningText=loadedWallet_ ?
      "Portefeuille déchiffré. Ne photographiez jamais ces mots." :
      "Écrivez ces mots dans l'ordre. Ne les photographiez jamais.";
  lv_obj_t *warning=explanation(root_,warningText,&aurora_font_10); lv_obj_set_style_text_color(warning,ORANGE,0); AuroraLayout::pos(warning,10,41);
#if defined(AURORA_BOARD_P4)
  lv_obj_set_style_text_font(warning,&aurora_font_18,0);
  lv_label_set_long_mode(warning,LV_LABEL_LONG_WRAP);
  lv_obj_set_pos(warning,15,134); lv_obj_set_size(warning,450,48);
#endif
  const uint8_t pageCount = (words_ + 11) / 12;
  if (mnemonicPage_ >= pageCount) mnemonicPage_ = pageCount - 1;
  const uint8_t first = mnemonicPage_ * 12;
  const uint8_t last = (first + 12 < words_) ? first + 12 : words_;

  lv_obj_t *box=lv_obj_create(root_); AuroraLayout::pos(box,10,56); AuroraLayout::size(box,300,121); lv_obj_set_style_bg_color(box,PANEL,0); lv_obj_set_style_border_width(box,0,0); lv_obj_set_style_radius(box,7,0);
#if defined(AURORA_BOARD_P4)
  lv_obj_set_pos(box,15,195); lv_obj_set_size(box,450,495);
#endif
  lv_obj_set_style_pad_all(box,5,0); lv_obj_clear_flag(box,LV_OBJ_FLAG_SCROLLABLE);
  char copy[256]; strlcpy(copy,wallet_.mnemonic,sizeof(copy)); char *save=nullptr; char *w=strtok_r(copy," ",&save); uint8_t i=0;
  while(w && i<last) {
    if(i>=first) {
      const uint8_t local=i-first, col=local/6, row=local%6;
#if defined(AURORA_BOARD_P4)
      const int x=col*218, y=10+row*78;
#else
      const int x=col*145, y=row*19;
#endif
      char number[4]; snprintf(number,sizeof(number),"%02u",static_cast<unsigned>(i)+1U);
      lv_obj_t *n=label(box,number,&aurora_font_12); lv_obj_set_style_text_color(n,ORANGE,0);
      lv_obj_t *word=label(box,w,&aurora_font_16);
#if defined(AURORA_BOARD_P4)
      lv_obj_set_pos(n,x,y+6); lv_obj_set_style_text_font(n,&aurora_font_18,0);
      lv_obj_set_pos(word,x+43,y); lv_obj_set_style_text_font(word,&aurora_font_30,0);
#else
      AuroraLayout::pos(n,x,y+2); AuroraLayout::pos(word,x+29,y);
#endif
    }
    w=strtok_r(nullptr," ",&save); ++i;
  }
  secureZero(copy,sizeof(copy));

  // Reviewing an unlocked wallet is not a step in the creation wizard.
  // Keep a way to leave immediately, including on the first of two pages.
  if(protectedSession_) {
    if(mnemonicPage_>0) {
      lv_obj_t *previous=button(root_,"PRÉCÉDENT",event,94);
      lv_obj_set_user_data(previous,(void*)MNEMONIC_PREVIOUS);
#if defined(AURORA_BOARD_P4)
      lv_obj_set_pos(previous,15,720); lv_obj_set_size(previous,140,64);
#else
      lv_obj_set_pos(previous,10,190);
#endif
    }
    lv_obj_t *back=button(root_,"RETOUR",event,94);
    lv_obj_set_user_data(back,(void*)LOCK_SESSION);
#if defined(AURORA_BOARD_P4)
    lv_obj_set_pos(back,170,720); lv_obj_set_size(back,140,64);
#else
    lv_obj_set_pos(back,113,190);
#endif
    if(mnemonicPage_+1<pageCount) {
      lv_obj_t *next=button(root_,"SUIVANT",event,94);
      lv_obj_set_user_data(next,(void*)MNEMONIC_NEXT);
#if defined(AURORA_BOARD_P4)
      lv_obj_set_pos(next,325,720); lv_obj_set_size(next,140,64);
#else
      lv_obj_set_pos(next,216,190);
#endif
    }
    return;
  }
  if(mnemonicPage_>0) {
    lv_obj_t *previous=button(root_,"< PRÉCÉDENT",event,112); lv_obj_set_user_data(previous,(void*)MNEMONIC_PREVIOUS); AuroraLayout::pos(previous,10,190);
#if defined(AURORA_BOARD_P4)
    lv_obj_set_y(previous,720);
#endif
  }
  if(mnemonicPage_+1<pageCount) {
    lv_obj_t *next=button(root_,"SUIVANT >",event,112); lv_obj_set_user_data(next,(void*)MNEMONIC_NEXT); AuroraLayout::pos(next,198,190);
#if defined(AURORA_BOARD_P4)
    lv_obj_set_y(next,720);
#endif
  } else {
    lv_obj_t *done=button(root_,loadedWallet_?"SUIVANT":"J'AI NOTÉ",event,132);
    const Action nextAction=(loadedWallet_ && !manualRestore_ && passphrase_[0]) ?
        SHOW_LOADED_PASSPHRASE : (loadedWallet_ ? TO_INFO : NEXT_VERIFY);
    lv_obj_set_user_data(done,(void*)nextAction); AuroraLayout::pos(done,178,190);
#if defined(AURORA_BOARD_P4)
    lv_obj_set_y(done,720);
#endif
  }
}

void AuroraUI::buildPassphraseReveal() {
  header("Mot supplémentaire", "2 / 4");
  lv_obj_t *warning=explanation(root_,
      "DANGER : cette passphrase BIP39 est indispensable pour retrouver exactement ce portefeuille.",
      &aurora_font_10);
  lv_label_set_long_mode(warning,LV_LABEL_LONG_WRAP); AuroraLayout::size(warning,292,31);
  lv_obj_set_style_text_color(warning,DANGER,0); AuroraLayout::pos(warning,14,43);

  lv_obj_t *panel=lv_obj_create(root_); AuroraLayout::pos(panel,12,78); AuroraLayout::size(panel,296,91);
  lv_obj_set_style_bg_color(panel,PANEL,0); lv_obj_set_style_border_color(panel,DANGER,0);
  lv_obj_set_style_border_width(panel,1,0); lv_obj_set_style_radius(panel,7,0);
  lv_obj_set_style_pad_all(panel,7,0); lv_obj_clear_flag(panel,LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *value=label(panel,passphrase_,&aurora_font_14);
  lv_label_set_long_mode(value,LV_LABEL_LONG_WRAP); AuroraLayout::size(value,278,74);
  lv_obj_set_style_text_color(value,lv_color_white(),0); AuroraLayout::pos(value,1,1);

  lv_obj_t *next=button(root_,"CONTINUER",event,132);
  lv_obj_set_user_data(next,(void*)TO_INFO); AuroraLayout::pos(next,94,190);
}

void AuroraUI::selectVerifyWords() {
  for(uint8_t i=0;i<3;++i) {
    uint8_t candidate;
    bool duplicate;
    do {
      uint32_t randomValue;
      const uint32_t threshold = static_cast<uint32_t>(-words_) % words_;
      do randomValue = hardwareRandom32(); while (randomValue < threshold);
      candidate=randomValue%words_;
      duplicate=false;
      for(uint8_t j=0;j<i;++j) duplicate |= verifyIndex_[j]==candidate;
    } while(duplicate);
    verifyIndex_[i]=candidate;
  }
  for(uint8_t i=0;i<2;++i) for(uint8_t j=i+1;j<3;++j) {
    if(verifyIndex_[j]<verifyIndex_[i]) {
      uint8_t tmp=verifyIndex_[i]; verifyIndex_[i]=verifyIndex_[j]; verifyIndex_[j]=tmp;
    }
  }
}

void AuroraUI::buildVerify() {
  header("Vérifier la sauvegarde", "5 / 7"); selectVerifyWords();
  verifyActiveIndex_=0; verifySuggestionCount_=0;
  secureZero(verifySuggestions_,sizeof(verifySuggestions_));
  for(uint8_t i=0;i<3;++i) {
    verifyArea_[i]=createInput(root_); AuroraLayout::pos(verifyArea_[i],8+i*104,40);
    AuroraLayout::size(verifyArea_[i],94,36); AuroraLayout::font(verifyArea_[i],&aurora_font_12,0);
    lv_textarea_set_one_line(verifyArea_[i],true);
    lv_textarea_set_max_length(verifyArea_[i],WalletEngine::BIP39_WORD_CAPACITY-1);
    lv_textarea_set_accepted_chars(verifyArea_[i],"abcdefghijklmnopqrstuvwxyz");
    char ph[16]; snprintf(ph,sizeof(ph),"Mot n° %u",verifyIndex_[i]+1);
    lv_textarea_set_placeholder_text(verifyArea_[i],ph);
    lv_obj_set_user_data(verifyArea_[i],(void*)VERIFY_WORD_CHANGED);
    lv_obj_add_event_cb(verifyArea_[i],event,LV_EVENT_FOCUSED,nullptr);
    lv_obj_add_event_cb(verifyArea_[i],event,LV_EVENT_VALUE_CHANGED,nullptr);

    verifySuggestionButtons_[i]=button(root_,"",event,94);
    AuroraLayout::size(verifySuggestionButtons_[i],94,26);
    AuroraLayout::pos(verifySuggestionButtons_[i],8+i*104,80);
    lv_obj_set_user_data(verifySuggestionButtons_[i],reinterpret_cast<void *>(static_cast<uintptr_t>(VERIFY_SUGGESTION_0+i)));
  }
  updateVerifySuggestions();
  keyboard_=createKeyboard(root_); AuroraLayout::size(keyboard_,320,112); AuroraLayout::align(keyboard_,LV_ALIGN_BOTTOM_MID,0,0); lv_keyboard_set_mode(keyboard_,LV_KEYBOARD_MODE_TEXT_LOWER); lv_keyboard_set_textarea(keyboard_,verifyArea_[0]);
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_READY,(void*)CHECK_VERIFY);
}

void AuroraUI::updateVerifySuggestions() {
  secureZero(verifySuggestions_,sizeof(verifySuggestions_));
  const char *prefix=verifyArea_[verifyActiveIndex_]?
      lv_textarea_get_text(verifyArea_[verifyActiveIndex_]):nullptr;
  verifySuggestionCount_=WalletEngine::bip39Suggestions(
      prefix,reinterpret_cast<char *>(verifySuggestions_),
      WalletEngine::BIP39_WORD_CAPACITY,3);
  for(uint8_t i=0;i<3;++i) {
    if(!verifySuggestionButtons_[i]) continue;
    if(i<verifySuggestionCount_) {
      lv_label_set_text(lv_obj_get_child(verifySuggestionButtons_[i],0),verifySuggestions_[i]);
      lv_obj_clear_flag(verifySuggestionButtons_[i],LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(verifySuggestionButtons_[i],LV_OBJ_FLAG_HIDDEN);
    }
  }
}

void AuroraUI::acceptVerifySuggestion(uint8_t index) {
  if(index>=verifySuggestionCount_ || !verifyArea_[verifyActiveIndex_]) return;
  char selected[WalletEngine::BIP39_WORD_CAPACITY]{};
  strlcpy(selected,verifySuggestions_[index],sizeof(selected));
  lv_textarea_set_text(verifyArea_[verifyActiveIndex_],selected);
  if(verifyActiveIndex_<2) {
    ++verifyActiveIndex_;
    lv_keyboard_set_textarea(keyboard_,verifyArea_[verifyActiveIndex_]);
  }
  updateVerifySuggestions();
  secureZero(selected,sizeof(selected));
}

bool AuroraUI::verifyWords() {
  if(!ensureSd(Screen::Verify)) return false;
  char copy[256]; strlcpy(copy,wallet_.mnemonic,sizeof(copy)); const char *wanted[3]={}; char *save=nullptr; char *w=strtok_r(copy," ",&save); uint8_t idx=0;
  while(w){ for(int j=0;j<3;++j) if(idx==verifyIndex_[j]) wanted[j]=w; w=strtok_r(nullptr," ",&save); ++idx; }
  bool ok=true;
  for(int j=0;j<3;++j) {
    const char *entered=lv_textarea_get_text(verifyArea_[j]);
    if(!WalletEngine::bip39Word(entered)) {
      char suggestions[3][WalletEngine::BIP39_WORD_CAPACITY]{};
      const uint8_t count=WalletEngine::bip39Suggestions(
          entered,reinterpret_cast<char *>(suggestions),
          WalletEngine::BIP39_WORD_CAPACITY,3);
      if(count==1) lv_textarea_set_text(verifyArea_[j],suggestions[0]);
      secureZero(suggestions,sizeof(suggestions));
    }
    ok &= wanted[j] && strcmp(wanted[j],lv_textarea_get_text(verifyArea_[j]))==0;
  }
  secureZero(copy,sizeof(copy)); return ok;
}

void AuroraUI::buildInfo() {
  if(protectedSession_) lv_obj_set_style_pad_all(root_,0,0);
  const char *step=manualRestore_ ? "2 / 3" :
      (loadedWallet_ ? (passphrase_[0] ? "3 / 4" : "2 / 3") : "6 / 7");
  header(protectedSession_?"Portefeuille":"Informations du portefeuille",protectedSession_?nullptr:step);
  lv_obj_t *p=lv_obj_create(root_); AuroraLayout::pos(p,10,42); AuroraLayout::size(p,300,132); lv_obj_set_style_bg_color(p,PANEL,0); lv_obj_set_style_border_width(p,0,0); lv_obj_set_style_radius(p,7,0); lv_obj_clear_flag(p,LV_OBJ_FLAG_SCROLLABLE);
  char txt[500];
#if defined(AURORA_BOARD_P4)
  snprintf(txt,sizeof(txt),"Adresse\n%s\n\nChemin : %s\n\nClé publique étendue du compte\n%s",wallet_.address,wallet_.path,wallet_.accountXpub);
#else
  snprintf(txt,sizeof(txt),"Adresse\n%s\nChemin : %s\nClé publique étendue du compte\n%s",wallet_.address,wallet_.path,wallet_.accountXpub);
#endif
  lv_obj_t *l=label(p,txt,&aurora_font_10); lv_label_set_long_mode(l,LV_LABEL_LONG_WRAP); AuroraLayout::size(l,290,124); AuroraLayout::pos(l,5,3);
#if defined(AURORA_BOARD_P4)
  lv_obj_set_style_pad_all(p,0,0);
  lv_obj_set_pos(p,15,140); lv_obj_set_size(p,450,protectedSession_?484:550);
  lv_obj_set_pos(l,12,16); lv_obj_set_size(l,426,protectedSession_?410:518);
  lv_obj_set_style_text_font(l,&aurora_font_18,0);
#endif
  if(protectedSession_ && exportSucceeded_ && exportStatus_[0]) {
    lv_obj_t *saved=label(p,exportStatus_,&aurora_font_10);
    lv_obj_set_style_text_color(saved,SUCCESS,0); AuroraLayout::pos(saved,5,110);
#if defined(AURORA_BOARD_P4)
    lv_obj_set_pos(saved,12,438); lv_obj_set_style_text_font(saved,&aurora_font_18,0);
#endif
  }
  lv_obj_t *q=button(root_,"CODES QR",event,95); lv_obj_set_user_data(q,(void*)TO_QR_ADDRESS); AuroraLayout::pos(q,10,181);
  lv_obj_t *r=button(root_,"CLÉ PRIVÉE",event,100); lv_obj_set_user_data(r,(void*)REVEAL_PRIVATE); AuroraLayout::pos(r,110,181);
#if defined(AURORA_BOARD_P4)
  lv_label_set_text(lv_obj_get_child(q,0),"CLÉ PUBLIQUE");
  lv_obj_set_style_bg_color(q,SUCCESS,0);
  if(protectedSession_) lv_obj_set_style_bg_color(r,DANGER,0);
#endif
  const char *lastText=protectedSession_?"EXPORTER":(manualRestore_?"EXPORTER":(loadedWallet_?"EFFACER":"SUIVANT"));
  const Action lastAction=protectedSession_?TO_BACKUP:(manualRestore_?TO_BACKUP:(loadedWallet_?DO_WIPE:TO_BACKUP));
  lv_obj_t *x=button(root_,lastText,event,95);
  lv_obj_set_user_data(x,(void*)lastAction); AuroraLayout::pos(x,215,181);
#if defined(AURORA_BOARD_P4)
  lv_obj_set_y(q,720); lv_obj_set_y(r,720); lv_obj_set_y(x,720);
#endif
  if(protectedSession_) {
#if defined(AURORA_BOARD_P4)
    lv_obj_set_pos(q,15,648); lv_obj_set_size(q,143,64);
    lv_obj_set_pos(r,165,648); lv_obj_set_size(r,150,64);
    lv_obj_set_pos(x,322,648); lv_obj_set_size(x,143,64);
#else
    AuroraLayout::pos(q,10,177); AuroraLayout::size(q,95,26);
    AuroraLayout::pos(r,110,177); AuroraLayout::size(r,100,26);
    AuroraLayout::pos(x,215,177); AuroraLayout::size(x,95,26);
#endif
    const char *names[3]={"MOTS","PASSPHRASE","VERROUILLER"};
    const Action actions[3]={SHOW_WORDS,SHOW_LOADED_PASSPHRASE,LOCK_SESSION};
    for(unsigned i=0;i<3;++i) {
      lv_obj_t *b=button(root_,names[i],event,95);
#if defined(AURORA_BOARD_P4)
      lv_obj_set_pos(b,15+i*153,720); lv_obj_set_size(b,143,64);
#else
      AuroraLayout::pos(b,10+i*102,207); AuroraLayout::size(b,95,26);
#endif
#if defined(AURORA_BOARD_P4)
      AuroraLayout::font(lv_obj_get_child(b,0),&aurora_font_10,0);
#endif
      lv_obj_set_user_data(b,(void*)actions[i]);
      if(actions[i]==SHOW_WORDS || actions[i]==SHOW_LOADED_PASSPHRASE)
        lv_obj_set_style_bg_color(b,DANGER,0);
      if(actions[i]==SHOW_LOADED_PASSPHRASE && !hasPassphrase()) {
        lv_obj_add_state(b,LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(b,PANEL,LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(b,0),MUTED,0);
        lv_obj_set_style_bg_opa(b,LV_OPA_COVER,LV_STATE_DISABLED);
      }
    }
  }
}

bool AuroraUI::renderQr(lv_obj_t *parent,const char *data,int size,int x,int y) {
  if (!data || !data[0]) return false;
#if LVGL_VERSION_MAJOR >= 9
  lv_obj_t *qr = lv_qrcode_create(parent);
  if (qr) {
    lv_qrcode_set_size(qr, AuroraLayout::x(size));
    lv_qrcode_set_dark_color(qr, lv_color_black());
    lv_qrcode_set_light_color(qr, lv_color_white());
  }
#else
  lv_obj_t *qr = lv_qrcode_create(parent, size, lv_color_black(), lv_color_white());
#endif
  if (!qr) return false;
  if (lv_qrcode_update(qr, data, strlen(data)) != LV_RES_OK) {
    lv_obj_del(qr);
    return false;
  }
  AuroraLayout::pos(qr, x, y);
  return true;
}

void AuroraUI::buildQr() {
  const bool restoreView=manualRestore_;
  const bool privateKey = qrContent_ == QrContent::PrivateKey;
  const char *title = privateKey ? "Clé privée - DANGER" :
                      (qrContent_ == QrContent::AccountXpub ? "Clé publique étendue" : "Code QR de l'adresse");
  const char *data = privateKey ? wallet_.privateWif :
                     (qrContent_ == QrContent::AccountXpub ? wallet_.accountXpub : wallet_.address);
  lv_obj_t *titleLabel=header(title); if(privateKey) lv_obj_set_style_text_color(titleLabel,DANGER,0);

#if defined(AURORA_BOARD_P4)
  if(restoreView) {
    static constexpr char DERIVATIONS[] =
        "Legacy - m/44'/0'/0'/0/0\n"
        "Nested SegWit - m/49'/0'/0'/0/0\n"
        "Native SegWit - m/84'/0'/0'/0/0\n"
        "Taproot - m/86'/0'/0'/0/0";
    restoreDerivationDropdown_=lv_dropdown_create(root_);
    lv_obj_set_pos(restoreDerivationDropdown_,72,124); lv_obj_set_size(restoreDerivationDropdown_,336,48);
    lv_obj_set_style_text_font(restoreDerivationDropdown_,&aurora_font_18,0);
    lv_dropdown_set_options(restoreDerivationDropdown_,DERIVATIONS);
    lv_dropdown_set_selected(restoreDerivationDropdown_,static_cast<uint16_t>(wallet_.kind));
    lv_obj_set_user_data(restoreDerivationDropdown_,(void*)RESTORE_DERIVATION_CHANGED);
    lv_obj_add_event_cb(restoreDerivationDropdown_,event,LV_EVENT_VALUE_CHANGED,nullptr);
  }

  const int qrTop=restoreView?190:130;
  if(!renderQr(root_,data,224,48,restoreView?57:39)) {
    lv_obj_t *error=label(root_,"QR impossible",&aurora_font_20);
    lv_obj_set_style_text_color(error,DANGER,0);lv_obj_set_pos(error,24,qrTop+130);
  }
  lv_obj_t *value=label(root_,data,&aurora_font_20);
  lv_label_set_long_mode(value,LV_LABEL_LONG_WRAP);
  lv_obj_set_pos(value,24,restoreView?540:480);
  lv_obj_set_size(value,432,restoreView?160:190);

  const int buttonWidth=210;
  bool hasAlternate=false;
  if(restoreView) {
    lv_obj_t *alternate=button(root_,privateKey?"ADRESSE":"CLÉ PRIVÉE",event,140);
    lv_obj_set_user_data(alternate,(void*)(privateKey?TO_QR_ADDRESS:REVEAL_PRIVATE));
    lv_obj_set_pos(alternate,24,720);lv_obj_set_size(alternate,buttonWidth,64);
    lv_obj_set_style_bg_color(alternate,privateKey?SUCCESS:(protectedSession_?DANGER:ORANGE),0);
    hasAlternate=true;
  } else if(qrContent_==QrContent::Address) {
    lv_obj_t *alternate=button(root_,"CLÉ ÉTENDUE",event,140);
    lv_obj_set_user_data(alternate,(void*)TO_QR_PUBLIC);
    lv_obj_set_pos(alternate,24,720);lv_obj_set_size(alternate,buttonWidth,64);
    hasAlternate=true;
  } else if(qrContent_==QrContent::AccountXpub) {
    lv_obj_t *alternate=button(root_,"ADRESSE",event,140);
    lv_obj_set_user_data(alternate,(void*)TO_QR_ADDRESS);
    lv_obj_set_pos(alternate,24,720);lv_obj_set_size(alternate,buttonWidth,64);
    hasAlternate=true;
  }
  lv_obj_t *back=button(root_,restoreView?"INFORMATIONS":"RETOUR",event,140);
  lv_obj_set_user_data(back,(void*)(fileSession_ && privateKey?LOCK_SESSION:TO_INFO));
  lv_obj_set_pos(back,hasAlternate?246:135,720);lv_obj_set_size(back,buttonWidth,64);
#else
  if(restoreView) {
    static constexpr char DERIVATIONS[] =
        "Legacy - m/44'/0'/0'/0/0\n"
        "Nested SegWit - m/49'/0'/0'/0/0\n"
        "Native SegWit - m/84'/0'/0'/0/0\n"
        "Taproot - m/86'/0'/0'/0/0";
    restoreDerivationDropdown_=lv_dropdown_create(root_);
    AuroraLayout::pos(restoreDerivationDropdown_,6,39); AuroraLayout::size(restoreDerivationDropdown_,128,30);
    AuroraLayout::font(restoreDerivationDropdown_,&aurora_font_10,0);
    lv_dropdown_set_options(restoreDerivationDropdown_,DERIVATIONS);
    lv_dropdown_set_selected(restoreDerivationDropdown_,static_cast<uint16_t>(wallet_.kind));
    lv_obj_set_user_data(restoreDerivationDropdown_,(void*)RESTORE_DERIVATION_CHANGED);
    lv_obj_add_event_cb(restoreDerivationDropdown_,event,LV_EVENT_VALUE_CHANGED,nullptr);
  }

  if (!renderQr(root_,data,restoreView?128:158,6,restoreView?73:42)) {
    lv_obj_t *error=label(root_,"QR impossible",&aurora_font_14); lv_obj_set_style_text_color(error,DANGER,0); AuroraLayout::align(error,LV_ALIGN_LEFT_MID,25,0);
  }
  const int rightX = restoreView?140:168;
  const int rightWidth = restoreView?164:136;
  lv_obj_t *l=label(root_,data,&aurora_font_10); lv_label_set_long_mode(l,LV_LABEL_LONG_WRAP);
  AuroraLayout::size(l,rightWidth,restoreView?68:(privateKey ? 139 : 99));
  AuroraLayout::pos(l,rightX,restoreView?75:43);

  if(restoreView) {
    lv_obj_t *alternate=button(root_,privateKey?"ADRESSE":"CLÉ PRIVÉE",event,rightWidth);
    lv_obj_set_user_data(alternate,(void*)(privateKey?TO_QR_ADDRESS:REVEAL_PRIVATE));
    AuroraLayout::pos(alternate,rightX,148);
    if(!privateKey) lv_obj_set_style_bg_color(alternate,ORANGE,0);
    else lv_obj_set_style_bg_color(alternate,DANGER,0);
  } else if (qrContent_ == QrContent::Address) {
    lv_obj_t *p=button(root_,"CLÉ ÉTENDUE",event,rightWidth);
    lv_obj_set_user_data(p,(void*)TO_QR_PUBLIC); AuroraLayout::pos(p,rightX,147);
  } else if (qrContent_ == QrContent::AccountXpub) {
    lv_obj_t *a=button(root_,"ADRESSE",event,rightWidth);
    lv_obj_set_user_data(a,(void*)TO_QR_ADDRESS); AuroraLayout::pos(a,rightX,147);
  }
  lv_obj_t *b=button(root_,restoreView?"INFORMATIONS":"RETOUR",event,rightWidth);
  lv_obj_set_user_data(b,(void*)(fileSession_ && privateKey?LOCK_SESSION:TO_INFO)); AuroraLayout::pos(b,rightX,190);
#endif
}

void AuroraUI::buildBackup() {
  header("Sauvegarde sur microSD", manualRestore_ ? "3 / 3" : "7 / 7");
  lv_obj_t *intro=explanation(root_,"Choisissez un format (carte FAT32).",&aurora_font_10);
  lv_obj_set_style_text_color(intro,MUTED,0); AuroraLayout::pos(intro,12,41);
#if defined(AURORA_BOARD_P4)
  lv_obj_set_style_text_font(intro,&aurora_font_18,0);
#endif

  lv_obj_t *aurora=button(root_,"AURORA WALLET CHIFFRÉ",event,280);
  lv_obj_set_user_data(aurora,(void*)EXPORT_AURORA); AuroraLayout::pos(aurora,20,61);
#if defined(AURORA_BOARD_P4)
  if(protectedSession_) lv_obj_set_style_bg_color(aurora,DANGER,0);
#endif
  lv_obj_t *electrum=button(root_,"ELECTRUM PRIVÉ NON CHIFFRÉ",event,280);
  lv_obj_set_user_data(electrum,(void*)EXPORT_ELECTRUM); AuroraLayout::pos(electrum,20,105);
  lv_obj_set_style_bg_color(electrum,DANGER,0);
  if (wallet_.kind == AddressKind::Taproot) {
    lv_obj_add_state(electrum,LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(electrum,MUTED,LV_STATE_DISABLED);
  }

  if (exportStatus_[0]) {
    lv_obj_t *status=explanation(root_,exportStatus_,&aurora_font_10);
    lv_label_set_long_mode(status,LV_LABEL_LONG_WRAP); AuroraLayout::size(status,296,42);
    AuroraLayout::pos(status,12,143);
    lv_obj_set_style_text_color(status,
        strncmp(exportStatus_,"Créé :",strlen("Créé :"))==0 ? ORANGE : DANGER,0);
  } else {
    const char *text=wallet_.kind==AddressKind::Taproot ?
        "Aurora Wallet : AES-256-GCM.\nElectrum indisponible en Taproot." :
        "Aurora Wallet est chiffré et authentifié.\nElectrum contient le xprv en clair.";
    lv_obj_t *details=explanation(root_,text,&aurora_font_10);
    lv_label_set_long_mode(details,LV_LABEL_LONG_WRAP); AuroraLayout::size(details,296,34);
    lv_obj_set_style_text_color(details,MUTED,0); AuroraLayout::pos(details,12,143);
  }
  lv_obj_t *wipe=button(root_,"EFFACER",event,122);
  lv_obj_set_user_data(wipe,(void*)DO_WIPE); AuroraLayout::pos(wipe,99,196);
}

void AuroraUI::buildExportWarning() {
  const bool aurora=exportFormat_==WalletExportFormat::AuroraWallet;
  header(aurora ? "Aurora Wallet chiffré" : "Electrum privé - DANGER",
         manualRestore_ ? "3 / 3" : "7 / 7");
  lv_obj_t *danger=label(root_,aurora?"AES-256-GCM":"SECRETS NON CHIFFRÉS",&aurora_font_16);
  lv_obj_set_style_text_color(danger,aurora?ORANGE:DANGER,0); AuroraLayout::align(danger,LV_ALIGN_TOP_MID,0,50);
  const char *message=aurora ?
      "Les mots et les clés seront chiffrés avec un mot de passe.\n"
      "Mot de passe perdu = fichier définitivement illisible." :
      "Ce fichier Electrum contient le xprv du compte en clair.\n"
      "Toute personne qui le possède peut dépenser les bitcoins.";
  lv_obj_t *warning=explanation(root_,message,&aurora_font_12);
  lv_label_set_long_mode(warning,LV_LABEL_LONG_WRAP); AuroraLayout::size(warning,292,76);
  lv_obj_set_style_text_align(warning,LV_TEXT_ALIGN_CENTER,0); AuroraLayout::pos(warning,14,82);
  lv_obj_t *cancel=button(root_,"ANNULER",event,120);
  lv_obj_set_user_data(cancel,(void*)BACK_BACKUP); AuroraLayout::pos(cancel,20,184);
  lv_obj_t *confirm=button(root_,aurora?"CONTINUER":"JE COMPRENDS",event,142);
  lv_obj_set_user_data(confirm,(void*)CONFIRM_PRIVATE); AuroraLayout::pos(confirm,158,184);
  if(!aurora) lv_obj_set_style_bg_color(confirm,DANGER,0);
}

void AuroraUI::buildExportName() {
  header("Nom du fichier", manualRestore_ ? "3 / 3" : "7 / 7");
  lv_obj_t *hint=explanation(root_,"Saisissez le nom sans extension (24 caractères max.)",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); AuroraLayout::pos(hint,12,41);
  exportNameArea_=createInput(root_); AuroraLayout::pos(exportNameArea_,12,57);
  AuroraLayout::size(exportNameArea_,296,38); AuroraLayout::font(exportNameArea_,&aurora_font_12,0);
  lv_textarea_set_one_line(exportNameArea_,true); lv_textarea_set_max_length(exportNameArea_,24);
  lv_textarea_set_accepted_chars(exportNameArea_,FILE_NAME_CHARS);
  lv_textarea_set_placeholder_text(exportNameArea_,"mon_nom");
  lv_textarea_set_text(exportNameArea_,exportBaseName_);
  char preview[80] = {};
  snprintf(preview,sizeof(preview),"Suffixe : %s   Validez avec ENTRÉE.",walletExportSuffix(exportFormat_));
  lv_obj_t *suffix=explanation(root_,preview,&aurora_font_10);
  lv_obj_set_style_text_color(suffix,ORANGE,0); AuroraLayout::pos(suffix,12,103);
  keyboard_=createKeyboard(root_); AuroraLayout::size(keyboard_,320,112);
  AuroraLayout::align(keyboard_,LV_ALIGN_BOTTOM_MID,0,0); lv_keyboard_set_textarea(keyboard_,exportNameArea_);
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_READY,(void*)SAVE_EXPORT);
}

void AuroraUI::buildExportPassword() {
  header("Mot de passe du fichier", manualRestore_ ? "3 / 3" : "7 / 7");
  const char *message=passwordStatus_[0] ? passwordStatus_ :
      "12 à 63 caractères. Conservez ce mot de passe séparément.";
  lv_obj_t *hint=explanation(root_,message,&aurora_font_10);
  lv_label_set_long_mode(hint,LV_LABEL_LONG_WRAP); AuroraLayout::size(hint,296,20);
  lv_obj_set_style_text_color(hint,passwordStatus_[0]?DANGER:ORANGE,0); AuroraLayout::pos(hint,12,39);

  filePasswordArea_=createInput(root_); AuroraLayout::pos(filePasswordArea_,12,58);
  AuroraLayout::size(filePasswordArea_,143,35); AuroraLayout::font(filePasswordArea_,&aurora_font_10,0);
  lv_textarea_set_one_line(filePasswordArea_,true); lv_textarea_set_password_mode(filePasswordArea_,true);
  lv_textarea_set_password_show_time(filePasswordArea_,0);
  lv_textarea_set_max_length(filePasswordArea_,63); lv_textarea_set_accepted_chars(filePasswordArea_,PASSPHRASE_ASCII);
  lv_textarea_set_placeholder_text(filePasswordArea_,"Mot de passe");
  lv_obj_add_event_cb(filePasswordArea_,event,LV_EVENT_FOCUSED,nullptr);

  filePasswordConfirmArea_=createInput(root_); AuroraLayout::pos(filePasswordConfirmArea_,165,58);
  AuroraLayout::size(filePasswordConfirmArea_,143,35); AuroraLayout::font(filePasswordConfirmArea_,&aurora_font_10,0);
  lv_textarea_set_one_line(filePasswordConfirmArea_,true); lv_textarea_set_password_mode(filePasswordConfirmArea_,true);
  lv_textarea_set_password_show_time(filePasswordConfirmArea_,0);
  lv_textarea_set_max_length(filePasswordConfirmArea_,63); lv_textarea_set_accepted_chars(filePasswordConfirmArea_,PASSPHRASE_ASCII);
  lv_textarea_set_placeholder_text(filePasswordConfirmArea_,"Confirmation");
  lv_obj_add_event_cb(filePasswordConfirmArea_,event,LV_EVENT_FOCUSED,nullptr);

  lv_obj_t *note=explanation(root_,"Ce mot de passe est différent de la passphrase BIP39.",&aurora_font_10);
  lv_obj_set_style_text_color(note,MUTED,0); AuroraLayout::pos(note,12,103);
  keyboard_=createKeyboard(root_); AuroraLayout::size(keyboard_,320,112);
  AuroraLayout::align(keyboard_,LV_ALIGN_BOTTOM_MID,0,0); lv_keyboard_set_textarea(keyboard_,filePasswordArea_);
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_READY,(void*)SAVE_EXPORT_PASSWORD);
}

void AuroraUI::performWalletExport() {
  exportSucceeded_=false;
  if(!authorized(Access::Export)) {
    strlcpy(exportStatus_,"Autorisation expirée : export annulé.",sizeof(exportStatus_));
    revokeAccess();
    secureZero(filePassword_,sizeof(filePassword_));
    return;
  }
  char accountXprv[128] = {};
  char writtenPath[56] = {};
  if (exportFormat_==WalletExportFormat::ElectrumPrivate &&
      wallet_.kind == AddressKind::Taproot) {
    strlcpy(exportStatus_,"Taproot / BIP86 non pris en charge par Electrum.",sizeof(exportStatus_));
    revokeAccess();
    secureZero(filePassword_,sizeof(filePassword_));
    return;
  }
  if (!engine_.accountXprv(wallet_,passphrase_,accountXprv,sizeof(accountXprv))) {
    strlcpy(exportStatus_,"Échec de dérivation de la clé privée étendue.",sizeof(exportStatus_));
    secureZero(accountXprv,sizeof(accountXprv));
    revokeAccess();
    secureZero(filePassword_,sizeof(filePassword_));
    return;
  }
  if(!authorized(Access::Export) || (sensitiveStateActive_ &&
      lv_disp_get_inactive_time(nullptr)>=SESSION_IDLE_MS)) {
    secureZero(accountXprv,sizeof(accountXprv));
    revokeAccess();
    secureZero(filePassword_,sizeof(filePassword_));
    strlcpy(exportStatus_,"Autorisation expirée : aucun fichier écrit.",sizeof(exportStatus_));
    return;
  }

  const WalletExportData data{
      static_cast<uint8_t>(wallet_.kind), words_, addressKindName(wallet_.kind),
      wallet_.path, wallet_.mnemonic, passphrase_, wallet_.address,
      wallet_.accountXpub, accountXprv, wallet_.privateWif,
      wallet_.watchDescriptor,
      nullptr};
  uint8_t fileFingerprint[32]{};
  const WalletExportResult result=exportFormat_==WalletExportFormat::AuroraWallet?
      writeAuroraWalletFileVerified(exportBaseName_,filePassword_,data,writtenPath,sizeof(writtenPath),fileFingerprint):
      writeWalletExportFile(exportFormat_,exportBaseName_,filePassword_,data,writtenPath,sizeof(writtenPath));
  secureZero(accountXprv,sizeof(accountXprv));
  switch(result) {
    case WalletExportResult::Ok:
      exportSucceeded_=true;
      if(exportFormat_==WalletExportFormat::AuroraWallet) {
        startFileSession(fileFingerprint,exportBaseName_);
        protectedSession_=loadedWallet_=true;
      }
      snprintf(exportStatus_,sizeof(exportStatus_),"Créé : %s",writtenPath); break;
    case WalletExportResult::InvalidName:
      strlcpy(exportStatus_,"Nom de fichier invalide.",sizeof(exportStatus_)); break;
    case WalletExportResult::InvalidData:
      strlcpy(exportStatus_,"Données incomplètes : export annulé.",sizeof(exportStatus_)); break;
    case WalletExportResult::UnsupportedFormat:
      strlcpy(exportStatus_,"Format indisponible pour ce portefeuille.",sizeof(exportStatus_)); break;
    case WalletExportResult::WeakPassword:
      strlcpy(exportStatus_,"Mot de passe trop court ou invalide.",sizeof(exportStatus_)); break;
    case WalletExportResult::InvalidPin:
      strlcpy(exportStatus_,"PIN manquant ou invalide : export annulé.",sizeof(exportStatus_)); break;
    case WalletExportResult::NoCard:
      strlcpy(exportStatus_,"Carte microSD absente ou illisible.",sizeof(exportStatus_)); break;
    case WalletExportResult::AlreadyExists:
      strlcpy(exportStatus_,"Ce fichier existe déjà : choisissez un autre nom.",sizeof(exportStatus_)); break;
    case WalletExportResult::OpenFailed:
      strlcpy(exportStatus_,"Impossible de créer le fichier sur la carte.",sizeof(exportStatus_)); break;
    case WalletExportResult::MemoryFailed:
      strlcpy(exportStatus_,"Mémoire insuffisante : export annulé.",sizeof(exportStatus_)); break;
    case WalletExportResult::CryptoFailed:
      strlcpy(exportStatus_,"Échec du chiffrement : export annulé.",sizeof(exportStatus_)); break;
    case WalletExportResult::WriteFailed:
      strlcpy(exportStatus_,"Écriture ou vérification échouée : vérifiez la carte.",sizeof(exportStatus_)); break;
#if defined(AURORA_BOARD_P4)
    case WalletExportResult::FinalizeFailed:
      strlcpy(exportStatus_,"Sauvegarde non confirmée : vérifiez le fichier sur la carte.",sizeof(exportStatus_)); break;
#endif
  }
  secureZero(fileFingerprint,sizeof(fileFingerprint));
  revokeAccess();
  secureZero(filePassword_,sizeof(filePassword_));
  secureZero(writtenPath,sizeof(writtenPath));
}

bool AuroraUI::performWalletImport() {
  uint8_t fileFingerprint[32]{};
  AuroraWalletData imported{};
  char derivedXprv[128] = {};
  bool ok = false;
  const AuroraWalletReadResult result=readAuroraWalletFileChecked(
      importBaseName_,filePassword_,imported,fileFingerprint);
  secureZero(filePassword_,sizeof(filePassword_));
  if(result==AuroraWalletReadResult::Ok) {
    const AddressKind importedKind=static_cast<AddressKind>(imported.addressKind);
    engine_.wipe(wallet_);
    if(engine_.restore(imported.mnemonic,imported.wordCount,importedKind,
                       imported.passphrase,wallet_) &&
       engine_.accountXprv(wallet_,imported.passphrase,derivedXprv,sizeof(derivedXprv)) &&
       strcmp(imported.addressType,addressKindName(importedKind))==0 &&
       strcmp(imported.derivationPath,wallet_.path)==0 &&
       strcmp(imported.address,wallet_.address)==0 &&
       strcmp(imported.accountXpub,wallet_.accountXpub)==0 &&
       strcmp(imported.accountXprv,derivedXprv)==0 &&
       strcmp(imported.privateWif,wallet_.privateWif)==0 &&
       strcmp(imported.receiveDescriptor,wallet_.watchDescriptor)==0) {
      strlcpy(passphrase_,imported.passphrase,sizeof(passphrase_));
      words_=imported.wordCount;
      kind_=importedKind;
      mnemonicPage_=0;
      qrContent_=QrContent::Address;
      loadedWallet_=true;
      startFileSession(fileFingerprint,importBaseName_);
      ok=true;
      protectedSession_=true;
      revokeAccess();
      secureZero(importStatus_,sizeof(importStatus_));
    } else {
      engine_.wipe(wallet_);
      secureZero(passphrase_,sizeof(passphrase_));
      strlcpy(importStatus_,"Fichier authentifié mais portefeuille incohérent.",sizeof(importStatus_));
    }
  } else {
    switch(result) {
      case AuroraWalletReadResult::Ok: break;
      case AuroraWalletReadResult::InvalidName:
        strlcpy(importStatus_,"Nom de fichier invalide.",sizeof(importStatus_)); break;
      case AuroraWalletReadResult::WeakPassword:
        strlcpy(importStatus_,"Mot de passe trop court ou invalide.",sizeof(importStatus_)); break;
      case AuroraWalletReadResult::NoCard:
        strlcpy(importStatus_,"Carte microSD absente ou illisible.",sizeof(importStatus_)); break;
      case AuroraWalletReadResult::NotFound:
        strlcpy(importStatus_,"Fichier .aurora introuvable.",sizeof(importStatus_)); break;
      case AuroraWalletReadResult::OpenFailed:
        strlcpy(importStatus_,"Impossible d'ouvrir le fichier.",sizeof(importStatus_)); break;
      case AuroraWalletReadResult::InvalidFormat:
        strlcpy(importStatus_,"Format Aurora Wallet invalide.",sizeof(importStatus_)); break;
      case AuroraWalletReadResult::MemoryFailed:
        strlcpy(importStatus_,"Mémoire insuffisante pour ouvrir le fichier.",sizeof(importStatus_)); break;
      case AuroraWalletReadResult::AuthenticationFailed:
        strlcpy(importStatus_,"Mot de passe incorrect ou fichier endommagé.",sizeof(importStatus_)); break;
      case AuroraWalletReadResult::ReadFailed:
        strlcpy(importStatus_,"Lecture incomplète du fichier.",sizeof(importStatus_)); break;
    }
  }
  secureZero(derivedXprv,sizeof(derivedXprv));
  wipeAuroraWalletData(imported);
  secureZero(fileFingerprint,sizeof(fileFingerprint));
  secureZero(filePassword_,sizeof(filePassword_));
  if(!ok) {
    engine_.wipe(wallet_);
    dropPrivateState();
    secureZero(sessionFingerprint_,sizeof(sessionFingerprint_));
    secureZero(sessionBaseName_,sizeof(sessionBaseName_));
    fileSession_=sessionHasPassphrase_=false;
    protectedSession_=loadedWallet_=false;
  }
  return ok;
}

AuroraUI::Access AuroraUI::accessFor(Screen screen) const {
  switch(screen) {
    case Screen::Mnemonic: case Screen::Verify: return Access::Words;
    case Screen::PassphraseReveal: return Access::Passphrase;
    case Screen::UmbrelQr: return Access::PrivateQr;
    case Screen::Qr: return qrContent_==QrContent::PrivateKey?Access::PrivateQr:Access::None;
    case Screen::ExportWarning: case Screen::ExportName: case Screen::ExportPassword: return Access::Export;
    case Screen::FileProcessing: return fileOperation_==FileOperation::Export?Access::Export:Access::None;
    default: return Access::None;
  }
}

bool AuroraUI::authorized(Access access) const {
  if(!fileSession_) return !protectedSession_;
  return fileSession_ && privateLoaded_ && access!=Access::None &&
      access_==access && millis()-accessGrantedMs_ < accessDurationMs(access);
}

void AuroraUI::wipeFileCredentials() {
  secureZero(filePassword_,sizeof(filePassword_));
}

bool AuroraUI::hasPassphrase() const {
  if(fileSession_) return sessionHasPassphrase_;
  return passphrase_[0]!=0;
}

void AuroraUI::revokeAccess() {
  if(fileSession_) {
    dropPrivateState();
    secureZero(filePassword_,sizeof(filePassword_));
  }
  access_=requestedAccess_=Access::None;
  accessGrantedMs_=0;
}

void AuroraUI::dropPrivateState() {
  secureZero(wallet_.mnemonic,sizeof(wallet_.mnemonic));
  secureZero(wallet_.privateWif,sizeof(wallet_.privateWif));
  secureZero(wallet_.privateDescriptor,sizeof(wallet_.privateDescriptor));
  secureZero(passphrase_,sizeof(passphrase_));
  secureZero(mixedEntropy_,sizeof(mixedEntropy_));
  secureZero(restoreWords_,sizeof(restoreWords_));
  secureZero(restoreMnemonic_,sizeof(restoreMnemonic_));
  secureZero(restoreSuggestions_,sizeof(restoreSuggestions_));
  secureZero(verifySuggestions_,sizeof(verifySuggestions_));
  secureZero(verifyIndex_,sizeof(verifyIndex_));
  secureZero(umbrelRootXprv_,sizeof(umbrelRootXprv_));
  entropy_.wipeSecretsWithoutHardware();
  privateLoaded_=false;
}

void AuroraUI::startFileSession(const uint8_t fingerprint[32],const char *baseName) {
  sessionHasPassphrase_=passphrase_[0]!=0;
  memcpy(sessionFingerprint_,fingerprint,sizeof(sessionFingerprint_));
  strlcpy(sessionBaseName_,baseName,sizeof(sessionBaseName_));
  fileSession_=true; manualRestore_=false;
  dropPrivateState();
}

bool AuroraUI::loadPrivateWallet() {
  AuroraWalletData imported{};
  char derivedXprv[128]{};
  bool ok=false;
  const AuroraWalletReadResult result=readAuroraWalletFileChecked(
      sessionBaseName_,filePassword_,imported,nullptr,sessionFingerprint_);
  secureZero(filePassword_,sizeof(filePassword_));
  if(result==AuroraWalletReadResult::Ok) {
    const AddressKind importedKind=static_cast<AddressKind>(imported.addressKind);
    engine_.wipe(wallet_);
    ok=engine_.restore(imported.mnemonic,imported.wordCount,importedKind,
                       imported.passphrase,wallet_) &&
       engine_.accountXprv(wallet_,imported.passphrase,derivedXprv,sizeof(derivedXprv)) &&
       strcmp(imported.addressType,addressKindName(importedKind))==0 &&
       strcmp(imported.derivationPath,wallet_.path)==0 &&
       strcmp(imported.address,wallet_.address)==0 &&
       strcmp(imported.accountXpub,wallet_.accountXpub)==0 &&
       strcmp(imported.accountXprv,derivedXprv)==0 &&
       strcmp(imported.privateWif,wallet_.privateWif)==0 &&
       strcmp(imported.receiveDescriptor,wallet_.watchDescriptor)==0;
    if(ok) strlcpy(passphrase_,imported.passphrase,sizeof(passphrase_));
  }
  secureZero(derivedXprv,sizeof(derivedXprv));
  wipeAuroraWalletData(imported);
  privateLoaded_=ok;
  if(!ok) {
    dropPrivateState();
    strlcpy(importStatus_,"Mot de passe incorrect, fichier modifié ou illisible.",sizeof(importStatus_));
  } else secureZero(importStatus_,sizeof(importStatus_));
  return ok;
}

void AuroraUI::submitPrivatePassword() {
  if(!fileSession_ || !filePasswordArea_ || requestedAccess_==Access::None) {
    closeSession(); return;
  }
  if(!ensureSd(Screen::PrivatePassword)) return;
  const char *source=lv_textarea_get_text(filePasswordArea_);
  const size_t length=source?strlen(source):0;
  secureZero(filePassword_,sizeof(filePassword_));
  if(length<AURORA_WALLET_MIN_PASSWORD_LENGTH || length>=sizeof(filePassword_)) {
    strlcpy(importStatus_,"Mot de passe : 12 caractères minimum.",sizeof(importStatus_));
    show(Screen::PrivatePassword); return;
  }
  strlcpy(filePassword_,source,sizeof(filePassword_));
  wipeObjectTree(filePasswordArea_);
  fileOperation_=FileOperation::PrivateRead;
  show(Screen::FileProcessing);
  if(screen_==Screen::FileProcessing) fileOperationDueMs_=millis()+100;
}


void AuroraUI::emergencyWipeSecrets() noexcept {
  // Fixed, owned storage only. In particular, cameraPixels_ may be caught
  // between free() and pointer reset on another stopped task: do not chase it.
  // LVGL allocations, task stacks, DMA and physical caches are separate duties
  // of the platform fail-closed handler, not a guarantee supplied by this API.
  secureZero(&wallet_,sizeof(wallet_));
  secureZero(passphrase_,sizeof(passphrase_));
  secureZero(filePassword_,sizeof(filePassword_));
  secureZero(mixedEntropy_,sizeof(mixedEntropy_));
  secureZero(restoreWords_,sizeof(restoreWords_));
  secureZero(restoreSuggestions_,sizeof(restoreSuggestions_));
  secureZero(restoreMnemonic_,sizeof(restoreMnemonic_));
  secureZero(restoreStatus_,sizeof(restoreStatus_));
  secureZero(umbrelRootXprv_,sizeof(umbrelRootXprv_));
  secureZero(exportStatus_,sizeof(exportStatus_));
  secureZero(importStatus_,sizeof(importStatus_));
  secureZero(passwordStatus_,sizeof(passwordStatus_));
  secureZero(exportBaseName_,sizeof(exportBaseName_));
  secureZero(importBaseName_,sizeof(importBaseName_));
  secureZero(verifyIndex_,sizeof(verifyIndex_));
  secureZero(verifySuggestions_,sizeof(verifySuggestions_));
  secureZero(auroraFileOptions_,sizeof(auroraFileOptions_));
  secureZero(entropyPreviewText_,sizeof(entropyPreviewText_));
  secureZero(sessionFingerprint_,sizeof(sessionFingerprint_));
  secureZero(sessionBaseName_,sizeof(sessionBaseName_));
  fileSession_=privateLoaded_=sessionHasPassphrase_=false;
  entropy_.wipeSecretsWithoutHardware();
  revokeAccess();
  sensitiveStateActive_=protectedSession_=false;
  visibleSecret_=Access::None; visibleSecretStartedMs_=0;
  entropyCollected_=exportSucceeded_=loadedWallet_=manualRestore_=umbrelRecovery_=false;
  entropyReadyPending_=entropyFailurePending_=false;
  generationDueMs_=entropyCompleteDueMs_=fileOperationDueMs_=0;
  fileOperation_=FileOperation::None;
#if defined(AURORA_BOARD_P4)
  afterSensorStop_=Screen::Mode;
#endif
  afterSd_=Screen::Mode;
  afterAuthentication_=Screen::Info;
  umbrelBirthdayDays_=0;
  restoreWordIndex_=restoreSuggestionCount_=verifyActiveIndex_=verifySuggestionCount_=mnemonicPage_=0;
  auroraFileCount_=0;
}

void AuroraUI::wipeSession() {
  // One inventory of owned secret buffers is shared by normal close, startup
  // and terminal failure. Do not add a new secret only to one cleanup path.
  emergencyWipeSecrets();
  entropy_.cancel();
  exportFormat_=WalletExportFormat::AuroraWallet;
  umbrelResult_=AezeedResult::Ok;
  qrContent_=QrContent::Address;
  strlcpy(exportBaseName_,"aurora",sizeof(exportBaseName_));
  strlcpy(importBaseName_,"aurora",sizeof(importBaseName_));
}

void AuroraUI::closeSession() {
  show(Screen::Mode);
}

void AuroraUI::buildWipe() {
  header("Effacement terminé"); lv_obj_t *ok=label(root_,"OK",&aurora_font_20); lv_obj_set_style_text_color(ok,ORANGE,0); AuroraLayout::align(ok,LV_ALIGN_CENTER,0,-40);
  lv_obj_t *msg=explanation(root_,"Les tampons sensibles de la session\nont été écrasés en mémoire vive.",&aurora_font_12); lv_obj_set_style_text_align(msg,LV_TEXT_ALIGN_CENTER,0); AuroraLayout::align(msg,LV_ALIGN_CENTER,0,0);
  lv_obj_t *b=button(root_,"RETOUR À L'ACCUEIL",event,190); lv_obj_set_user_data(b,(void*)BACK_MODE); AuroraLayout::align(b,LV_ALIGN_BOTTOM_MID,0,-18);
}

void AuroraUI::event(lv_event_t *e) {
  if(!g_ui)return;
#if defined(AURORA_BOARD_P4)
  if(g_ui->sensorStopPending_) return;
#endif
  lv_obj_t *target=static_cast<lv_obj_t *>(lv_event_get_target(e));
  if(lv_obj_has_state(target,LV_STATE_DISABLED)) return;
  if(lv_event_get_code(e)==LV_EVENT_FOCUSED &&
     g_ui->keyboard_ && lv_obj_check_type(target,&lv_textarea_class)) {
    lv_keyboard_set_textarea(g_ui->keyboard_,target);
    if(g_ui->screen_==Screen::Verify) {
      for(uint8_t i=0;i<3;++i)
        if(target==g_ui->verifyArea_[i]) g_ui->verifyActiveIndex_=i;
      g_ui->updateVerifySuggestions();
    }
    return;
  }
  uintptr_t a=(uintptr_t)lv_obj_get_user_data(target);
  if(lv_event_get_code(e)==LV_EVENT_READY || lv_event_get_code(e)==LV_EVENT_CANCEL)
    a=(uintptr_t)lv_event_get_user_data(e);
  switch((Action)a){
    case START: if(!g_ui->selfTestPending_) g_ui->show(Screen::Mode); break;
    case OPEN_WALLET:
      g_ui->wipeSession();
      g_ui->engine_.wipe(g_ui->wallet_); secureZero(g_ui->passphrase_,sizeof(g_ui->passphrase_));
      g_ui->loadedWallet_=false; g_ui->manualRestore_=false;
      secureZero(g_ui->importStatus_,sizeof(g_ui->importStatus_));
      g_ui->show(Screen::ImportName); break;
    case NEW_WALLET:
      g_ui->wipeSession();
      g_ui->engine_.wipe(g_ui->wallet_); secureZero(g_ui->passphrase_,sizeof(g_ui->passphrase_));
      g_ui->loadedWallet_=false; g_ui->manualRestore_=false;
      secureZero(g_ui->exportStatus_,sizeof(g_ui->exportStatus_));
      g_ui->show(Screen::Setup); break;
    case RESTORE_WALLET:
      g_ui->wipeSession();
      g_ui->engine_.wipe(g_ui->wallet_); secureZero(g_ui->passphrase_,sizeof(g_ui->passphrase_));
      g_ui->loadedWallet_=false; g_ui->manualRestore_=true; g_ui->kind_=AddressKind::NativeSegwit;
      g_ui->words_=12; g_ui->restoreWordIndex_=0; g_ui->restoreSuggestionCount_=0;
      secureZero(g_ui->restoreWords_,sizeof(g_ui->restoreWords_));
      secureZero(g_ui->restoreSuggestions_,sizeof(g_ui->restoreSuggestions_));
      secureZero(g_ui->restoreMnemonic_,sizeof(g_ui->restoreMnemonic_));
      secureZero(g_ui->restoreStatus_,sizeof(g_ui->restoreStatus_));
      g_ui->show(Screen::RestoreSetup); break;
    case RECOVER_UMBREL:
      g_ui->wipeSession();
      g_ui->umbrelRecovery_=true; g_ui->words_=24; g_ui->restoreWordIndex_=0;
      g_ui->umbrelResult_=AezeedResult::Ok;
      g_ui->show(Screen::UmbrelWarning); break;
    case TO_IMPORT_PASSWORD: {
      if(!g_ui->importFileDropdown_ || !g_ui->auroraFileCount_) break;
      lv_dropdown_get_selected_str(g_ui->importFileDropdown_,g_ui->importBaseName_,
                                   sizeof(g_ui->importBaseName_));
      secureZero(g_ui->importStatus_,sizeof(g_ui->importStatus_));
      g_ui->show(Screen::ImportPassword); break;
    }
    case REFRESH_AURORA_FILES:
      secureZero(g_ui->importStatus_,sizeof(g_ui->importStatus_));
      g_ui->show(Screen::ImportName); break;
    case RESTORE_SETUP_CONTINUE:
      g_ui->restoreWordIndex_=0; secureZero(g_ui->restoreStatus_,sizeof(g_ui->restoreStatus_));
      g_ui->show(Screen::RestoreWords); break;
    case UMBREL_CONTINUE:
      g_ui->words_=24; g_ui->restoreWordIndex_=0;
      secureZero(g_ui->restoreStatus_,sizeof(g_ui->restoreStatus_));
      g_ui->show(Screen::RestoreWords); break;
    case RESTORE_WORD_CHANGED:
      g_ui->updateRestoreSuggestions(); break;
    case RESTORE_WORD_READY: {
      const char *word=g_ui->restoreWordArea_?lv_textarea_get_text(g_ui->restoreWordArea_):"";
      g_ui->acceptRestoreWord(word); break;
    }
    case RESTORE_SUGGESTION_0: case RESTORE_SUGGESTION_1: case RESTORE_SUGGESTION_2: {
      const uint8_t index=static_cast<uint8_t>(a-RESTORE_SUGGESTION_0);
      if(index<g_ui->restoreSuggestionCount_)
        g_ui->acceptRestoreWord(g_ui->restoreSuggestions_[index]);
      break;
    }
    case VERIFY_WORD_CHANGED:
      g_ui->updateVerifySuggestions(); break;
    case VERIFY_SUGGESTION_0: case VERIFY_SUGGESTION_1: case VERIFY_SUGGESTION_2:
      g_ui->acceptVerifySuggestion(static_cast<uint8_t>(a-VERIFY_SUGGESTION_0)); break;
    case RESTORE_DERIVE: {
      if(!g_ui->confirmPassphrase()) break;
      g_ui->show(Screen::Restoring);
      if(g_ui->screen_==Screen::Restoring)
        g_ui->generationDueMs_=millis()+100;
      break;
    }
    case UMBREL_DECODE: {
      if(!g_ui->passArea_) break;
      char *source=const_cast<char *>(lv_textarea_get_text(g_ui->passArea_));
      const size_t length=source?strlen(source):0;
      secureZero(g_ui->passphrase_,sizeof(g_ui->passphrase_));
      if(length>=sizeof(g_ui->passphrase_)) {
        strlcpy(g_ui->restoreStatus_,"Passphrase trop longue (63 caractères maximum).",sizeof(g_ui->restoreStatus_));
        if(source) secureZero(source,length); g_ui->show(Screen::UmbrelPassphrase); break;
      }
      if(source) { strlcpy(g_ui->passphrase_,source,sizeof(g_ui->passphrase_)); secureZero(source,length); }
      secureZero(g_ui->restoreStatus_,sizeof(g_ui->restoreStatus_));
      g_ui->show(Screen::UmbrelProcessing);
      if(g_ui->screen_==Screen::UmbrelProcessing)
        g_ui->generationDueMs_=millis()+100;
      break;
    }
    case UMBREL_SHOW_XPRV: g_ui->show(Screen::UmbrelQr); break;
    case UMBREL_RESULT_BACK:
      g_ui->show(g_ui->screen_==Screen::RestoreWords?Screen::UmbrelWarning:Screen::UmbrelResult); break;
    case RESTORE_DERIVATION_CHANGED: {
      if(!g_ui->restoreDerivationDropdown_) break;
      const uint16_t selected=lv_dropdown_get_selected(g_ui->restoreDerivationDropdown_);
      if(selected<=static_cast<uint16_t>(AddressKind::Taproot) &&
         g_ui->rederiveManualWallet(static_cast<AddressKind>(selected))) {
        g_ui->qrContent_=QrContent::Address;
        g_ui->show(Screen::Qr);
      }
      break;
    }
    case UNLOCK_WALLET: {
      if(!g_ui->filePasswordArea_ || !g_ui->ensureSd(Screen::ImportPassword)) break;
      char *source=const_cast<char *>(lv_textarea_get_text(g_ui->filePasswordArea_));
      const size_t length=source?strlen(source):0;
      if(length<AURORA_WALLET_MIN_PASSWORD_LENGTH || length>=sizeof(g_ui->filePassword_)) {
        strlcpy(g_ui->importStatus_,"Mot de passe : 12 caractères minimum.",sizeof(g_ui->importStatus_));
        if(source) secureZero(source,length);
        g_ui->show(Screen::ImportPassword);
      } else {
        strlcpy(g_ui->filePassword_,source,sizeof(g_ui->filePassword_));
        secureZero(source,length); secureZero(g_ui->importStatus_,sizeof(g_ui->importStatus_));
        g_ui->fileOperation_=FileOperation::Import; g_ui->show(Screen::FileProcessing);
        if(g_ui->screen_==Screen::FileProcessing) g_ui->fileOperationDueMs_=millis()+100;
      }
      break;
    }
    case TO_PASSPHRASE: g_ui->show(Screen::Entropy); break;
    case TO_ENTROPY: {
      if(!g_ui->entropyCollected_ || !g_ui->confirmPassphrase()) break;
      g_ui->show(Screen::Generating);
      if(g_ui->screen_==Screen::Generating)
        g_ui->generationDueMs_=millis()+100;
      break;
    }
    case NEXT_VERIFY: g_ui->show(Screen::Verify); break;
    case CHECK_VERIFY: if(g_ui->verifyWords())g_ui->show(Screen::Info); break;
    case SHOW_LOADED_PASSPHRASE:
      if(g_ui->hasPassphrase())
        g_ui->show(Screen::PassphraseReveal);
      else
        g_ui->show(Screen::Info);
      break;
    case TO_INFO: g_ui->qrContent_=QrContent::Address; g_ui->show(Screen::Info); break;
    case TO_QR_ADDRESS: g_ui->qrContent_=QrContent::Address; g_ui->show(Screen::Qr); break;
    case TO_QR_PUBLIC: g_ui->qrContent_=QrContent::AccountXpub; g_ui->show(Screen::Qr); break;
    case REVEAL_PRIVATE: g_ui->qrContent_=QrContent::PrivateKey; g_ui->show(Screen::Qr); break;
    case TO_BACKUP: g_ui->show(Screen::Backup); break;
    case EXPORT_ELECTRUM:
      g_ui->exportFormat_=WalletExportFormat::ElectrumPrivate;
      secureZero(g_ui->exportStatus_,sizeof(g_ui->exportStatus_));
      if(g_ui->wallet_.kind==AddressKind::Taproot) {
        strlcpy(g_ui->exportStatus_,"Electrum indisponible pour Taproot / BIP86.",sizeof(g_ui->exportStatus_));
        g_ui->show(Screen::Backup);
      } else {
        g_ui->show(Screen::ExportWarning);
      }
      break;
    case EXPORT_AURORA:
      g_ui->exportFormat_=WalletExportFormat::AuroraWallet;
      secureZero(g_ui->exportStatus_,sizeof(g_ui->exportStatus_));
      g_ui->show(Screen::ExportWarning); break;
    case CONFIRM_PRIVATE: g_ui->show(Screen::ExportName); break;
    case SAVE_EXPORT: {
      const char *source=lv_textarea_get_text(g_ui->exportNameArea_);
      strlcpy(g_ui->exportBaseName_,source ? source : "",sizeof(g_ui->exportBaseName_));
      if(g_ui->exportFormat_==WalletExportFormat::AuroraWallet) {
        secureZero(g_ui->passwordStatus_,sizeof(g_ui->passwordStatus_));
        g_ui->show(Screen::ExportPassword);
      } else {
        g_ui->fileOperation_=FileOperation::Export; g_ui->show(Screen::FileProcessing);
        if(g_ui->screen_==Screen::FileProcessing) g_ui->fileOperationDueMs_=millis()+100;
      }
      break;
    }
    case SAVE_EXPORT_PASSWORD: {
      if(!g_ui->filePasswordArea_ || !g_ui->filePasswordConfirmArea_ ||
         !g_ui->ensureSd(Screen::ExportPassword)) break;
      char *first=const_cast<char *>(lv_textarea_get_text(g_ui->filePasswordArea_));
      char *second=const_cast<char *>(lv_textarea_get_text(g_ui->filePasswordConfirmArea_));
      const size_t firstLength=first?strlen(first):0;
      const size_t secondLength=second?strlen(second):0;
      if(firstLength<AURORA_WALLET_MIN_PASSWORD_LENGTH ||
         firstLength>=sizeof(g_ui->filePassword_)) {
        strlcpy(g_ui->passwordStatus_,"Utilisez au moins 12 caractères.",sizeof(g_ui->passwordStatus_));
      } else if(!second || strcmp(first,second)!=0) {
        strlcpy(g_ui->passwordStatus_,"Les deux mots de passe sont différents.",sizeof(g_ui->passwordStatus_));
      } else {
        strlcpy(g_ui->filePassword_,first,sizeof(g_ui->filePassword_));
        secureZero(g_ui->passwordStatus_,sizeof(g_ui->passwordStatus_));
      }
      if(first) secureZero(first,firstLength);
      if(second) secureZero(second,secondLength);
      if(g_ui->passwordStatus_[0]) {
        g_ui->show(Screen::ExportPassword);
      } else {
        g_ui->fileOperation_=FileOperation::Export;
        g_ui->show(Screen::FileProcessing);
        if(g_ui->screen_==Screen::FileProcessing) g_ui->fileOperationDueMs_=millis()+100;
      }
      break;
    }
    case DO_WIPE: g_ui->show(Screen::Wipe); break;
    case PRIVATE_PASSWORD_CHECK: g_ui->submitPrivatePassword(); break;
    case PRIVATE_PASSWORD_CANCEL:
      g_ui->revokeAccess();
      g_ui->fileOperation_=FileOperation::None; g_ui->fileOperationDueMs_=0;
      g_ui->qrContent_=QrContent::Address; g_ui->show(Screen::Info); break;
    case LOCK_SESSION: g_ui->closeSession(); break;
    case RETRY_SD: g_ui->show(g_ui->afterSd_); break;
    case SHOW_WORDS:
      g_ui->mnemonicPage_=0;
      g_ui->show(Screen::Mnemonic); break;
    case BACK_MODE:
      g_ui->engine_.wipe(g_ui->wallet_); secureZero(g_ui->passphrase_,sizeof(g_ui->passphrase_));
      secureZero(g_ui->filePassword_,sizeof(g_ui->filePassword_));
      secureZero(g_ui->restoreWords_,sizeof(g_ui->restoreWords_));
      secureZero(g_ui->restoreSuggestions_,sizeof(g_ui->restoreSuggestions_));
      secureZero(g_ui->restoreMnemonic_,sizeof(g_ui->restoreMnemonic_));
      secureZero(g_ui->restoreStatus_,sizeof(g_ui->restoreStatus_));
      g_ui->loadedWallet_=false; g_ui->manualRestore_=false;
      g_ui->show(Screen::Mode); break;
    case BACK_SETUP: g_ui->show(Screen::Setup); break;
    case BACK_PASSPHRASE: g_ui->show(Screen::Passphrase); break;
    case BACK_ENTROPY:
      g_ui->engine_.wipe(g_ui->wallet_); secureZero(g_ui->mixedEntropy_,sizeof(g_ui->mixedEntropy_));
      g_ui->show(Screen::Entropy); break;
    case BACK_MNEMONIC: g_ui->show(Screen::Mnemonic); break;
    case BACK_VERIFY: g_ui->show(Screen::Verify); break;
    case BACK_INFO: g_ui->show(Screen::Info); break;
    case BACK_BACKUP:
      g_ui->wipeFileCredentials();
      g_ui->show(Screen::Backup); break;
    case BACK_IMPORT_NAME:
      secureZero(g_ui->filePassword_,sizeof(g_ui->filePassword_));
      g_ui->show(Screen::ImportName); break;
    case BACK_EXPORT_NAME:
      secureZero(g_ui->filePassword_,sizeof(g_ui->filePassword_));
      g_ui->show(Screen::ExportName); break;
    case BACK_MODE_WIPE: g_ui->show(Screen::Wipe); break;
    case BACK_RESTORE_SETUP:
      g_ui->restoreWordIndex_=0; secureZero(g_ui->restoreStatus_,sizeof(g_ui->restoreStatus_));
      // Editing the word count invalidates the old concatenation. The accepted
      // per-word inputs remain available until the user changes that count.
      secureZero(g_ui->restoreMnemonic_,sizeof(g_ui->restoreMnemonic_));
      g_ui->show(Screen::RestoreSetup); break;
    case BACK_RESTORE_WORDS:
      g_ui->restoreWordIndex_=g_ui->words_?g_ui->words_-1:0;
      g_ui->show(Screen::RestoreWords); break;
    case RESTORE_WORD_BACK:
      if(g_ui->restoreWordIndex_>0) --g_ui->restoreWordIndex_;
      secureZero(g_ui->restoreStatus_,sizeof(g_ui->restoreStatus_));
      g_ui->show(Screen::RestoreWords); break;
    case MNEMONIC_PREVIOUS:
      if(g_ui->mnemonicPage_>0) --g_ui->mnemonicPage_;
      g_ui->show(Screen::Mnemonic); break;
    case MNEMONIC_NEXT:
      if((g_ui->mnemonicPage_+1)*12<g_ui->words_) ++g_ui->mnemonicPage_;
      g_ui->show(Screen::Mnemonic); break;
    case RETRY_ENTROPY: g_ui->show(Screen::Entropy); break;
    case WORD_12: case WORD_15: case WORD_18: case WORD_21: case WORD_24:
      g_ui->words_=12+3*((uint8_t)a-WORD_12);
      if(g_ui->screen_==Screen::RestoreSetup) {
        g_ui->restoreWordIndex_=0;
        secureZero(g_ui->restoreWords_,sizeof(g_ui->restoreWords_));
        secureZero(g_ui->restoreMnemonic_,sizeof(g_ui->restoreMnemonic_));
        secureZero(g_ui->restoreStatus_,sizeof(g_ui->restoreStatus_));
        g_ui->show(Screen::RestoreSetup);
      } else g_ui->show(Screen::Setup);
      break;
    case TYPE_LEGACY: case TYPE_NESTED: case TYPE_NATIVE: case TYPE_TAPROOT:
      g_ui->kind_=(AddressKind)((uint8_t)a-TYPE_LEGACY); g_ui->show(Screen::Setup); break;
    default: break;
  }
}
