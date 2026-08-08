#include "ui.h"
#include "hardware_rng.h"
#include "bitcoin_logo.h"
#include "sd_export.h"
#include "secure_memory.h"
#include "splash_img.h"
#include "version.h"

namespace {
AuroraUI *g_ui = nullptr;
constexpr lv_color_t ORANGE = LV_COLOR_MAKE(0xF7, 0x93, 0x1A);
constexpr lv_color_t DANGER = LV_COLOR_MAKE(0xFF, 0x3B, 0x30);
constexpr lv_color_t BLACK = LV_COLOR_MAKE(0x08, 0x09, 0x0B);
constexpr lv_color_t PANEL = LV_COLOR_MAKE(0x16, 0x18, 0x1D);
constexpr lv_color_t MUTED = LV_COLOR_MAKE(0x9A, 0xA0, 0xAA);
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

enum Action : uint8_t { START, OPEN_WALLET, NEW_WALLET, RESTORE_WALLET,
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
                        VERIFY_SUGGESTION_1, VERIFY_SUGGESTION_2 };

void styleRoot(lv_obj_t *o) {
  lv_obj_set_style_bg_color(o, BLACK, 0); lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(o, lv_color_white(), 0); lv_obj_set_style_border_width(o, 0, 0);
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
  if (lv_obj_check_type(object, &lv_canvas_class)) {
    lv_img_dsc_t *image = lv_canvas_get_img(object);
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
      secureZero(filePassword_, sizeof(filePassword_));
      Serial.printf("AURORA export SD : %lu ms\n", static_cast<unsigned long>(millis() - started));
      show(Screen::Backup);
    } else if (operation == FileOperation::Import) {
      const bool imported = performWalletImport();
      secureZero(filePassword_, sizeof(filePassword_));
      Serial.printf("AURORA lecture SD : %lu ms\n", static_cast<unsigned long>(millis() - started));
      show(imported ? Screen::Mnemonic : Screen::ImportPassword);
    }
    return;
  }
  if (entropyFailurePending_) {
    entropyFailurePending_ = false;
    show(Screen::GenerationError);
    return;
  }
  if (entropyReadyPending_ && screen_ == Screen::Entropy) {
    entropyReadyPending_ = false;
    show(Screen::Generating);
    generationDueMs_ = millis() + 100;
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
      show(Screen::Qr);
    } else {
      restoreWordIndex_ = words_ ? words_ - 1 : 0;
      show(Screen::RestoreWords);
    }
  }
}

void AuroraUI::clear() {
  if (root_) {
    wipeObjectTree(root_);
    lv_obj_del(root_);
  }
  root_ = lv_obj_create(lv_scr_act()); lv_obj_set_size(root_, 320, 240); lv_obj_set_pos(root_, 0, 0);
  lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE); styleRoot(root_);
  passArea_ = entropyBar_ = entropyStatus_ = keyboard_ = exportNameArea_ =
      importFileDropdown_ = restoreWordArea_ = restoreDerivationDropdown_ =
      filePasswordArea_ = filePasswordConfirmArea_ = nullptr;
  memset(verifyArea_, 0, sizeof(verifyArea_));
  memset(verifySuggestionButtons_, 0, sizeof(verifySuggestionButtons_));
  memset(restoreSuggestionButtons_, 0, sizeof(restoreSuggestionButtons_));
}

lv_obj_t *AuroraUI::label(lv_obj_t *p, const char *text, const lv_font_t *font) {
  lv_obj_t *l = lv_label_create(p); lv_label_set_text(l, text); lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, lv_color_white(), 0); return l;
}

lv_obj_t *AuroraUI::button(lv_obj_t *p, const char *text, lv_event_cb_t cb, int w) {
  lv_obj_t *b = lv_btn_create(p); lv_obj_set_size(b, w, 34); lv_obj_set_style_radius(b, 8, 0);
  lv_obj_set_style_bg_color(b, ORANGE, 0); lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *l = label(b, text, &aurora_font_12); lv_obj_set_style_text_color(l, BLACK, 0); lv_obj_center(l);
  return b;
}

lv_obj_t *AuroraUI::header(const char *title, const char *step) {
  lv_obj_t *titleLabel = nullptr;
  if (step) {
    lv_obj_t *badge = lv_obj_create(root_); lv_obj_set_pos(badge, 9, 3); lv_obj_set_size(badge, 27, 27);
    lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0); lv_obj_set_style_bg_color(badge, ORANGE, 0);
    lv_obj_set_style_border_width(badge, 0, 0); lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    char n[2] = {step[0], 0}; lv_obj_t *num = label(badge, n, &aurora_font_14);
    lv_obj_set_style_text_color(num, BLACK, 0); lv_obj_center(num);
    titleLabel = label(root_, title, &aurora_font_14); lv_obj_set_pos(titleLabel, 44, 9);
    lv_obj_t *s = label(root_, step, &aurora_font_10); lv_obj_set_style_text_color(s, MUTED, 0); lv_obj_set_pos(s, 247, 10);
    Action backAction = BACK_MODE;
    switch(screen_) {
      case Screen::Setup: backAction=BACK_MODE; break;
      case Screen::Passphrase: backAction=BACK_SETUP; break;
      case Screen::Entropy: backAction=BACK_PASSPHRASE; break;
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
    lv_obj_t *back=lv_btn_create(root_); lv_obj_set_pos(back,276,3); lv_obj_set_size(back,28,28);
    lv_obj_set_style_radius(back,7,0); lv_obj_set_style_bg_color(back,PANEL,0);
    lv_obj_set_style_border_color(back,ORANGE,0); lv_obj_set_style_border_width(back,1,0);
    lv_obj_set_style_shadow_width(back,0,0); lv_obj_set_user_data(back,(void*)backAction);
    lv_obj_add_event_cb(back,event,LV_EVENT_CLICKED,nullptr);
    lv_obj_t *arrow=label(back,"<",&aurora_font_16); lv_obj_set_style_text_color(arrow,ORANGE,0); lv_obj_center(arrow);
  } else {
    lv_obj_t *brand = label(root_, "AURORA", &aurora_font_14); lv_obj_set_pos(brand, 10, 9);
    lv_obj_set_style_text_color(brand, ORANGE, 0);
    titleLabel = label(root_, title, &aurora_font_14); lv_obj_set_pos(titleLabel, 105, 9);
  }
  lv_obj_t *line = lv_obj_create(root_); lv_obj_set_pos(line, 10, 34); lv_obj_set_size(line, 300, 1);
  lv_obj_set_style_bg_color(line, LV_COLOR_MAKE(0x38,0x3B,0x42),0); lv_obj_set_style_border_width(line,0,0);
  return titleLabel;
}

void AuroraUI::show(Screen s) {
  if (screen_ == Screen::Entropy && s != Screen::Entropy) entropy_.cancel();
  screen_ = s; clear();
  switch (s) {
    case Screen::Splash: buildSplash(); break; case Screen::Mode: buildMode(); break;
    case Screen::ImportName: buildImportName(); break;
    case Screen::ImportPassword: buildImportPassword(); break;
    case Screen::RestoreSetup: buildRestoreSetup(); break;
    case Screen::RestoreWords: buildRestoreWords(); break;
    case Screen::RestorePassphrase: buildRestorePassphrase(); break;
    case Screen::Restoring: buildRestoring(); break;
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
  }
}

void AuroraUI::buildSplash() {
  lv_obj_t *bg = lv_img_create(root_); lv_img_set_src(bg, &aurora_splash); lv_obj_set_pos(bg, 0, 0);
  lv_obj_t *shade = lv_obj_create(root_); lv_obj_set_pos(shade, 0, 142); lv_obj_set_size(shade, 320, 98);
  lv_obj_set_style_bg_color(shade, BLACK, 0); lv_obj_set_style_bg_opa(shade, LV_OPA_50, 0);
  lv_obj_set_style_border_width(shade, 0, 0); lv_obj_clear_flag(shade, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *b = button(root_, "CONTINUER", event, 140); lv_obj_set_user_data(b, (void *)START);
  lv_obj_align(b, LV_ALIGN_BOTTOM_MID, 0, -51);
  lv_obj_t *name = label(root_, "A U R O R A", &aurora_font_20);
  lv_obj_align(name, LV_ALIGN_BOTTOM_MID, 0, -19);
  lv_obj_t *sub = label(root_, "SEED GENERATOR", &aurora_font_10);
  lv_obj_set_style_text_color(sub, ORANGE, 0); lv_obj_align(sub, LV_ALIGN_BOTTOM_MID, 0, -4);
  lv_obj_t *version = label(root_, "v" AURORA_FIRMWARE_VERSION, &aurora_font_10);
  lv_obj_set_style_text_color(version, lv_color_white(), 0);
  lv_obj_align(version, LV_ALIGN_BOTTOM_RIGHT, -4, -3);
}

void AuroraUI::buildMode() {
  header("Choisissez une action");
  lv_obj_t *create=button(root_,"NOUVEAU PORTEFEUILLE",event,178);
  lv_obj_set_user_data(create,(void*)NEW_WALLET); lv_obj_set_pos(create,12,48);
  lv_obj_set_size(create,178,44);
  lv_obj_t *open=button(root_,"OUVRIR AURORA WALLET",event,178);
  lv_obj_set_user_data(open,(void*)OPEN_WALLET); lv_obj_set_pos(open,12,108);
  lv_obj_set_size(open,178,44);
  lv_obj_t *restore=button(root_,"RESTAURER UNE SEED",event,178);
  lv_obj_set_user_data(restore,(void*)RESTORE_WALLET); lv_obj_set_pos(restore,12,168);
  lv_obj_set_size(restore,178,44);
  lv_obj_t *logo=lv_img_create(root_); lv_img_set_src(logo,&aurora_bitcoin_logo);
  lv_obj_set_pos(logo,200,43);
}

void AuroraUI::buildImportName() {
  header("Ouvrir Aurora Wallet");
  lv_obj_t *back=button(root_,"<",event,28); lv_obj_set_size(back,28,28);
  lv_obj_set_user_data(back,(void*)BACK_MODE); lv_obj_set_pos(back,276,3);
  lv_obj_t *hint=label(root_,"Sélectionnez un fichier .aurora sur la carte microSD",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); lv_obj_set_pos(hint,12,41);
  const AuroraWalletListResult listResult=listAuroraWalletFiles(
      auroraFileOptions_,sizeof(auroraFileOptions_),auroraFileCount_);
  importFileDropdown_=lv_dropdown_create(root_); lv_obj_set_pos(importFileDropdown_,12,59);
  lv_obj_set_size(importFileDropdown_,296,40);
  lv_obj_set_style_text_font(importFileDropdown_,&aurora_font_12,0);
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
  lv_obj_t *status=label(root_,statusText?statusText:"",&aurora_font_10);
  lv_label_set_long_mode(status,LV_LABEL_LONG_WRAP); lv_obj_set_size(status,296,30);
  lv_obj_set_style_text_color(status,statusColor,0); lv_obj_set_pos(status,12,108);

  lv_obj_t *refresh=button(root_,"ACTUALISER",event,130);
  lv_obj_set_user_data(refresh,(void*)REFRESH_AURORA_FILES); lv_obj_set_pos(refresh,12,164);
  lv_obj_t *open=button(root_,"OUVRIR",event,130);
  lv_obj_set_user_data(open,(void*)TO_IMPORT_PASSWORD); lv_obj_set_pos(open,178,164);
  if(listResult!=AuroraWalletListResult::Ok || !auroraFileCount_) {
    lv_obj_add_state(open,LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(open,PANEL,LV_STATE_DISABLED);
  }
}

void AuroraUI::buildImportPassword() {
  lv_obj_t *title=header("Mot de passe Aurora Wallet");
  lv_obj_set_style_text_font(title,&aurora_font_12,0); lv_obj_set_pos(title,90,10);
  lv_obj_t *back=button(root_,"<",event,28); lv_obj_set_size(back,28,28);
  lv_obj_set_user_data(back,(void*)BACK_IMPORT_NAME); lv_obj_set_pos(back,276,3);
  const char *message=importStatus_[0] ? importStatus_ :
      "Saisissez le mot de passe du fichier (12 caractères minimum).";
  lv_obj_t *hint=label(root_,message,&aurora_font_10);
  lv_label_set_long_mode(hint,LV_LABEL_LONG_WRAP); lv_obj_set_size(hint,296,26);
  lv_obj_set_style_text_color(hint,importStatus_[0]?DANGER:MUTED,0); lv_obj_set_pos(hint,12,40);
  filePasswordArea_=lv_textarea_create(root_); lv_obj_set_pos(filePasswordArea_,12,69);
  lv_obj_set_size(filePasswordArea_,296,38); lv_obj_set_style_text_font(filePasswordArea_,&aurora_font_12,0);
  lv_textarea_set_one_line(filePasswordArea_,true); lv_textarea_set_password_mode(filePasswordArea_,true);
  lv_textarea_set_max_length(filePasswordArea_,63); lv_textarea_set_accepted_chars(filePasswordArea_,PASSPHRASE_ASCII);
  lv_textarea_set_placeholder_text(filePasswordArea_,"Mot de passe du fichier");
  keyboard_=lv_keyboard_create(root_); lv_obj_set_size(keyboard_,320,112);
  lv_obj_align(keyboard_,LV_ALIGN_BOTTOM_MID,0,0); lv_keyboard_set_textarea(keyboard_,filePasswordArea_);
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_READY,(void*)UNLOCK_WALLET);
}

void AuroraUI::buildRestoreSetup() {
  header("Restaurer une seed");
  lv_obj_t *back=button(root_,"<",event,28); lv_obj_set_size(back,28,28);
  lv_obj_set_user_data(back,(void*)BACK_MODE); lv_obj_set_pos(back,276,3);
  lv_obj_t *hint=label(root_,"Choisissez le nombre de mots de la phrase BIP39.",&aurora_font_10);
  lv_obj_set_style_text_color(hint,ORANGE,0); lv_obj_set_pos(hint,12,43);

  const uint8_t counts[5]={12,15,18,21,24};
  for(uint8_t i=0;i<5;++i) {
    lv_obj_t *choice=button(root_,"",event,54); lv_obj_set_pos(choice,10+i*61,70);
    lv_obj_set_size(choice,54,40); lv_obj_set_user_data(choice,(void*)(WORD_12+i));
    const bool selected=words_==counts[i];
    lv_obj_set_style_bg_color(choice,selected?ORANGE:PANEL,0);
    lv_obj_set_style_border_color(choice,ORANGE,0);
    lv_obj_set_style_border_width(choice,selected?2:1,0);
    char text[3]; snprintf(text,sizeof(text),"%u",counts[i]);
    lv_obj_t *value=lv_obj_get_child(choice,0); lv_label_set_text(value,text);
    lv_obj_set_style_text_color(value,selected?BLACK:lv_color_white(),0);
  }

  lv_obj_t *info=label(root_,
      "Les mots sont vérifiés avec la liste anglaise officielle.\n"
      "Le checksum BIP39 sera contrôlé avant toute dérivation.",&aurora_font_10);
  lv_label_set_long_mode(info,LV_LABEL_LONG_WRAP); lv_obj_set_size(info,296,46);
  lv_obj_set_style_text_color(info,MUTED,0); lv_obj_set_pos(info,12,127);
  lv_obj_t *next=button(root_,"SAISIR LES MOTS",event,170);
  lv_obj_set_user_data(next,(void*)RESTORE_SETUP_CONTINUE);
  lv_obj_align(next,LV_ALIGN_BOTTOM_MID,0,-20);
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
  char title[40]; snprintf(title,sizeof(title),"Mot BIP39 %u / %u",
                           static_cast<unsigned>(restoreWordIndex_+1),
                           static_cast<unsigned>(words_));
  header(title);
  lv_obj_t *back=button(root_,"<",event,28); lv_obj_set_size(back,28,28);
  lv_obj_set_user_data(back,(void*)(restoreWordIndex_?RESTORE_WORD_BACK:BACK_RESTORE_SETUP));
  lv_obj_set_pos(back,276,3);

  const int16_t contentOffset=restoreStatus_[0]?14:0;
  if(restoreStatus_[0]) {
    lv_obj_t *hint=label(root_,restoreStatus_,&aurora_font_10);
    lv_label_set_long_mode(hint,LV_LABEL_LONG_WRAP); lv_obj_set_size(hint,296,14);
    lv_obj_set_style_text_color(hint,DANGER,0); lv_obj_set_pos(hint,12,38);
  }

  restoreWordArea_=lv_textarea_create(root_); lv_obj_set_pos(restoreWordArea_,12,40+contentOffset);
  lv_obj_set_size(restoreWordArea_,296,36); lv_obj_set_style_text_font(restoreWordArea_,&aurora_font_14,0);
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
    lv_obj_set_size(restoreSuggestionButtons_[i],94,26);
    lv_obj_set_pos(restoreSuggestionButtons_[i],8+i*104,80+contentOffset);
    lv_obj_set_user_data(restoreSuggestionButtons_[i],
                         (void*)(RESTORE_SUGGESTION_0+i));
  }

  updateRestoreSuggestions();

  keyboard_=lv_keyboard_create(root_); lv_obj_set_size(keyboard_,320,108);
  lv_obj_align(keyboard_,LV_ALIGN_BOTTOM_MID,0,0);
  lv_keyboard_set_mode(keyboard_,LV_KEYBOARD_MODE_TEXT_LOWER);
  lv_keyboard_set_textarea(keyboard_,restoreWordArea_);
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_READY,(void*)RESTORE_WORD_READY);
}

bool AuroraUI::acceptRestoreWord(const char *word) {
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
  show(Screen::RestorePassphrase);
  return true;
}

void AuroraUI::buildRestorePassphrase() {
  header("Passphrase BIP39");
  lv_obj_t *back=button(root_,"<",event,28); lv_obj_set_size(back,28,28);
  lv_obj_set_user_data(back,(void*)BACK_RESTORE_WORDS); lv_obj_set_pos(back,276,3);
  lv_obj_t *hint=label(root_,
      "Laissez vide si la phrase n'utilise pas de passphrase.",&aurora_font_10);
  lv_obj_set_style_text_color(hint,ORANGE,0); lv_obj_set_pos(hint,12,42);
  passArea_=lv_textarea_create(root_); lv_obj_set_pos(passArea_,12,62);
  lv_obj_set_size(passArea_,296,38); lv_obj_set_style_text_font(passArea_,&aurora_font_12,0);
  lv_textarea_set_one_line(passArea_,true); lv_textarea_set_password_mode(passArea_,true);
  lv_textarea_set_max_length(passArea_,63); lv_textarea_set_accepted_chars(passArea_,PASSPHRASE_ASCII);
  lv_textarea_set_placeholder_text(passArea_,"Passphrase optionnelle");
  if(passphrase_[0]) lv_textarea_set_text(passArea_,passphrase_);
  keyboard_=lv_keyboard_create(root_); lv_obj_set_size(keyboard_,320,112);
  lv_obj_align(keyboard_,LV_ALIGN_BOTTOM_MID,0,0); lv_keyboard_set_textarea(keyboard_,passArea_);
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_READY,(void*)RESTORE_DERIVE);
}

void AuroraUI::buildRestoring() {
  lv_obj_t *spinner=lv_spinner_create(root_,900,70); lv_obj_set_size(spinner,58,58);
  lv_obj_set_style_arc_color(spinner,PANEL,LV_PART_MAIN);
  lv_obj_set_style_arc_color(spinner,ORANGE,LV_PART_INDICATOR);
  lv_obj_align(spinner,LV_ALIGN_CENTER,0,-40);
  lv_obj_t *title=label(root_,"Restauration du portefeuille...",&aurora_font_16);
  lv_obj_align(title,LV_ALIGN_CENTER,0,15);
  lv_obj_t *hint=label(root_,"Validation BIP39 et dérivation BIP32 en cours.",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); lv_obj_align(hint,LV_ALIGN_CENTER,0,48);
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

void AuroraUI::buildSetup() {
  header("Configuration du portefeuille", "1 / 7");
  lv_obj_t *l1 = label(root_, "Nombre de mots", &aurora_font_10); lv_obj_set_pos(l1, 10, 41);
  const uint8_t counts[5] = {12,15,18,21,24};
  for (int i=0;i<5;++i) {
    lv_obj_t *b=lv_btn_create(root_); lv_obj_set_pos(b,10+i*61,56); lv_obj_set_size(b,55,27);
    bool selected=words_==counts[i]; lv_obj_set_style_radius(b,6,0);
    lv_obj_set_style_bg_color(b,selected?ORANGE:PANEL,0); lv_obj_set_style_border_color(b,ORANGE,0);
    lv_obj_set_style_border_width(b,selected?1:0,0); lv_obj_set_style_shadow_width(b,0,0);
    lv_obj_set_user_data(b,(void*)(WORD_12+i)); lv_obj_add_event_cb(b,event,LV_EVENT_CLICKED,nullptr);
    char text[3]; snprintf(text,sizeof(text),"%u",counts[i]); lv_obj_t *v=label(b,text,&aurora_font_10);
    lv_obj_set_style_text_color(v,selected?BLACK:lv_color_white(),0); lv_obj_center(v);
  }
  lv_obj_t *l2 = label(root_, "Type d'adresse", &aurora_font_10); lv_obj_set_pos(l2, 10, 86);
  const char *names[4]={"Legacy\nm/44'/0'/0'/0/0","Nested SegWit\nm/49'/0'/0'/0/0","Native SegWit\nm/84'/0'/0'/0/0","Taproot\nm/86'/0'/0'/0/0"};
  for(int i=0;i<4;++i){
    lv_obj_t *b=lv_btn_create(root_); int x=10+(i%2)*155, y=99+(i/2)*36; lv_obj_set_pos(b,x,y); lv_obj_set_size(b,145,32);
    bool selected=(uint8_t)kind_==i; lv_obj_set_style_radius(b,6,0); lv_obj_set_style_bg_color(b,PANEL,0);
    lv_obj_set_style_border_color(b,ORANGE,0); lv_obj_set_style_border_width(b,selected?2:1,0); lv_obj_set_style_shadow_width(b,0,0);
    lv_obj_set_user_data(b,(void*)(TYPE_LEGACY+i)); lv_obj_add_event_cb(b,event,LV_EVENT_CLICKED,nullptr);
    lv_obj_t *v=label(b,names[i],&aurora_font_10); lv_obj_set_style_text_color(v,selected?ORANGE:lv_color_white(),0); lv_obj_center(v);
  }
  lv_obj_t *safe=label(root_,"Bitcoin Mainnet • hors ligne",&aurora_font_10); lv_obj_set_style_text_color(safe,MUTED,0); lv_obj_set_pos(safe,10,181);
  lv_obj_t *b = button(root_, "CONTINUER", event, 105); lv_obj_set_user_data(b,(void*)TO_PASSPHRASE); lv_obj_set_pos(b,205,198);
}

void AuroraUI::buildPassphrase() {
  header("Passphrase BIP39", "2 / 7");
  lv_obj_t *warn = label(root_, "Optionnelle — ASCII uniquement, 63 caractères maximum.", &aurora_font_10);
  lv_obj_set_style_text_color(warn, ORANGE, 0); lv_obj_set_pos(warn, 14, 39);
  passArea_ = lv_textarea_create(root_); lv_obj_set_pos(passArea_, 14, 58); lv_obj_set_size(passArea_, 292, 38);
  lv_obj_set_style_text_font(passArea_, &aurora_font_12, 0);
  lv_textarea_set_one_line(passArea_, true); lv_textarea_set_password_mode(passArea_, true); lv_textarea_set_max_length(passArea_, 63);
  lv_textarea_set_accepted_chars(passArea_, PASSPHRASE_ASCII);
  lv_textarea_set_placeholder_text(passArea_, "Laisser vide ou saisir une passphrase");
  if(passphrase_[0]) lv_textarea_set_text(passArea_,passphrase_);
  keyboard_ = lv_keyboard_create(root_); lv_obj_set_size(keyboard_, 320, 104); lv_obj_align(keyboard_, LV_ALIGN_BOTTOM_MID,0,0); lv_keyboard_set_textarea(keyboard_,passArea_);
  lv_obj_add_event_cb(passArea_, event, LV_EVENT_READY, (void*)TO_ENTROPY);
}

void AuroraUI::buildEntropy() {
  header("Collecte d'entropie", "3 / 7"); entropy_.begin(); entropyReadyPending_ = false; entropyFailurePending_ = false;
  lv_obj_t *title = label(root_, "Tracez des lignes aléatoires avec votre doigt", &aurora_font_14); lv_obj_align(title,LV_ALIGN_TOP_MID,0,43);
  lv_obj_t *pad = lv_obj_create(root_); lv_obj_set_pos(pad,18,68); lv_obj_set_size(pad,284,104);
  lv_obj_set_style_bg_color(pad,PANEL,0); lv_obj_set_style_border_color(pad,ORANGE,0); lv_obj_set_style_border_width(pad,1,0); lv_obj_set_style_radius(pad,8,0);
  lv_obj_t *hint = label(pad,"RNG physique ESP32 + geste tactile\nMouvement, timing et pression",&aurora_font_10); lv_obj_set_style_text_color(hint,MUTED,0); lv_obj_center(hint);
  entropyBar_ = lv_bar_create(root_); lv_obj_set_pos(entropyBar_,18,184); lv_obj_set_size(entropyBar_,220,12); lv_bar_set_range(entropyBar_,0,100);
  lv_obj_set_style_bg_color(entropyBar_,ORANGE,LV_PART_INDICATOR);
  entropyStatus_ = label(root_,"0 %",&aurora_font_10); lv_obj_set_pos(entropyStatus_,246,183);
}

void AuroraUI::onTouchSample(int16_t x, int16_t y, uint16_t pressure) {
  if (screen_ != Screen::Entropy || !entropyBar_ || entropyReadyPending_) return;
  entropy_.add(x,y,pressure); uint8_t p = entropy_.progress(); lv_bar_set_value(entropyBar_,p,LV_ANIM_OFF);
  char s[12]; snprintf(s,sizeof(s),"%u %%",p); lv_label_set_text(entropyStatus_,s);
  if (entropy_.ready()) {
    if (entropy_.finish(mixedEntropy_)) entropyReadyPending_ = true;
    else entropyFailurePending_ = true;
  }
}

void AuroraUI::buildGenerating() {
  lv_obj_t *spinner=lv_spinner_create(root_,900,70); lv_obj_set_size(spinner,58,58);
  lv_obj_set_style_arc_color(spinner,PANEL,LV_PART_MAIN);
  lv_obj_set_style_arc_color(spinner,ORANGE,LV_PART_INDICATOR);
  lv_obj_align(spinner,LV_ALIGN_CENTER,0,-42);
  lv_obj_t *title=label(root_,"Génération du portefeuille...",&aurora_font_16);
  lv_obj_align(title,LV_ALIGN_CENTER,0,13);
  lv_obj_t *hint=label(root_,"Calcul BIP39 / BIP32 en cours\nVeuillez patienter quelques secondes.",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_CENTER,0);
  lv_obj_align(hint,LV_ALIGN_CENTER,0,48);
}

void AuroraUI::buildFileProcessing() {
  lv_obj_t *spinner=lv_spinner_create(root_,900,70); lv_obj_set_size(spinner,58,58);
  lv_obj_set_style_arc_color(spinner,PANEL,LV_PART_MAIN);
  lv_obj_set_style_arc_color(spinner,ORANGE,LV_PART_INDICATOR);
  lv_obj_align(spinner,LV_ALIGN_CENTER,0,-42);
  const bool importing=fileOperation_==FileOperation::Import;
  lv_obj_t *title=label(root_,importing?"Déchiffrement en cours...":"Chiffrement en cours...",&aurora_font_16);
  lv_obj_align(title,LV_ALIGN_CENTER,0,13);
  lv_obj_t *hint=label(root_,"PBKDF2-HMAC-SHA-256 + AES-256-GCM\nVeuillez patienter.",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_CENTER,0);
  lv_obj_align(hint,LV_ALIGN_CENTER,0,48);
}

void AuroraUI::buildGenerationError() {
  header("Erreur de génération");
  lv_obj_t *title=label(root_,"La génération n'a pas abouti.",&aurora_font_16);
  lv_obj_align(title,LV_ALIGN_CENTER,0,-30);
  lv_obj_t *hint=label(root_,"Aucune phrase ni clé n'a été conservée.",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); lv_obj_align(hint,LV_ALIGN_CENTER,0,2);
  lv_obj_t *retry=button(root_,"RECOMMENCER",event,150); lv_obj_set_user_data(retry,(void*)RETRY_ENTROPY);
  lv_obj_align(retry,LV_ALIGN_BOTTOM_MID,0,-24);
}

void AuroraUI::buildSecurityError() {
  header("ÉCHEC DE SÉCURITÉ");
  lv_obj_t *title=label(root_,"Autotest cryptographique échoué",&aurora_font_16);
  lv_obj_set_style_text_color(title,DANGER,0); lv_obj_align(title,LV_ALIGN_CENTER,0,-35);
  lv_obj_t *hint=label(root_,"Aucune seed ne peut être générée.\nReflashez un firmware vérifié.",&aurora_font_12);
  lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_CENTER,0); lv_obj_align(hint,LV_ALIGN_CENTER,0,10);
  char code[28];
  snprintf(code,sizeof(code),"Code diagnostic : E%02u",static_cast<unsigned>(selfTestResult_));
  lv_obj_t *detail=label(root_,code,&aurora_font_10);
  lv_obj_set_style_text_color(detail,MUTED,0); lv_obj_align(detail,LV_ALIGN_BOTTOM_MID,0,-22);
}

bool AuroraUI::generate() { return engine_.create(words_,kind_,passphrase_,mixedEntropy_,wallet_); }

void AuroraUI::buildMnemonic() {
  header("Phrase de récupération",
         loadedWallet_ ? (passphrase_[0] ? "1 / 4" : "1 / 3") : "4 / 7");
  const char *warningText=loadedWallet_ ?
      "Portefeuille déchiffré. Ne photographiez jamais ces mots." :
      "Écrivez ces mots dans l'ordre. Ne les photographiez jamais.";
  lv_obj_t *warning=label(root_,warningText,&aurora_font_10); lv_obj_set_style_text_color(warning,ORANGE,0); lv_obj_set_pos(warning,10,41);
  const uint8_t pageCount = (words_ + 7) / 8;
  if (mnemonicPage_ >= pageCount) mnemonicPage_ = pageCount - 1;
  const uint8_t first = mnemonicPage_ * 8;
  const uint8_t last = (first + 8 < words_) ? first + 8 : words_;

  lv_obj_t *box=lv_obj_create(root_); lv_obj_set_pos(box,10,56); lv_obj_set_size(box,300,121); lv_obj_set_style_bg_color(box,PANEL,0); lv_obj_set_style_border_width(box,0,0); lv_obj_set_style_radius(box,7,0);
  lv_obj_set_style_pad_all(box,5,0); lv_obj_clear_flag(box,LV_OBJ_FLAG_SCROLLABLE);
  char copy[256]; strlcpy(copy,wallet_.mnemonic,sizeof(copy)); char *save=nullptr; char *w=strtok_r(copy," ",&save); uint8_t i=0;
  while(w && i<last) {
    if(i>=first) {
      const uint8_t local=i-first, col=local/4, row=local%4;
      const int x=col*145, y=row*27;
      char number[4]; snprintf(number,sizeof(number),"%02u",i+1);
      lv_obj_t *n=label(box,number,&aurora_font_12); lv_obj_set_style_text_color(n,ORANGE,0); lv_obj_set_pos(n,x,y+2);
      lv_obj_t *word=label(box,w,&aurora_font_16); lv_obj_set_pos(word,x+29,y);
    }
    w=strtok_r(nullptr," ",&save); ++i;
  }
  secureZero(copy,sizeof(copy));

  if(mnemonicPage_>0) {
    lv_obj_t *previous=button(root_,"< PRÉCÉDENT",event,112); lv_obj_set_user_data(previous,(void*)MNEMONIC_PREVIOUS); lv_obj_set_pos(previous,10,190);
  }
  if(mnemonicPage_+1<pageCount) {
    lv_obj_t *next=button(root_,"SUIVANT >",event,112); lv_obj_set_user_data(next,(void*)MNEMONIC_NEXT); lv_obj_set_pos(next,198,190);
  } else {
    lv_obj_t *done=button(root_,loadedWallet_?"SUIVANT":"J'AI NOTÉ",event,132);
    const Action nextAction=(loadedWallet_ && !manualRestore_ && passphrase_[0]) ?
        SHOW_LOADED_PASSPHRASE : (loadedWallet_ ? TO_INFO : NEXT_VERIFY);
    lv_obj_set_user_data(done,(void*)nextAction); lv_obj_set_pos(done,178,190);
  }
}

void AuroraUI::buildPassphraseReveal() {
  header("Mot supplémentaire", "2 / 4");
  lv_obj_t *warning=label(root_,
      "DANGER : cette passphrase BIP39 est indispensable pour retrouver exactement ce portefeuille.",
      &aurora_font_10);
  lv_label_set_long_mode(warning,LV_LABEL_LONG_WRAP); lv_obj_set_size(warning,292,31);
  lv_obj_set_style_text_color(warning,DANGER,0); lv_obj_set_pos(warning,14,43);

  lv_obj_t *panel=lv_obj_create(root_); lv_obj_set_pos(panel,12,78); lv_obj_set_size(panel,296,91);
  lv_obj_set_style_bg_color(panel,PANEL,0); lv_obj_set_style_border_color(panel,DANGER,0);
  lv_obj_set_style_border_width(panel,1,0); lv_obj_set_style_radius(panel,7,0);
  lv_obj_set_style_pad_all(panel,7,0); lv_obj_clear_flag(panel,LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *value=label(panel,passphrase_,&aurora_font_14);
  lv_label_set_long_mode(value,LV_LABEL_LONG_WRAP); lv_obj_set_size(value,278,74);
  lv_obj_set_style_text_color(value,lv_color_white(),0); lv_obj_set_pos(value,1,1);

  lv_obj_t *next=button(root_,"CONTINUER",event,132);
  lv_obj_set_user_data(next,(void*)TO_INFO); lv_obj_set_pos(next,94,190);
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
    verifyArea_[i]=lv_textarea_create(root_); lv_obj_set_pos(verifyArea_[i],8+i*104,40);
    lv_obj_set_size(verifyArea_[i],94,36); lv_obj_set_style_text_font(verifyArea_[i],&aurora_font_12,0);
    lv_textarea_set_one_line(verifyArea_[i],true);
    lv_textarea_set_max_length(verifyArea_[i],WalletEngine::BIP39_WORD_CAPACITY-1);
    lv_textarea_set_accepted_chars(verifyArea_[i],"abcdefghijklmnopqrstuvwxyz");
    char ph[16]; snprintf(ph,sizeof(ph),"Mot n° %u",verifyIndex_[i]+1);
    lv_textarea_set_placeholder_text(verifyArea_[i],ph);
    lv_obj_set_user_data(verifyArea_[i],(void*)VERIFY_WORD_CHANGED);
    lv_obj_add_event_cb(verifyArea_[i],event,LV_EVENT_FOCUSED,nullptr);
    lv_obj_add_event_cb(verifyArea_[i],event,LV_EVENT_VALUE_CHANGED,nullptr);

    verifySuggestionButtons_[i]=button(root_,"",event,94);
    lv_obj_set_size(verifySuggestionButtons_[i],94,26);
    lv_obj_set_pos(verifySuggestionButtons_[i],8+i*104,80);
    lv_obj_set_user_data(verifySuggestionButtons_[i],(void*)(VERIFY_SUGGESTION_0+i));
  }
  updateVerifySuggestions();
  keyboard_=lv_keyboard_create(root_); lv_obj_set_size(keyboard_,320,112); lv_obj_align(keyboard_,LV_ALIGN_BOTTOM_MID,0,0); lv_keyboard_set_mode(keyboard_,LV_KEYBOARD_MODE_TEXT_LOWER); lv_keyboard_set_textarea(keyboard_,verifyArea_[0]);
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
  const char *step=manualRestore_ ? "2 / 3" :
      (loadedWallet_ ? (passphrase_[0] ? "3 / 4" : "2 / 3") : "6 / 7");
  header("Informations du portefeuille",step);
  lv_obj_t *p=lv_obj_create(root_); lv_obj_set_pos(p,10,42); lv_obj_set_size(p,300,132); lv_obj_set_style_bg_color(p,PANEL,0); lv_obj_set_style_border_width(p,0,0); lv_obj_set_style_radius(p,7,0); lv_obj_clear_flag(p,LV_OBJ_FLAG_SCROLLABLE);
  char txt[500]; snprintf(txt,sizeof(txt),"Adresse\n%s\nChemin : %s\nClé publique étendue du compte\n%s",wallet_.address,wallet_.path,wallet_.accountXpub);
  lv_obj_t *l=label(p,txt,&aurora_font_10); lv_label_set_long_mode(l,LV_LABEL_LONG_WRAP); lv_obj_set_size(l,290,124); lv_obj_set_pos(l,5,3);
  lv_obj_t *q=button(root_,"CODES QR",event,95); lv_obj_set_user_data(q,(void*)TO_QR_ADDRESS); lv_obj_set_pos(q,10,181);
  lv_obj_t *r=button(root_,"CLÉ PRIVÉE",event,100); lv_obj_set_user_data(r,(void*)REVEAL_PRIVATE); lv_obj_set_pos(r,110,181);
  const char *lastText=manualRestore_?"EXPORTER":(loadedWallet_?"EFFACER":"SUIVANT");
  const Action lastAction=manualRestore_?TO_BACKUP:(loadedWallet_?DO_WIPE:TO_BACKUP);
  lv_obj_t *x=button(root_,lastText,event,95);
  lv_obj_set_user_data(x,(void*)lastAction); lv_obj_set_pos(x,215,181);
}

bool AuroraUI::renderQr(lv_obj_t *parent,const char *data,int size,int x,int y) {
  if (!data || !data[0]) return false;
  lv_obj_t *qr = lv_qrcode_create(parent, size, lv_color_black(), lv_color_white());
  if (!qr) return false;
  if (lv_qrcode_update(qr, data, strlen(data)) != LV_RES_OK) {
    lv_obj_del(qr);
    return false;
  }
  lv_obj_set_pos(qr, x, y);
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

  if(restoreView) {
    static constexpr char DERIVATIONS[] =
        "Legacy - m/44'/0'/0'/0/0\n"
        "Nested SegWit - m/49'/0'/0'/0/0\n"
        "Native SegWit - m/84'/0'/0'/0/0\n"
        "Taproot - m/86'/0'/0'/0/0";
    restoreDerivationDropdown_=lv_dropdown_create(root_);
    lv_obj_set_pos(restoreDerivationDropdown_,6,39); lv_obj_set_size(restoreDerivationDropdown_,128,30);
    lv_obj_set_style_text_font(restoreDerivationDropdown_,&aurora_font_10,0);
    lv_dropdown_set_options(restoreDerivationDropdown_,DERIVATIONS);
    lv_dropdown_set_selected(restoreDerivationDropdown_,static_cast<uint16_t>(wallet_.kind));
    lv_obj_set_user_data(restoreDerivationDropdown_,(void*)RESTORE_DERIVATION_CHANGED);
    lv_obj_add_event_cb(restoreDerivationDropdown_,event,LV_EVENT_VALUE_CHANGED,nullptr);
  }

  if (!renderQr(root_,data,restoreView?128:158,6,restoreView?73:42)) {
    lv_obj_t *error=label(root_,"QR impossible",&aurora_font_14); lv_obj_set_style_text_color(error,DANGER,0); lv_obj_align(error,LV_ALIGN_LEFT_MID,25,0);
  }
  const int rightX = restoreView?140:168;
  const int rightWidth = restoreView?164:136;
  lv_obj_t *l=label(root_,data,&aurora_font_10); lv_label_set_long_mode(l,LV_LABEL_LONG_WRAP);
  lv_obj_set_size(l,rightWidth,restoreView?68:(privateKey ? 139 : 99));
  lv_obj_set_pos(l,rightX,restoreView?75:43);

  if(restoreView) {
    lv_obj_t *alternate=button(root_,privateKey?"ADRESSE":"CLÉ PRIVÉE",event,rightWidth);
    lv_obj_set_user_data(alternate,(void*)(privateKey?TO_QR_ADDRESS:REVEAL_PRIVATE));
    lv_obj_set_pos(alternate,rightX,148);
    if(!privateKey) lv_obj_set_style_bg_color(alternate,ORANGE,0);
    else lv_obj_set_style_bg_color(alternate,DANGER,0);
  } else if (qrContent_ == QrContent::Address) {
    lv_obj_t *p=button(root_,"CLÉ ÉTENDUE",event,rightWidth);
    lv_obj_set_user_data(p,(void*)TO_QR_PUBLIC); lv_obj_set_pos(p,rightX,147);
  } else if (qrContent_ == QrContent::AccountXpub) {
    lv_obj_t *a=button(root_,"ADRESSE",event,rightWidth);
    lv_obj_set_user_data(a,(void*)TO_QR_ADDRESS); lv_obj_set_pos(a,rightX,147);
  }
  lv_obj_t *b=button(root_,restoreView?"INFORMATIONS":"RETOUR",event,rightWidth);
  lv_obj_set_user_data(b,(void*)TO_INFO); lv_obj_set_pos(b,rightX,190);
}

void AuroraUI::buildBackup() {
  header("Sauvegarde sur microSD", manualRestore_ ? "3 / 3" : "7 / 7");
  lv_obj_t *intro=label(root_,"Choisissez un format (carte FAT32).",&aurora_font_10);
  lv_obj_set_style_text_color(intro,MUTED,0); lv_obj_set_pos(intro,12,41);

  lv_obj_t *aurora=button(root_,"AURORA WALLET CHIFFRÉ",event,280);
  lv_obj_set_user_data(aurora,(void*)EXPORT_AURORA); lv_obj_set_pos(aurora,20,61);
  lv_obj_t *electrum=button(root_,"ELECTRUM PRIVÉ NON CHIFFRÉ",event,280);
  lv_obj_set_user_data(electrum,(void*)EXPORT_ELECTRUM); lv_obj_set_pos(electrum,20,105);
  lv_obj_set_style_bg_color(electrum,DANGER,0);
  if (wallet_.kind == AddressKind::Taproot) {
    lv_obj_add_state(electrum,LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(electrum,MUTED,LV_STATE_DISABLED);
  }

  if (exportStatus_[0]) {
    lv_obj_t *status=label(root_,exportStatus_,&aurora_font_10);
    lv_label_set_long_mode(status,LV_LABEL_LONG_WRAP); lv_obj_set_size(status,296,42);
    lv_obj_set_pos(status,12,143);
    lv_obj_set_style_text_color(status,
        strncmp(exportStatus_,"Créé :",strlen("Créé :"))==0 ? ORANGE : DANGER,0);
  } else {
    const char *text=wallet_.kind==AddressKind::Taproot ?
        "Aurora Wallet : AES-256-GCM.\nElectrum indisponible en Taproot." :
        "Aurora Wallet est chiffré et authentifié.\nElectrum contient le xprv en clair.";
    lv_obj_t *explanation=label(root_,text,&aurora_font_10);
    lv_label_set_long_mode(explanation,LV_LABEL_LONG_WRAP); lv_obj_set_size(explanation,296,34);
    lv_obj_set_style_text_color(explanation,MUTED,0); lv_obj_set_pos(explanation,12,143);
  }
  lv_obj_t *wipe=button(root_,"EFFACER",event,122);
  lv_obj_set_user_data(wipe,(void*)DO_WIPE); lv_obj_set_pos(wipe,99,196);
}

void AuroraUI::buildExportWarning() {
  const bool aurora=exportFormat_==WalletExportFormat::AuroraWallet;
  header(aurora ? "Aurora Wallet chiffré" : "Electrum privé - DANGER",
         manualRestore_ ? "3 / 3" : "7 / 7");
  lv_obj_t *danger=label(root_,aurora?"AES-256-GCM":"SECRETS NON CHIFFRÉS",&aurora_font_16);
  lv_obj_set_style_text_color(danger,aurora?ORANGE:DANGER,0); lv_obj_align(danger,LV_ALIGN_TOP_MID,0,50);
  const char *message=aurora ?
      "Les mots et les clés seront chiffrés avec un mot de passe.\n"
      "Mot de passe perdu = fichier définitivement illisible." :
      "Ce fichier Electrum contient le xprv du compte en clair.\n"
      "Toute personne qui le possède peut dépenser les bitcoins.";
  lv_obj_t *warning=label(root_,message,&aurora_font_12);
  lv_label_set_long_mode(warning,LV_LABEL_LONG_WRAP); lv_obj_set_size(warning,292,76);
  lv_obj_set_style_text_align(warning,LV_TEXT_ALIGN_CENTER,0); lv_obj_set_pos(warning,14,82);
  lv_obj_t *cancel=button(root_,"ANNULER",event,120);
  lv_obj_set_user_data(cancel,(void*)BACK_BACKUP); lv_obj_set_pos(cancel,20,184);
  lv_obj_t *confirm=button(root_,aurora?"CONTINUER":"JE COMPRENDS",event,142);
  lv_obj_set_user_data(confirm,(void*)CONFIRM_PRIVATE); lv_obj_set_pos(confirm,158,184);
  if(!aurora) lv_obj_set_style_bg_color(confirm,DANGER,0);
}

void AuroraUI::buildExportName() {
  header("Nom du fichier", manualRestore_ ? "3 / 3" : "7 / 7");
  lv_obj_t *hint=label(root_,"Saisissez le nom sans extension (24 caractères max.)",&aurora_font_10);
  lv_obj_set_style_text_color(hint,MUTED,0); lv_obj_set_pos(hint,12,41);
  exportNameArea_=lv_textarea_create(root_); lv_obj_set_pos(exportNameArea_,12,57);
  lv_obj_set_size(exportNameArea_,296,38); lv_obj_set_style_text_font(exportNameArea_,&aurora_font_12,0);
  lv_textarea_set_one_line(exportNameArea_,true); lv_textarea_set_max_length(exportNameArea_,24);
  lv_textarea_set_accepted_chars(exportNameArea_,FILE_NAME_CHARS);
  lv_textarea_set_placeholder_text(exportNameArea_,"mon_nom");
  lv_textarea_set_text(exportNameArea_,exportBaseName_);
  char preview[80] = {};
  snprintf(preview,sizeof(preview),"Suffixe : %s   Validez avec ENTRÉE.",walletExportSuffix(exportFormat_));
  lv_obj_t *suffix=label(root_,preview,&aurora_font_10);
  lv_obj_set_style_text_color(suffix,ORANGE,0); lv_obj_set_pos(suffix,12,103);
  keyboard_=lv_keyboard_create(root_); lv_obj_set_size(keyboard_,320,112);
  lv_obj_align(keyboard_,LV_ALIGN_BOTTOM_MID,0,0); lv_keyboard_set_textarea(keyboard_,exportNameArea_);
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_READY,(void*)SAVE_EXPORT);
}

void AuroraUI::buildExportPassword() {
  header("Mot de passe du fichier", manualRestore_ ? "3 / 3" : "7 / 7");
  const char *message=passwordStatus_[0] ? passwordStatus_ :
      "12 à 63 caractères. Conservez ce mot de passe séparément.";
  lv_obj_t *hint=label(root_,message,&aurora_font_10);
  lv_label_set_long_mode(hint,LV_LABEL_LONG_WRAP); lv_obj_set_size(hint,296,20);
  lv_obj_set_style_text_color(hint,passwordStatus_[0]?DANGER:ORANGE,0); lv_obj_set_pos(hint,12,39);

  filePasswordArea_=lv_textarea_create(root_); lv_obj_set_pos(filePasswordArea_,12,58);
  lv_obj_set_size(filePasswordArea_,143,35); lv_obj_set_style_text_font(filePasswordArea_,&aurora_font_10,0);
  lv_textarea_set_one_line(filePasswordArea_,true); lv_textarea_set_password_mode(filePasswordArea_,true);
  lv_textarea_set_max_length(filePasswordArea_,63); lv_textarea_set_accepted_chars(filePasswordArea_,PASSPHRASE_ASCII);
  lv_textarea_set_placeholder_text(filePasswordArea_,"Mot de passe");
  lv_obj_add_event_cb(filePasswordArea_,event,LV_EVENT_FOCUSED,nullptr);

  filePasswordConfirmArea_=lv_textarea_create(root_); lv_obj_set_pos(filePasswordConfirmArea_,165,58);
  lv_obj_set_size(filePasswordConfirmArea_,143,35); lv_obj_set_style_text_font(filePasswordConfirmArea_,&aurora_font_10,0);
  lv_textarea_set_one_line(filePasswordConfirmArea_,true); lv_textarea_set_password_mode(filePasswordConfirmArea_,true);
  lv_textarea_set_max_length(filePasswordConfirmArea_,63); lv_textarea_set_accepted_chars(filePasswordConfirmArea_,PASSPHRASE_ASCII);
  lv_textarea_set_placeholder_text(filePasswordConfirmArea_,"Confirmation");
  lv_obj_add_event_cb(filePasswordConfirmArea_,event,LV_EVENT_FOCUSED,nullptr);

  lv_obj_t *note=label(root_,"Ce mot de passe est différent de la passphrase BIP39.",&aurora_font_10);
  lv_obj_set_style_text_color(note,MUTED,0); lv_obj_set_pos(note,12,103);
  keyboard_=lv_keyboard_create(root_); lv_obj_set_size(keyboard_,320,112);
  lv_obj_align(keyboard_,LV_ALIGN_BOTTOM_MID,0,0); lv_keyboard_set_textarea(keyboard_,filePasswordArea_);
  lv_obj_add_event_cb(keyboard_,event,LV_EVENT_READY,(void*)SAVE_EXPORT_PASSWORD);
}

void AuroraUI::performWalletExport() {
  char accountXprv[128] = {};
  char writtenPath[56] = {};
  if (exportFormat_==WalletExportFormat::ElectrumPrivate &&
      wallet_.kind == AddressKind::Taproot) {
    strlcpy(exportStatus_,"Taproot / BIP86 non pris en charge par Electrum.",sizeof(exportStatus_));
    return;
  }
  if (!engine_.accountXprv(wallet_,passphrase_,accountXprv,sizeof(accountXprv))) {
    strlcpy(exportStatus_,"Échec de dérivation de la clé privée étendue.",sizeof(exportStatus_));
    secureZero(accountXprv,sizeof(accountXprv));
    return;
  }

  const WalletExportData data{
      static_cast<uint8_t>(wallet_.kind), words_, addressKindName(wallet_.kind),
      wallet_.path, wallet_.mnemonic, passphrase_, wallet_.address,
      wallet_.accountXpub, accountXprv, wallet_.privateWif,
      wallet_.watchDescriptor};
  const WalletExportResult result=writeWalletExportFile(
      exportFormat_,exportBaseName_,filePassword_,data,writtenPath,sizeof(writtenPath));
  secureZero(accountXprv,sizeof(accountXprv));
  switch(result) {
    case WalletExportResult::Ok:
      snprintf(exportStatus_,sizeof(exportStatus_),"Créé : %s",writtenPath); break;
    case WalletExportResult::InvalidName:
      strlcpy(exportStatus_,"Nom de fichier invalide.",sizeof(exportStatus_)); break;
    case WalletExportResult::InvalidData:
      strlcpy(exportStatus_,"Données incomplètes : export annulé.",sizeof(exportStatus_)); break;
    case WalletExportResult::UnsupportedFormat:
      strlcpy(exportStatus_,"Format indisponible pour ce portefeuille.",sizeof(exportStatus_)); break;
    case WalletExportResult::WeakPassword:
      strlcpy(exportStatus_,"Mot de passe trop court ou invalide.",sizeof(exportStatus_)); break;
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
      strlcpy(exportStatus_,"Écriture incomplète : fichier supprimé.",sizeof(exportStatus_)); break;
  }
  secureZero(writtenPath,sizeof(writtenPath));
}

bool AuroraUI::performWalletImport() {
  AuroraWalletData imported{};
  char derivedXprv[128] = {};
  bool ok = false;
  const AuroraWalletReadResult result=readAuroraWalletFile(
      importBaseName_,filePassword_,imported);
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
      secureZero(importStatus_,sizeof(importStatus_));
      ok=true;
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
  return ok;
}

void AuroraUI::buildWipe() {
  engine_.wipe(wallet_); secureZero(passphrase_,sizeof(passphrase_));
  secureZero(filePassword_,sizeof(filePassword_)); secureZero(mixedEntropy_,sizeof(mixedEntropy_));
  exportFormat_=WalletExportFormat::AuroraWallet; fileOperation_=FileOperation::None;
  loadedWallet_=false; manualRestore_=false; restoreWordIndex_=0; restoreSuggestionCount_=0;
  secureZero(restoreWords_,sizeof(restoreWords_)); secureZero(restoreSuggestions_,sizeof(restoreSuggestions_));
  secureZero(restoreMnemonic_,sizeof(restoreMnemonic_)); secureZero(restoreStatus_,sizeof(restoreStatus_));
  secureZero(exportStatus_,sizeof(exportStatus_));
  secureZero(importStatus_,sizeof(importStatus_)); secureZero(passwordStatus_,sizeof(passwordStatus_));
  secureZero(exportBaseName_,sizeof(exportBaseName_)); strlcpy(exportBaseName_,"aurora",sizeof(exportBaseName_));
  secureZero(importBaseName_,sizeof(importBaseName_)); strlcpy(importBaseName_,"aurora",sizeof(importBaseName_));
  header("Effacement terminé"); lv_obj_t *ok=label(root_,"OK",&aurora_font_20); lv_obj_set_style_text_color(ok,ORANGE,0); lv_obj_align(ok,LV_ALIGN_CENTER,0,-40);
  lv_obj_t *msg=label(root_,"Les tampons sensibles de la session\nont été écrasés en mémoire vive.",&aurora_font_12); lv_obj_set_style_text_align(msg,LV_TEXT_ALIGN_CENTER,0); lv_obj_align(msg,LV_ALIGN_CENTER,0,0);
  lv_obj_t *b=button(root_,"RETOUR À L'ACCUEIL",event,190); lv_obj_set_user_data(b,(void*)BACK_MODE); lv_obj_align(b,LV_ALIGN_BOTTOM_MID,0,-18);
}

void AuroraUI::event(lv_event_t *e) {
  if(!g_ui)return; lv_obj_t *target=lv_event_get_target(e);
  if(lv_event_get_code(e)==LV_EVENT_FOCUSED &&
     (g_ui->screen_==Screen::Verify || g_ui->screen_==Screen::ExportPassword)) {
    lv_keyboard_set_textarea(g_ui->keyboard_,target);
    if(g_ui->screen_==Screen::Verify) {
      for(uint8_t i=0;i<3;++i)
        if(target==g_ui->verifyArea_[i]) g_ui->verifyActiveIndex_=i;
      g_ui->updateVerifySuggestions();
    }
    return;
  }
  uintptr_t a=(uintptr_t)lv_obj_get_user_data(target);
  if(lv_event_get_code(e)==LV_EVENT_READY) a=(uintptr_t)lv_event_get_user_data(e);
  switch((Action)a){
    case START: if(!g_ui->selfTestPending_) g_ui->show(Screen::Mode); break;
    case OPEN_WALLET:
      g_ui->engine_.wipe(g_ui->wallet_); secureZero(g_ui->passphrase_,sizeof(g_ui->passphrase_));
      g_ui->loadedWallet_=false; g_ui->manualRestore_=false;
      secureZero(g_ui->importStatus_,sizeof(g_ui->importStatus_));
      g_ui->show(Screen::ImportName); break;
    case NEW_WALLET:
      g_ui->engine_.wipe(g_ui->wallet_); secureZero(g_ui->passphrase_,sizeof(g_ui->passphrase_));
      g_ui->loadedWallet_=false; g_ui->manualRestore_=false;
      secureZero(g_ui->exportStatus_,sizeof(g_ui->exportStatus_));
      g_ui->show(Screen::Setup); break;
    case RESTORE_WALLET:
      g_ui->engine_.wipe(g_ui->wallet_); secureZero(g_ui->passphrase_,sizeof(g_ui->passphrase_));
      g_ui->loadedWallet_=false; g_ui->manualRestore_=true; g_ui->kind_=AddressKind::NativeSegwit;
      g_ui->words_=12; g_ui->restoreWordIndex_=0; g_ui->restoreSuggestionCount_=0;
      secureZero(g_ui->restoreWords_,sizeof(g_ui->restoreWords_));
      secureZero(g_ui->restoreSuggestions_,sizeof(g_ui->restoreSuggestions_));
      secureZero(g_ui->restoreMnemonic_,sizeof(g_ui->restoreMnemonic_));
      secureZero(g_ui->restoreStatus_,sizeof(g_ui->restoreStatus_));
      g_ui->show(Screen::RestoreSetup); break;
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
      char *source=const_cast<char *>(lv_textarea_get_text(g_ui->passArea_));
      const size_t length=source?strlen(source):0;
      strlcpy(g_ui->passphrase_,source?source:"",sizeof(g_ui->passphrase_));
      if(source) secureZero(source,length);
      g_ui->show(Screen::Restoring); g_ui->generationDueMs_=millis()+100;
      break;
    }
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
        g_ui->fileOperationDueMs_=millis()+100;
      }
      break;
    }
    case TO_PASSPHRASE: g_ui->show(Screen::Passphrase); break;
    case TO_ENTROPY: {
      char *source=const_cast<char *>(lv_textarea_get_text(g_ui->passArea_));
      strlcpy(g_ui->passphrase_,source,sizeof(g_ui->passphrase_));
      secureZero(source,strlen(source));
      lv_textarea_set_text(g_ui->passArea_,"");
      g_ui->show(Screen::Entropy);
      break;
    }
    case NEXT_VERIFY: g_ui->show(Screen::Verify); break;
    case CHECK_VERIFY: if(g_ui->verifyWords())g_ui->show(Screen::Info); break;
    case SHOW_LOADED_PASSPHRASE:
      if(g_ui->loadedWallet_ && !g_ui->manualRestore_ && g_ui->passphrase_[0])
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
        g_ui->fileOperationDueMs_=millis()+100;
      }
      break;
    }
    case SAVE_EXPORT_PASSWORD: {
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
        g_ui->fileOperation_=FileOperation::Export; g_ui->show(Screen::FileProcessing);
        g_ui->fileOperationDueMs_=millis()+100;
      }
      break;
    }
    case DO_WIPE: g_ui->show(Screen::Wipe); break;
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
    case BACK_BACKUP: g_ui->show(Screen::Backup); break;
    case BACK_IMPORT_NAME:
      secureZero(g_ui->filePassword_,sizeof(g_ui->filePassword_));
      g_ui->show(Screen::ImportName); break;
    case BACK_EXPORT_NAME:
      secureZero(g_ui->filePassword_,sizeof(g_ui->filePassword_));
      g_ui->show(Screen::ExportName); break;
    case BACK_MODE_WIPE: g_ui->show(Screen::Wipe); break;
    case BACK_RESTORE_SETUP:
      g_ui->restoreWordIndex_=0; secureZero(g_ui->restoreStatus_,sizeof(g_ui->restoreStatus_));
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
      if((g_ui->mnemonicPage_+1)*8<g_ui->words_) ++g_ui->mnemonicPage_;
      g_ui->show(Screen::Mnemonic); break;
    case RETRY_ENTROPY: g_ui->show(Screen::Entropy); break;
    case WORD_12: case WORD_15: case WORD_18: case WORD_21: case WORD_24:
      g_ui->words_=12+3*((uint8_t)a-WORD_12);
      if(g_ui->screen_==Screen::RestoreSetup) {
        g_ui->restoreWordIndex_=0;
        secureZero(g_ui->restoreWords_,sizeof(g_ui->restoreWords_));
        secureZero(g_ui->restoreStatus_,sizeof(g_ui->restoreStatus_));
        g_ui->show(Screen::RestoreSetup);
      } else g_ui->show(Screen::Setup);
      break;
    case TYPE_LEGACY: case TYPE_NESTED: case TYPE_NATIVE: case TYPE_TAPROOT:
      g_ui->kind_=(AddressKind)((uint8_t)a-TYPE_LEGACY); g_ui->show(Screen::Setup); break;
    default: break;
  }
}
