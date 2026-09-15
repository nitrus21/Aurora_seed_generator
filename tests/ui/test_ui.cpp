#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include "Arduino.h"
#include "Hash.h"
#include "lvgl.h"
#define private public
#include "ui.h"
#undef private
#include "sensors.h"
#include "secure_lvgl_memory.h"
static unsigned wipedAllocations=0;
extern "C" void auroraUiWipeAudit(const void *pointer,size_t size) {
  for(size_t i=0;i<size;++i) assert(static_cast<const uint8_t *>(pointer)[i]==0);
  ++wipedAllocations;
}

// Only test fixtures: no real wallet, device, SD, camera, or microphone access.
static WalletOutput importedWallet{};
static AuroraWalletData importedData{};
static bool importOk=false, exportOk=false;
static unsigned exportCalls=0, xprvCalls=0, importCalls=0;
static bool sdReady=true;
static unsigned sdChecks=0, sdFailOnCheck=0;
bool auroraSdReady() { ++sdChecks; return sdReady && sdChecks!=sdFailOnCheck; }
WalletEngine::WalletEngine() {}
WalletSelfTest WalletEngine::selfTest() { return WalletSelfTest::Ok; }
bool WalletEngine::create(uint8_t, AddressKind, const char *, const uint8_t *, WalletOutput &) { return true; }
bool WalletEngine::restore(const char *, uint8_t, AddressKind, const char *, WalletOutput &out) { out=importedWallet; return true; }
bool WalletEngine::accountXprv(const WalletOutput &, const char *, char *out, size_t size) {
  ++xprvCalls; strlcpy(out,"test-xprv",size); return true;
}
bool WalletEngine::rootXprvFromSeed(const uint8_t *,size_t,char *out,size_t size) {
  strlcpy(out,"xprv9s21ZrQH143K3-test-only-not-a-real-master-private-key",size); return true;
}
AezeedResult AezeedEngine::decode(const char *,const char *,AezeedDecoded &out) {
  out.birthdayDays=4242; memset(out.entropy,0x42,sizeof(out.entropy)); return AezeedResult::Ok;
}
void AezeedEngine::wipe(AezeedDecoded &out) { secureZero(&out,sizeof(out)); }
void WalletEngine::wipe(WalletOutput &wallet) { secureZero(&wallet, sizeof(wallet)); }
bool WalletEngine::bip39Word(const char *) { return true; }
uint8_t WalletEngine::bip39Suggestions(const char *, char *, size_t, uint8_t) { return 0; }
const char *walletExportSuffix(WalletExportFormat kind) { return kind == WalletExportFormat::AuroraWallet ? ".aurora" : ".json"; }
bool auroraWalletCryptoSelfTest() { return true; }
void wipeAuroraWalletData(AuroraWalletData &data) { secureZero(&data, sizeof(data)); }
WalletExportResult writeWalletExportFile(WalletExportFormat, const char *, const char *, const WalletExportData &data, char *path, size_t size) {
  ++exportCalls; assert(data.pin && auroraPinRecordValid(*data.pin));
  strlcpy(path,"/fixture.aurora",size); return exportOk && sdReady?WalletExportResult::Ok:WalletExportResult::NoCard;
}
AuroraWalletReadResult readAuroraWalletFile(const char *, const char *, AuroraWalletData &out) {
  ++importCalls;
  if(!importOk || !sdReady) return AuroraWalletReadResult::NoCard;
  out=importedData; return AuroraWalletReadResult::Ok;
}
AuroraWalletListResult listAuroraWalletFiles(char *out, size_t size, uint16_t &count) { if(size) out[0]=0; count=0; return AuroraWalletListResult::NoCard; }
namespace AuroraSensors {
bool running = false, requested = false, acknowledge = true;
bool start() { running = true; requested = false; return true; }
void requestStop() { requested = true; }
bool stopped() { if (requested && acknowledge) running = false; return !running; }
Status status() { return {State::Active, State::Absent, 35, 0, 0}; }
void drain(TouchEntropy &) {}
bool copyPreview(uint16_t *, size_t, uint32_t &) { return false; }
}
#include "../../src/ui.cpp"
#include "../../src/pin_security.cpp"
#include "../../targets/waveshare_p4/main/ui_portrait.cpp"

static std::array<uint16_t, 480 * 800> frame;
static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *data) {
  auto *pixels = reinterpret_cast<uint16_t *>(data);
  for (int y = area->y1; y <= area->y2; ++y) for (int x = area->x1; x <= area->x2; ++x)
    frame[y * 480 + x] = *pixels++;
  lv_display_flush_ready(display);
}
static void snapshot(AuroraUI &ui, const char *name) {
  lv_obj_update_layout(ui.root_); lv_refr_now(nullptr);
  FILE *file = fopen(name, "wb"); assert(file);
  fprintf(file, "P6\n480 800\n255\n");
  for (uint16_t pixel : frame) {
    const uint8_t rgb[] = {static_cast<uint8_t>(((pixel >> 11) & 31) * 255 / 31),
        static_cast<uint8_t>(((pixel >> 5) & 63) * 255 / 63), static_cast<uint8_t>((pixel & 31) * 255 / 31)};
    fwrite(rgb, 1, sizeof(rgb), file);
  }
  fclose(file);
}
static void fixture(AuroraUI &ui) {
  ui.words_ = 24; ui.wallet_.valid = true;
  strlcpy(ui.wallet_.mnemonic, "abandon ability able about above absent absorb abstract absurd abuse access accident account accuse achieve acid acoustic acquire across act action actor actress actual", sizeof(ui.wallet_.mnemonic));
  strlcpy(ui.wallet_.address, "bc1qfixtureonlyneverusethisaddress0000000000000000", sizeof(ui.wallet_.address));
  strlcpy(ui.wallet_.accountXpub, "zpub-fixture-only-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789", sizeof(ui.wallet_.accountXpub));
  strlcpy(ui.wallet_.privateWif, "L1-fixture-only-not-a-real-private-key-0123456789", sizeof(ui.wallet_.privateWif));
  strlcpy(ui.wallet_.path, "m/84'/0'/0'/0/0", sizeof(ui.wallet_.path));
}
static void click(AuroraUI &ui, Action action) {
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_button_class) &&
       reinterpret_cast<uintptr_t>(lv_obj_get_user_data(child))==action) {
      lv_obj_send_event(child,LV_EVENT_CLICKED,nullptr); return;
    }
  }
  assert(false && "Expected button missing");
}
static void blocked(const AuroraUI &ui) {
  assert(ui.screen_==AuroraUI::Screen::SdRequired && !ui.keyboard_);
  assert(!ui.passArea_ && !ui.passConfirmArea_ && !ui.pinArea_ && !ui.pinConfirmArea_);
  assert(!ui.filePasswordArea_ && !ui.filePasswordConfirmArea_ && !ui.restoreWordArea_);
  for(auto *area:ui.verifyArea_) assert(!area);
}
int main() {
  char *allocation=static_cast<char *>(auroraUiAlloc(20)); assert(allocation);
  strcpy(allocation,"secret-copy");
  assert(!auroraUiRealloc(allocation,SIZE_MAX) && !strcmp(allocation,"secret-copy"));
  allocation=static_cast<char *>(auroraUiRealloc(allocation,40));
  assert(allocation && !strcmp(allocation,"secret-copy") && wipedAllocations==1);
  assert(!auroraUiRealloc(allocation,0) && wipedAllocations==2);
  lv_init();
  auto *display = lv_display_create(480, 800);
  static uint16_t buffer[480 * 40];
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
  lv_display_set_buffers(display, buffer, nullptr, sizeof(buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, flush);
  lv_tick_set_cb([]() -> uint32_t { return millis(); });
  static AuroraUI ui; ui.begin(); ui.selfTestPending_ = false;
  using Screen = AuroraUI::Screen;
  auto *input = lv_indev_create();
  lv_indev_set_type(input, LV_INDEV_TYPE_POINTER);
  lv_indev_set_display(input, display);
  lv_indev_set_read_cb(input, [](lv_indev_t *, lv_indev_data_t *data) {
    data->point.x = 100; data->point.y = 200; data->state = LV_INDEV_STATE_PRESSED;
  });
  fixture(ui); snapshot(ui, "splash.ppm");
  ui.show(Screen::Mode); snapshot(ui,"mode-portrait.ppm");
  unsigned modeButtons=0, modeLogos=0;
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_button_class)) {
      assert(lv_obj_get_width(child)==384 && lv_obj_get_height(child)==64);
      assert(lv_obj_get_x(child)==48 && lv_obj_get_y(child)==316+88*modeButtons);
      ++modeButtons;
    } else if(lv_obj_check_type(child,&lv_image_class)) {
      assert(lv_obj_get_x(child)==184 && lv_obj_get_y(child)==92);
      assert(lv_obj_get_width(child)==112 && lv_obj_get_height(child)==160);
      ++modeLogos;
    }
  }
  assert(modeButtons==4 && modeLogos==1);
  sdReady=false;
  const Action modeActions[]={NEW_WALLET,OPEN_WALLET,RESTORE_WALLET,RECOVER_UMBREL};
  const Screen modeDestinations[]={Screen::Setup,Screen::ImportName,Screen::RestoreSetup,Screen::UmbrelWarning};
  for(unsigned i=0;i<4;++i) {
    ui.show(Screen::Mode); const unsigned before=sdChecks; click(ui,modeActions[i]);
    assert(ui.screen_==modeDestinations[i] && sdChecks==before);
  }
  ui.show(Screen::Mode); click(ui,RECOVER_UMBREL); click(ui,UMBREL_CONTINUE);
  assert(ui.umbrelRecovery_ && ui.words_==24 && ui.screen_==Screen::RestoreWords);
  strlcpy(ui.restoreMnemonic_,"24-word aezeed test fixture",sizeof(ui.restoreMnemonic_));
  ui.show(Screen::UmbrelPassphrase); lv_obj_send_event(ui.keyboard_,LV_EVENT_READY,nullptr);
  mock.time+=101000; ui.tick(); assert(ui.screen_==Screen::PinSetup && ui.umbrelRootXprv_[0]);
  lv_textarea_set_text(ui.pinArea_,"2468"); lv_textarea_set_text(ui.pinConfirmArea_,"2468");
  ui.submitPinSetup(); assert(ui.screen_==Screen::UmbrelResult && ui.pinGuard_.enabled());
  click(ui,UMBREL_SHOW_XPRV); assert(ui.screen_==Screen::PinUnlock);
  lv_textarea_set_text(ui.pinArea_,"2468"); ui.submitPinUnlock(); assert(ui.screen_==Screen::UmbrelQr);
  mock.time+=15001000; ui.tick(); assert(ui.screen_==Screen::UmbrelResult);
  click(ui,LOCK_SESSION); assert(ui.screen_==Screen::Mode && !ui.umbrelRootXprv_[0]);
  for(auto screen:{Screen::Setup,Screen::RestoreSetup,Screen::ImportName,
                  Screen::Passphrase,Screen::RestoreWords,Screen::RestorePassphrase,
                  Screen::ImportPassword,Screen::PinSetup}) {
    ui.closeSession(); const unsigned before=sdChecks; ui.show(screen);
    assert(ui.screen_==screen && sdChecks==before);
  }
  for(auto screen:{Screen::Backup,Screen::ExportWarning,Screen::ExportName,Screen::ExportPassword}) {
    ui.closeSession(); sdReady=false; ui.show(screen); blocked(ui);
    assert(ui.afterSd_==screen); click(ui,RETRY_SD); blocked(ui);
    sdReady=true; click(ui,RETRY_SD); assert(ui.screen_==screen);
  }
  sdReady=false; ui.show(Screen::Backup); fixture(ui); blocked(ui);
  snapshot(ui,"sd-required.ppm"); click(ui,LOCK_SESSION);
  assert(ui.screen_==Screen::Mode && !ui.wallet_.valid && !ui.wallet_.mnemonic[0]);
  sdReady=true;
  for (auto screen : {Screen::Mode, Screen::Setup, Screen::Passphrase, Screen::Mnemonic, Screen::Info,
      Screen::Qr, Screen::Verify, Screen::Backup, Screen::ImportName, Screen::ImportPassword,
      Screen::RestoreSetup, Screen::RestoreWords, Screen::RestorePassphrase,
      Screen::UmbrelWarning, Screen::UmbrelPassphrase, Screen::UmbrelProcessing, Screen::UmbrelResult,
      Screen::ExportWarning, Screen::ExportName, Screen::ExportPassword}) {
    ui.show(screen); fixture(ui); lv_obj_update_layout(ui.root_);
    assert(lv_obj_get_width(ui.root_) == 480 && lv_obj_get_height(ui.root_) == 800);
    char name[40]; snprintf(name, sizeof(name), "screen-%02u.ppm", static_cast<unsigned>(screen)); snapshot(ui, name);
  }
  ui.show(Screen::Entropy); ui.onTouchSample(23, 200, 0); ui.onTouchSample(300, 341, 0);
  assert(ui.entropy_.sampleCount() == 0);
  snapshot(ui, "entropy-red.ppm");
  for (unsigned i = 1; i <= 320; ++i) {
    mock.time += 30000; ui.onTouchSample(50 + i % 300, 170 + i % 150, 0);
    if (i == 160) { ui.tick(); assert(lv_bar_get_value(ui.entropyBar_) == 50); snapshot(ui, "entropy-orange.ppm"); }
  }
  assert(ui.entropyReadyPending_ && lv_bar_get_value(ui.entropyBar_) == 100);
  snapshot(ui, "entropy-green.ppm");
  sdReady=false; const unsigned checksBeforeStop=sdChecks;
  std::array<uint8_t,32> collected{};
  static_assert(sizeof(ui.mixedEntropy_)==collected.size());
  memcpy(collected.data(),ui.mixedEntropy_,collected.size());
  AuroraSensors::acknowledge = false;
  mock.time += 1000000; ui.tick();
  assert(ui.sensorStopPending_ && ui.screen_ == Screen::Entropy && AuroraSensors::requested);
  mock.time += 6000000; ui.tick();
  assert(ui.screen_ == Screen::Entropy && ui.sensorStopPending_);
  assert(sdChecks==checksBeforeStop); // Never mount SD while capture is active.
  snapshot(ui, "sensor-stop-blocked.ppm");
  AuroraSensors::acknowledge = true; ui.tick();
  assert(ui.screen_==Screen::Passphrase && ui.entropyCollected_ && !ui.sensorStopPending_);
  assert(sdChecks==checksBeforeStop); // Collection and passphrase work without SD.
  assert(!memcmp(collected.data(),ui.mixedEntropy_,collected.size()));
  assert(ui.passArea_ && ui.passConfirmArea_);
  lv_textarea_set_text(ui.passArea_,"test"); lv_textarea_set_text(ui.passConfirmArea_,"typo");
  assert(!ui.confirmPassphrase() && ui.entropyCollected_ && !ui.passphrase_[0]);
  lv_textarea_set_text(ui.passArea_,"same"); lv_textarea_set_text(ui.passConfirmArea_,"same");
  assert(ui.confirmPassphrase() && !strcmp(ui.passphrase_,"same"));
  assert(sdChecks==checksBeforeStop);
  assert(ui.generate() && !ui.entropyCollected_);
  for(uint8_t byte:ui.mixedEntropy_) assert(!byte);
  sdReady=true;
  ui.show(Screen::Entropy); ui.onTouchSample(100, 200, 0);
  AuroraSensors::acknowledge = false; ui.show(Screen::Passphrase);
  assert(ui.screen_ == Screen::Entropy && !mock.rngEnabled);
  AuroraSensors::acknowledge = true; ui.tick();
  assert(ui.screen_ == Screen::Passphrase);
  ui.show(Screen::Entropy); assert(ui.entropy_.sampleCount() == 0);
  lv_obj_update_layout(ui.root_); lv_indev_read(input);
  assert(ui.entropy_.sampleCount() == 1); // Real PRESSING callback, not cached polling.
  for (int i = 0; i < 10; ++i) ui.tick();
  assert(ui.entropy_.sampleCount() == 1);
  mock.time += 30000; lv_indev_read(input);
  assert(ui.entropy_.sampleCount() == 2);
  ui.show(Screen::Mode); ui.tick();
  ui.show(Screen::RestoreWords); lv_textarea_set_text(ui.restoreWordArea_,"abandon");
  sdReady=false; unsigned offlineChecks=sdChecks;
  assert(ui.acceptRestoreWord(lv_textarea_get_text(ui.restoreWordArea_)));
  assert(ui.screen_==Screen::RestoreWords && !strcmp(ui.restoreWords_[0],"abandon") && ui.restoreWordIndex_==1);
  assert(sdChecks==offlineChecks);
  sdReady=true; ui.closeSession();
  fixture(ui); ui.show(Screen::Verify); sdReady=false;
  char verifyFixture[256]; strlcpy(verifyFixture,ui.wallet_.mnemonic,sizeof(verifyFixture));
  char *verifyNext=nullptr; unsigned wordIndex=0;
  for(char *word=strtok_r(verifyFixture," ",&verifyNext);word;word=strtok_r(nullptr," ",&verifyNext),++wordIndex)
    for(unsigned i=0;i<3;++i) if(ui.verifyIndex_[i]==wordIndex) lv_textarea_set_text(ui.verifyArea_[i],word);
  offlineChecks=sdChecks;
  assert(ui.verifyWords() && ui.screen_==Screen::Verify && sdChecks==offlineChecks);
  secureZero(verifyFixture,sizeof(verifyFixture));
  sdReady=true; ui.closeSession();
  for(auto screen:{Screen::ExportPassword}) {
    ui.show(screen); lv_textarea_set_text(ui.filePasswordArea_,"test-password-only");
    if(ui.filePasswordConfirmArea_) lv_textarea_set_text(ui.filePasswordConfirmArea_,"test-password-only");
    sdReady=false; lv_obj_send_event(ui.keyboard_,LV_EVENT_READY,nullptr); blocked(ui);
    assert(!ui.filePassword_[0] && ui.fileOperation_==AuroraUI::FileOperation::None);
    sdReady=true; click(ui,RETRY_SD); assert(ui.screen_==screen && !lv_textarea_get_text(ui.filePasswordArea_)[0]);
    ui.closeSession();
  }
  fixture(ui); ui.pinForExport_=true;
  strlcpy(ui.filePassword_,"test-password-only",sizeof(ui.filePassword_));
  ui.show(Screen::PinSetup);
  lv_textarea_set_text(ui.pinArea_,"1234"); lv_textarea_set_text(ui.pinConfirmArea_,"1234");
  sdReady=false; ui.submitPinSetup(); blocked(ui);
  assert(ui.afterSd_==Screen::ExportPassword && !ui.filePassword_[0] && !exportCalls);
  assert(!auroraPinRecordValid(ui.exportPin_) && !ui.pinGuard_.enabled());
  sdReady=true; click(ui,RETRY_SD); assert(ui.screen_==Screen::ExportPassword);
  ui.closeSession();
  // Removal between successful validation and the delayed operation screen.
  fixture(ui); ui.pinForExport_=true; ui.show(Screen::PinSetup);
  strlcpy(ui.filePassword_,"test-password-only",sizeof(ui.filePassword_));
  lv_textarea_set_text(ui.pinArea_,"1234"); lv_textarea_set_text(ui.pinConfirmArea_,"1234");
  sdFailOnCheck=sdChecks+2; ui.submitPinSetup(); blocked(ui);
  assert(ui.afterSd_==Screen::ExportPassword && !ui.fileOperationDueMs_ && !ui.filePassword_[0]);
  assert(!auroraPinRecordValid(ui.exportPin_) && !exportCalls);
  sdFailOnCheck=0; ui.closeSession();
  // Import uses the existing reader's inline error, not the save-only dialog.
  ui.show(Screen::ImportPassword); lv_textarea_set_text(ui.filePasswordArea_,"test-password-only");
  sdReady=false; offlineChecks=sdChecks;
  lv_obj_send_event(ui.keyboard_,LV_EVENT_READY,nullptr);
  assert(ui.screen_==Screen::FileProcessing && sdChecks==offlineChecks);
  mock.time+=101000; ui.tick();
  assert(importCalls==1 && ui.screen_==Screen::ImportPassword && ui.importStatus_[0] && !ui.filePassword_[0]);
  sdReady=true; ui.closeSession();
  fixture(ui); strlcpy(ui.passphrase_,"fixture-passphrase",sizeof(ui.passphrase_));
  AuroraPinRecord pin{}; assert(auroraPinCreate("01234567",pin));
  ui.pinGuard_.begin(pin); ui.protectedSession_=ui.loadedWallet_=true;
  lv_disp_trig_activity(nullptr);
  ui.show(Screen::Info); snapshot(ui,"protected-info.ppm");
  for(auto screen:{Screen::Mnemonic,Screen::PassphraseReveal,Screen::Verify,
                   Screen::ExportWarning,Screen::ExportName,Screen::ExportPassword}) {
    ui.show(screen); assert(ui.screen_==Screen::PinUnlock);
    ui.show(Screen::Info); assert(ui.access_==AuroraUI::Access::None);
  }
  ui.qrContent_=AuroraUI::QrContent::PrivateKey; ui.show(Screen::Qr);
  assert(ui.screen_==Screen::PinUnlock); snapshot(ui,"pin-unlock.ppm");
  lv_textarea_set_text(ui.pinArea_,"0000"); ui.submitPinUnlock();
  assert(ui.pinGuard_.failures()==1);
  lv_obj_send_event(ui.keyboard_,LV_EVENT_CANCEL,nullptr);
  assert(ui.screen_==Screen::Info);
  ui.show(Screen::Mnemonic);
  assert(ui.pinGuard_.failures()==1); mock.time+=1000000;
  sdReady=false; offlineChecks=sdChecks;
  lv_textarea_set_text(ui.pinArea_,"01234567"); lv_obj_send_event(ui.keyboard_,LV_EVENT_READY,nullptr);
  assert(ui.screen_==Screen::Mnemonic && ui.pinGuard_.failures()==1);
  assert(sdChecks==offlineChecks);
  ui.show(Screen::PassphraseReveal); assert(ui.screen_==Screen::PinUnlock);
  lv_textarea_set_text(ui.pinArea_,"01234567"); ui.submitPinUnlock();
  assert(ui.screen_==Screen::PassphraseReveal);
  mock.time+=15001000; ui.tick(); assert(ui.screen_==Screen::Info && !ui.passArea_);
  ui.show(Screen::Mnemonic); lv_textarea_set_text(ui.pinArea_,"0000"); ui.submitPinUnlock();
  mock.time+=1000000; lv_textarea_set_text(ui.pinArea_,"0000"); ui.submitPinUnlock();
  assert(ui.screen_==Screen::Mode && !ui.wallet_.valid && !ui.protectedSession_ && !ui.pinGuard_.enabled());
  assert(!ui.passphrase_[0] && !ui.wallet_.mnemonic[0] && !ui.wallet_.privateWif[0]);
  fixture(ui); ui.protectedSession_=true; ui.legacyImported_=true;
  ui.show(Screen::Mnemonic); assert(ui.screen_==Screen::PinSetup);
  snapshot(ui,"pin-setup.ppm");
  lv_textarea_set_text(ui.pinArea_,"1234"); lv_textarea_set_text(ui.pinConfirmArea_,"1235");
  ui.submitPinSetup(); assert(!ui.pinGuard_.enabled());
  lv_textarea_set_text(ui.pinArea_,"1234"); lv_textarea_set_text(ui.pinConfirmArea_,"1234");
  ui.submitPinSetup(); assert(ui.screen_==Screen::Info && ui.pinGuard_.enabled());
  assert(!sdReady && sdChecks==offlineChecks); // Session PIN also works offline.
  mock.time+=120001000; ui.tick(); assert(ui.screen_==Screen::Mode && !ui.wallet_.valid);
  sdReady=true;
  fixture(ui); importedWallet=ui.wallet_;
  importedData.fileVersion=2; importedData.pin=pin; importedData.wordCount=24;
  importedData.addressKind=static_cast<uint8_t>(ui.wallet_.kind);
  strlcpy(importedData.addressType,addressKindName(ui.wallet_.kind),sizeof(importedData.addressType));
  strlcpy(importedData.mnemonic,ui.wallet_.mnemonic,sizeof(importedData.mnemonic));
  strlcpy(importedData.derivationPath,ui.wallet_.path,sizeof(importedData.derivationPath));
  strlcpy(importedData.address,ui.wallet_.address,sizeof(importedData.address));
  strlcpy(importedData.accountXpub,ui.wallet_.accountXpub,sizeof(importedData.accountXpub));
  strlcpy(importedData.accountXprv,"test-xprv",sizeof(importedData.accountXprv));
  strlcpy(importedData.privateWif,ui.wallet_.privateWif,sizeof(importedData.privateWif));
  strlcpy(importedData.receiveDescriptor,ui.wallet_.watchDescriptor,sizeof(importedData.receiveDescriptor));
  ui.show(Screen::Mode); importOk=true;
  ui.fileOperation_=AuroraUI::FileOperation::Import;
  ui.show(Screen::FileProcessing); ui.fileOperationDueMs_=millis(); ui.tick();
  assert(ui.screen_==Screen::Info && ui.protectedSession_ && ui.pinGuard_.enabled());
  const unsigned derivations=xprvCalls;
  ui.performWalletExport(); assert(!exportCalls && xprvCalls==derivations);
  ui.show(Screen::ExportName); assert(ui.screen_==Screen::PinUnlock);
  lv_textarea_set_text(ui.pinArea_,"0000"); ui.submitPinUnlock();
  assert(ui.pinGuard_.failures()==1); mock.time+=1000000;
  lv_textarea_set_text(ui.pinArea_,"01234567"); sdReady=false; ui.submitPinUnlock(); blocked(ui);
  assert(ui.afterSd_==Screen::ExportName && ui.pinGuard_.failures()==1);
  assert(ui.access_==AuroraUI::Access::None);
  sdReady=true; click(ui,RETRY_SD);
  assert(ui.screen_==Screen::PinUnlock && !lv_textarea_get_text(ui.pinArea_)[0]);
  lv_textarea_set_text(ui.pinArea_,"01234567"); ui.submitPinUnlock();
  assert(ui.screen_==Screen::ExportName);
  ui.pinForExport_=true; ui.show(Screen::PinSetup);
  lv_textarea_set_text(ui.pinArea_,"9876"); lv_textarea_set_text(ui.pinConfirmArea_,"9876");
  exportOk=true; ui.submitPinSetup(); mock.time+=101000; ui.tick();
  assert(exportCalls==1 && ui.screen_==Screen::Info && !ui.filePassword_[0]);
  assert(auroraPinVerify("9876",ui.pinGuard_.record()) && !auroraPinRecordValid(ui.exportPin_));
  ui.show(Screen::ExportWarning); lv_textarea_set_text(ui.pinArea_,"9876"); ui.submitPinUnlock();
  mock.time+=120001000; ui.performWalletExport(); assert(exportCalls==1);
  ui.closeSession(); importedData.fileVersion=1; importedData.pin={};
  ui.fileOperation_=AuroraUI::FileOperation::Import;
  ui.show(Screen::FileProcessing); ui.fileOperationDueMs_=millis(); ui.tick();
  assert(ui.screen_==Screen::PinSetup && ui.legacyImported_);
  ui.closeSession(); importedData.fileVersion=2; importedData.pin=pin;
  importedData.address[0]='x';
  assert(!ui.performWalletImport() && !ui.wallet_.valid && !ui.passphrase_[0]);
  fixture(ui); ui.protectedSession_=true; ui.pinGuard_.begin(pin);
  lv_disp_trig_activity(nullptr); sdReady=false; ui.show(Screen::Backup); blocked(ui);
  mock.time+=120001000; ui.tick();
  assert(ui.screen_==Screen::Mode && !ui.wallet_.valid && !ui.pinGuard_.enabled());
  sdReady=true;
  puts("PASS: V2 import opens public info, V1 requires PIN setup, actual export entry point enforces grant and promotes new file PIN only on success");
  puts("PASS: centered P4 logo and 4-button menu including Umbrel recovery, offline creation/restoration/session PIN, SD required only for save/export");
  puts("PASS: passphrase confirmation, per-action PIN, persistent failure count, 3 errors close/wipe, secret/session timeouts, mandatory legacy PIN");
  assert(wipedAllocations>1000);
  puts("PASS: LVGL free/realloc allocations wiped before release, including earlier edited text copies");
  puts("PASS: native 480x800 screens, portrait collection, threshold, sensor stop interlock, cancel/restart");
  return 0;
}
