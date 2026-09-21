#pragma once

// Included by both native runners after the real UI and their click helper.
// All credentials/keys are public fixtures; no physical storage is used.
static void navigationKeyboardEvent(AuroraUI &ui, lv_event_code_t code) {
  lv_disp_trig_activity(nullptr);
#if defined(AURORA_BOARD_P4)
  lv_obj_send_event(ui.keyboard_,code,nullptr);
#else
  lv_event_send(ui.keyboard_,code,nullptr);
#endif
}

static void testFileNavigation(AuroraUI &ui, const char *password) {
  using Screen=AuroraUI::Screen;
  using Access=AuroraUI::Access;
  assert(ui.screen_==Screen::Info && ui.fileSession_ && ui.hasPassphrase());
  uint8_t identity[32]; memcpy(identity,ui.sessionFingerprint_,sizeof(identity));
  char name[25]; strlcpy(name,ui.sessionBaseName_,sizeof(name));
  const auto zero=[](const void *data,size_t size) {
    for(size_t i=0;i<size;++i) assert(!static_cast<const uint8_t *>(data)[i]);
  };
  const auto publicOnly=[&]() {
    assert(ui.fileSession_ && ui.protectedSession_ && !ui.privateLoaded_);
    assert(!memcmp(identity,ui.sessionFingerprint_,sizeof(identity)));
    assert(!strcmp(name,ui.sessionBaseName_));
    zero(ui.wallet_.mnemonic,sizeof(ui.wallet_.mnemonic));
    zero(ui.wallet_.privateWif,sizeof(ui.wallet_.privateWif));
    zero(ui.wallet_.privateDescriptor,sizeof(ui.wallet_.privateDescriptor));
    zero(ui.passphrase_,sizeof(ui.passphrase_));
    zero(ui.filePassword_,sizeof(ui.filePassword_));
    zero(ui.mixedEntropy_,sizeof(ui.mixedEntropy_));
    zero(ui.restoreWords_,sizeof(ui.restoreWords_));
    zero(ui.restoreMnemonic_,sizeof(ui.restoreMnemonic_));
    zero(ui.umbrelRootXprv_,sizeof(ui.umbrelRootXprv_));
    assert(ui.access_==Access::None && !ui.accessGrantedMs_);
    assert(ui.visibleSecret_==Access::None && !ui.visibleSecretStartedMs_);
    assert(!ui.secretCountdown_ && !ui.fileOperationDueMs_);
    assert(ui.fileOperation_==AuroraUI::FileOperation::None);
  };
  const auto tap=[&](Action action) {
    lv_disp_trig_activity(nullptr); click(ui,action);
  };
  const auto authenticate=[&]() {
    assert(ui.screen_==Screen::PrivatePassword);
    publicOnly();
    assert(!lv_textarea_get_text(ui.filePasswordArea_)[0]);
    lv_textarea_set_text(ui.filePasswordArea_,password);
    navigationKeyboardEvent(ui,LV_EVENT_READY);
    assert(ui.screen_==Screen::FileProcessing);
    mock.time+=101000; ui.tick();
    assert(ui.privateLoaded_ && !ui.filePassword_[0]);
  };

  // Repeated consultations, including words on page two, return to the same
  // dashboard. Each new category requires a fresh password and file read.
  for(Action action:{SHOW_WORDS,REVEAL_PRIVATE,SHOW_LOADED_PASSPHRASE,SHOW_WORDS}) {
    tap(action); authenticate();
    const Screen expected=action==SHOW_WORDS?Screen::Mnemonic:
        action==REVEAL_PRIVATE?Screen::Qr:Screen::PassphraseReveal;
    assert(ui.screen_==expected);
    if(action==SHOW_WORDS) {
      ui.words_=24; ui.show(Screen::Mnemonic);
      const auto started=ui.visibleSecretStartedMs_;
      mock.time+=1000000; tap(MNEMONIC_NEXT);
      assert(ui.mnemonicPage_==1 && ui.visibleSecretStartedMs_==started);
    }
    // Inject public markers even in the minimal CYD derivation mock.
    strlcpy(ui.wallet_.mnemonic,"PUBLIC-NAV-WORDS",sizeof(ui.wallet_.mnemonic));
    strlcpy(ui.wallet_.privateWif,"PUBLIC-NAV-WIF",sizeof(ui.wallet_.privateWif));
    strlcpy(ui.wallet_.privateDescriptor,"PUBLIC-NAV-DESCRIPTOR",sizeof(ui.wallet_.privateDescriptor));
    // Legacy sessions preserve their public data in place. P4 sessions rebuild
    // the selected public child from the authenticated account xpub cache.
    strlcpy(ui.wallet_.address,"public-navigation-address",sizeof(ui.wallet_.address));
    strlcpy(ui.wallet_.accountXpub,"public-navigation-xpub",sizeof(ui.wallet_.accountXpub));
    strlcpy(ui.wallet_.path,"m/84'/0'/0'/0/0",sizeof(ui.wallet_.path));
    for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
      auto *child=lv_obj_get_child(ui.root_,i);
      assert(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(child))!=BACK_MNEMONIC);
    }
    if(action==SHOW_LOADED_PASSPHRASE) snapshot(ui,"navigation-passphrase.ppm");
    tap(TO_INFO); assert(ui.screen_==Screen::Info); publicOnly();
    assert(ui.requestedAccess_==Access::None);
#if defined(AURORA_BOARD_P4)
    assert(!strcmp(ui.wallet_.address,ui.bip39Public_[ui.bip39Scope_].address));
    assert(!strcmp(ui.wallet_.accountXpub,
                   ui.bip39Public_[ui.bip39Scope_].accountXpub));
    assert(!strcmp(ui.wallet_.path,ui.bip39Public_[ui.bip39Scope_].path));
    assertDisplayReplaced(ui);
#else
    assert(!strcmp(ui.wallet_.address,"public-navigation-address"));
    assert(!strcmp(ui.wallet_.accountXpub,"public-navigation-xpub"));
    assert(!strcmp(ui.wallet_.path,"m/84'/0'/0'/0/0"));
#endif
    snapshot(ui,"navigation-return-info.ppm");
  }
  // Public QR needs no password and returns to the same dashboard.
  tap(TO_QR_ADDRESS); assert(ui.screen_==Screen::Qr); publicOnly();
  tap(TO_QR_PUBLIC); assert(ui.screen_==Screen::Qr); publicOnly();
  tap(TO_INFO); assert(ui.screen_==Screen::Info); publicOnly();

  // Both the arrow and keyboard cancel of a password request return to its
  // actual parent. There must be no pending write or retained edited password.
  for(Action action:{SHOW_WORDS,REVEAL_PRIVATE,SHOW_LOADED_PASSPHRASE}) {
    for(bool keyboard:{false,true}) {
      tap(action); assert(ui.screen_==Screen::PrivatePassword);
      lv_textarea_set_text(ui.filePasswordArea_,"cancel-public-fixture");
      if(keyboard) navigationKeyboardEvent(ui,LV_EVENT_CANCEL);
      else tap(PRIVATE_PASSWORD_CANCEL);
      assert(ui.screen_==Screen::Info); publicOnly();
    }
  }
  for(Action format:{EXPORT_AURORA,EXPORT_ELECTRUM}) {
    tap(TO_BACKUP); assert(ui.screen_==Screen::Backup);
    for(bool keyboard:{false,true}) {
      tap(format); assert(ui.screen_==Screen::PrivatePassword);
      lv_textarea_set_text(ui.filePasswordArea_,"cancel-public-fixture");
      if(keyboard) navigationKeyboardEvent(ui,LV_EVENT_CANCEL);
      else tap(PRIVATE_PASSWORD_CANCEL);
      assert(ui.screen_==Screen::Backup); publicOnly();
    }
    tap(format); authenticate(); assert(ui.screen_==Screen::ExportWarning);
    const auto grant=ui.accessGrantedMs_;
    for(bool keyboard:{false,true}) {
      tap(CONFIRM_PRIVATE); assert(ui.screen_==Screen::ExportName);
      if(format==EXPORT_AURORA) {
        navigationKeyboardEvent(ui,LV_EVENT_READY);
        assert(ui.screen_==Screen::ExportPassword);
        lv_textarea_set_text(ui.filePasswordArea_,"discard-export-password");
        lv_textarea_set_text(ui.filePasswordConfirmArea_,"discard-confirmation");
        if(keyboard) navigationKeyboardEvent(ui,LV_EVENT_CANCEL);
        else tap(BACK_EXPORT_NAME);
        assert(ui.screen_==Screen::ExportName && !ui.filePassword_[0]);
      }
      if(keyboard) navigationKeyboardEvent(ui,LV_EVENT_CANCEL);
      else tap(BACK_EXPORT_WARNING);
      assert(ui.screen_==Screen::ExportWarning && ui.accessGrantedMs_==grant);
    }
    tap(BACK_BACKUP); assert(ui.screen_==Screen::Backup); publicOnly();
    tap(BACK_INFO); assert(ui.screen_==Screen::Info); publicOnly();
  }
  tap(LOCK_SESSION);
  assert(ui.screen_==Screen::Mode && !ui.fileSession_ && !ui.protectedSession_);
  zero(&ui.wallet_,sizeof(ui.wallet_)); zero(ui.sessionFingerprint_,sizeof(ui.sessionFingerprint_));
  zero(ui.sessionBaseName_,sizeof(ui.sessionBaseName_));
  // A touch at expiry must still close, not revive the expired consultation.
  // Also check that returning early does not disable the public-session idle lock.
  for(unsigned route=0;route<3;++route) {
    lv_disp_trig_activity(nullptr);
    strlcpy(ui.importBaseName_,name,sizeof(ui.importBaseName_));
    strlcpy(ui.filePassword_,password,sizeof(ui.filePassword_));
    assert(ui.performWalletImport()); ui.show(Screen::Info);
    tap(route==1?REVEAL_PRIVATE:SHOW_WORDS); authenticate();
    if(route<2) mock.time+=AuroraUI::accessDurationMs(ui.visibleSecret_)*1000;
    tap(TO_INFO); // Deliberately before the next periodic tick.
    if(route==2) {
      assert(ui.screen_==Screen::Info); publicOnly();
      mock.time+=AuroraUI::SESSION_IDLE_MS*1000; ui.tick();
    }
    assert(ui.screen_==Screen::Mode && !ui.fileSession_ && !ui.privateLoaded_);
    zero(&ui.wallet_,sizeof(ui.wallet_)); zero(ui.filePassword_,sizeof(ui.filePassword_));
    zero(ui.sessionFingerprint_,sizeof(ui.sessionFingerprint_));
  }
  puts("PASS: shared navigation: private Return keeps only public dashboard/identity; fresh password; public QR; password/export arrow and keyboard back; explicit lock wipes session");
  puts("PASS: Return at words/key deadline still closes; returned public dashboard still locks on inactivity");
}
