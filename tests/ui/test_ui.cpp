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
#include "platform/crypto.h"
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
static unsigned sessionReadCalls=0;
static bool sessionReadOk=true;
static bool sessionFileChanged=false;
static uint32_t accountXprvDelayMs=0;
static uint8_t mockFingerprint[32]{};
static uint32_t lifecycleTick=0, operationDelayMs=0;
static void consumeOperationDelay() { lifecycleTick+=operationDelayMs; operationDelayMs=0; }
static void advanceBlockingDelay(uint32_t &delayMs) {
  lifecycleTick+=delayMs; mock.time+=delayMs*1000; delayMs=0;
}
static bool sdReady=true;
static unsigned sdChecks=0, sdFailOnCheck=0;
bool auroraSdReady() { ++sdChecks; return sdReady && sdChecks!=sdFailOnCheck; }
WalletEngine::WalletEngine() {}
WalletSelfTest WalletEngine::selfTest() { return WalletSelfTest::Ok; }
bool WalletEngine::create(uint8_t, AddressKind, const char *, const uint8_t *, WalletOutput &) { consumeOperationDelay(); return true; }
bool WalletEngine::restore(const char *, uint8_t, AddressKind, const char *, WalletOutput &out) { consumeOperationDelay(); out=importedWallet; return true; }
bool WalletEngine::accountXprv(const WalletOutput &, const char *, char *out, size_t size) {
  advanceBlockingDelay(accountXprvDelayMs);
  ++xprvCalls; strlcpy(out,"test-xprv",size); return true;
}
bool WalletEngine::rootXprvFromSeed(const uint8_t *,size_t,char *out,size_t size) {
  strlcpy(out,"xprv9s21ZrQH143K3-test-only-not-a-real-master-private-key",size); return true;
}
AezeedResult AezeedEngine::decode(const char *,const char *,AezeedDecoded &out) {
  consumeOperationDelay();
  out.birthdayDays=4242; memset(out.entropy,0x42,sizeof(out.entropy)); return AezeedResult::Ok;
}
void AezeedEngine::wipe(AezeedDecoded &out) { secureZero(&out,sizeof(out)); }
void WalletEngine::wipe(WalletOutput &wallet) { secureZero(&wallet, sizeof(wallet)); }
bool WalletEngine::bip39Word(const char *) { return true; }
uint8_t WalletEngine::bip39Suggestions(const char *, char *, size_t, uint8_t) { return 0; }
const char *walletExportSuffix(WalletExportFormat kind) { return kind == WalletExportFormat::AuroraWallet ? ".aurora" : ".json"; }
bool auroraWalletCryptoSelfTest() { return true; }
void wipeAuroraWalletData(AuroraWalletData &data) { secureZero(&data, sizeof(data)); }
WalletExportResult writeWalletExportFile(WalletExportFormat format, const char *, const char *, const WalletExportData &data, char *path, size_t size) {
  consumeOperationDelay();
  ++exportCalls;
  if(format==WalletExportFormat::AuroraWallet) assert(!data.pin);
  strlcpy(path,"/fixture.aurora",size); return exportOk && sdReady?WalletExportResult::Ok:WalletExportResult::NoCard;
}
AuroraWalletReadResult readAuroraWalletFile(const char *, const char *, AuroraWalletData &out) {
  consumeOperationDelay();
  ++importCalls;
  wipeAuroraWalletData(out);
  if(!importOk || !sdReady) return AuroraWalletReadResult::NoCard;
  out=importedData; return AuroraWalletReadResult::Ok;
}
static void resetMockFileAccess() {
  for(size_t i=0;i<32;++i) mockFingerprint[i]=static_cast<uint8_t>(0xa0+i);
  sessionFileChanged=false; sessionReadOk=true;
}
AuroraWalletReadResult readAuroraWalletFileChecked(const char *name,const char *password,
    AuroraWalletData &out,uint8_t *fingerprint,const uint8_t *expected) {
  if(fingerprint) secureZero(fingerprint,32);
  wipeAuroraWalletData(out);
  if(expected) {
    ++sessionReadCalls; consumeOperationDelay();
    if(!sdReady || !sessionReadOk) return AuroraWalletReadResult::ReadFailed;
    if(sessionFileChanged || memcmp(expected,mockFingerprint,32) ||
       !password || strcmp(password,"test-password-only")) return AuroraWalletReadResult::AuthenticationFailed;
    out=importedData;
  } else {
    const auto result=readAuroraWalletFile(name,password,out);
    if(result!=AuroraWalletReadResult::Ok) return result;
  }
  if(fingerprint) memcpy(fingerprint,mockFingerprint,32);
  return AuroraWalletReadResult::Ok;
}
WalletExportResult writeAuroraWalletFileVerified(const char *name,const char *password,
    const WalletExportData &data,char *path,size_t size,uint8_t *fingerprint) {
  if(fingerprint) secureZero(fingerprint,32);
  const auto result=writeWalletExportFile(WalletExportFormat::AuroraWallet,name,password,data,path,size);
  if(result!=WalletExportResult::Ok) return result;
  importedData={}; importedData.fileVersion=1;
  importedData.addressKind=data.addressKind; importedData.wordCount=data.wordCount;
  strlcpy(importedData.addressType,data.addressType,sizeof(importedData.addressType));
  strlcpy(importedData.derivationPath,data.derivationPath,sizeof(importedData.derivationPath));
  strlcpy(importedData.mnemonic,data.mnemonic,sizeof(importedData.mnemonic));
  strlcpy(importedData.passphrase,data.passphrase,sizeof(importedData.passphrase));
  strlcpy(importedData.address,data.address,sizeof(importedData.address));
  strlcpy(importedData.accountXpub,data.accountXpub,sizeof(importedData.accountXpub));
  strlcpy(importedData.accountXprv,data.accountXprv,sizeof(importedData.accountXprv));
  strlcpy(importedData.privateWif,data.privateWif,sizeof(importedData.privateWif));
  strlcpy(importedData.receiveDescriptor,data.receiveDescriptor,sizeof(importedData.receiveDescriptor));
  ++mockFingerprint[0]; if(fingerprint) memcpy(fingerprint,mockFingerprint,32);
  return WalletExportResult::Ok;
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
static std::array<std::array<uint16_t,480*800>,3> panelFrames;
static unsigned panelWriteIndex=0, completedFrames=0;
static size_t flushedPixels=0;
static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *data) {
  auto *pixels = reinterpret_cast<uint16_t *>(data);
  for (int y = area->y1; y <= area->y2; ++y) for (int x = area->x1; x <= area->x2; ++x) {
    panelFrames[panelWriteIndex][y*480+x]=frame[y*480+x]=*pixels++;
    ++flushedPixels;
  }
  if(lv_display_flush_is_last(display)) {
    ++completedFrames; panelWriteIndex=(panelWriteIndex+1)%panelFrames.size();
  }
  lv_display_flush_ready(display);
}
static void assertDisplayReplaced(AuroraUI &ui) {
  assert(ui.displayRefreshPending_);
  // Model old secret pixels retained in each of the three panel buffers.
  for(auto &old:panelFrames) old.fill(0xFFFF);
  const auto beforePixels=flushedPixels;
  const auto beforeFrames=completedFrames;
  ui.refreshDisplayAfterClear();
  assert(!ui.displayRefreshPending_);
  assert(completedFrames-beforeFrames==3 && flushedPixels-beforePixels==3*480*800);
  for(const auto &rendered:panelFrames) assert(rendered==frame);
  const auto after=flushedPixels; ui.refreshDisplayAfterClear(); assert(flushedPixels==after);
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
  strlcpy(ui.wallet_.privateDescriptor, "pkh(private-fixture-only)", sizeof(ui.wallet_.privateDescriptor));
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
  fprintf(stderr,"Missing action %u on screen %u\n",unsigned(action),unsigned(ui.screen_));
  assert(false && "Expected button missing");
}
static lv_obj_t *actionButton(AuroraUI &ui, Action action) {
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_button_class) &&
       reinterpret_cast<uintptr_t>(lv_obj_get_user_data(child))==action) return child;
  }
  return nullptr;
}
static void assertSessionWiped(const AuroraUI &ui) {
  const auto zero=[](const void *data,size_t size) {
    const auto *bytes=static_cast<const uint8_t *>(data);
    for(size_t i=0;i<size;++i) assert(bytes[i]==0);
  };
  zero(&ui.wallet_,sizeof(ui.wallet_));
  zero(ui.passphrase_,sizeof(ui.passphrase_));
  zero(ui.filePassword_,sizeof(ui.filePassword_));
  zero(ui.mixedEntropy_,sizeof(ui.mixedEntropy_));
  zero(ui.restoreWords_,sizeof(ui.restoreWords_));
  zero(ui.restoreMnemonic_,sizeof(ui.restoreMnemonic_));
  zero(ui.restoreSuggestions_,sizeof(ui.restoreSuggestions_));
  zero(ui.verifySuggestions_,sizeof(ui.verifySuggestions_));
  zero(ui.umbrelRootXprv_,sizeof(ui.umbrelRootXprv_));
  zero(ui.sessionFingerprint_,sizeof(ui.sessionFingerprint_));
  zero(ui.sessionBaseName_,sizeof(ui.sessionBaseName_));
  assert(!ui.fileSession_ && !ui.privateLoaded_ && !ui.sessionHasPassphrase_);
  assert(!ui.sensitiveStateActive_ && !ui.protectedSession_);
  assert(ui.access_==AuroraUI::Access::None && ui.requestedAccess_==AuroraUI::Access::None);
  assert(ui.fileOperation_==AuroraUI::FileOperation::None && !ui.fileOperationDueMs_);
  assert(!ui.generationDueMs_ && !ui.entropyCollected_);
}
static void assertPrivateStateAbsent(const AuroraUI &ui) {
  const auto zero=[](const void *data,size_t size) {
    const auto *bytes=static_cast<const uint8_t *>(data);
    for(size_t i=0;i<size;++i) assert(!bytes[i]);
  };
  zero(ui.wallet_.mnemonic,sizeof(ui.wallet_.mnemonic));
  zero(ui.wallet_.privateWif,sizeof(ui.wallet_.privateWif));
  zero(ui.wallet_.privateDescriptor,sizeof(ui.wallet_.privateDescriptor));
  zero(ui.passphrase_,sizeof(ui.passphrase_));
  zero(ui.filePassword_,sizeof(ui.filePassword_));
  assert(!ui.privateLoaded_);
}
static void assertPublicFileSession(const AuroraUI &ui) {
  assert(ui.wallet_.valid && ui.protectedSession_ && ui.fileSession_);
  assertPrivateStateAbsent(ui);
  const auto contains=[](const void *haystack,size_t length,const uint8_t *needle,size_t count) {
    const auto *bytes=static_cast<const uint8_t *>(haystack);
    for(size_t i=0;i+count<=length;++i) if(!memcmp(bytes+i,needle,count)) return true;
    return false;
  };
  if(auroraPinRecordValid(importedData.pin))
    assert(!contains(&ui,sizeof(ui),importedData.pin.verifier,sizeof(importedData.pin.verifier)));
}
static void prepareImportedFixture(AuroraUI &ui,const AuroraPinRecord &pin,const char *passphrase="") {
  ui.closeSession(); fixture(ui); importedWallet=ui.wallet_;
  importedData={}; importedData.fileVersion=2; importedData.pin=pin; importedData.wordCount=ui.words_;
  importedData.addressKind=static_cast<uint8_t>(ui.wallet_.kind);
  strlcpy(importedData.addressType,addressKindName(ui.wallet_.kind),sizeof(importedData.addressType));
  strlcpy(importedData.mnemonic,ui.wallet_.mnemonic,sizeof(importedData.mnemonic));
  strlcpy(importedData.passphrase,passphrase,sizeof(importedData.passphrase));
  strlcpy(importedData.derivationPath,ui.wallet_.path,sizeof(importedData.derivationPath));
  strlcpy(importedData.address,ui.wallet_.address,sizeof(importedData.address));
  strlcpy(importedData.accountXpub,ui.wallet_.accountXpub,sizeof(importedData.accountXpub));
  strlcpy(importedData.accountXprv,"test-xprv",sizeof(importedData.accountXprv));
  strlcpy(importedData.privateWif,ui.wallet_.privateWif,sizeof(importedData.privateWif));
  strlcpy(importedData.receiveDescriptor,ui.wallet_.watchDescriptor,sizeof(importedData.receiveDescriptor));
  ui.show(AuroraUI::Screen::Mode); resetMockFileAccess();
  importOk=true; sdReady=true;
  strlcpy(ui.importBaseName_,"fixture",sizeof(ui.importBaseName_));
  strlcpy(ui.filePassword_,"test-password-only",sizeof(ui.filePassword_));
}
static void runMockImport(AuroraUI &ui) {
  ui.fileOperation_=AuroraUI::FileOperation::Import;
  ui.show(AuroraUI::Screen::FileProcessing); ui.fileOperationDueMs_=millis(); ui.tick();
}
static void blocked(const AuroraUI &ui) {
  assert(ui.screen_==AuroraUI::Screen::SdRequired && !ui.keyboard_);
  assert(!ui.passArea_ && !ui.passConfirmArea_);
  assert(!ui.filePasswordArea_ && !ui.filePasswordConfirmArea_ && !ui.restoreWordArea_);
  for(auto *area:ui.verifyArea_) assert(!area);
}
static void assertHeaderSeparation(AuroraUI &ui) {
  lv_obj_update_layout(ui.root_);
  const int dividerTop=AuroraLayout::y(34);
  const int dividerHeight=AuroraLayout::y(1);
  const int dividerBottom=dividerTop+dividerHeight-1;
  unsigned dividers=0;
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_get_x(child)==AuroraLayout::x(10) &&
        lv_obj_get_y(child)==dividerTop && lv_obj_get_width(child)==AuroraLayout::x(300) &&
        lv_obj_get_height(child)==dividerHeight) ++dividers;
  }
  // Processing-only screens intentionally have no header or divider.
  if(!dividers) return;
  assert(dividers==1);
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    const int top=lv_obj_get_y(child);
    const int height=lv_obj_get_height(child);
    const bool divider=lv_obj_get_x(child)==AuroraLayout::x(10) &&
        top==dividerTop && lv_obj_get_width(child)==AuroraLayout::x(300) &&
        height==dividerHeight;
    if(divider) continue;
    // Header objects stay above the divider. Screen information and controls
    // keep a visible gap below it and may never cover or cut the line.
    assert(top+height-1<dividerTop || top>=dividerBottom+6);
  }
}
static void testTransientWordCopies(AuroraUI &ui) {
  using Screen=AuroraUI::Screen;
  const auto zero=[](const void *pointer,size_t size) {
    const auto *bytes=static_cast<const uint8_t *>(pointer);
    for(size_t i=0;i<size;++i) assert(bytes[i]==0);
  };
  ui.closeSession(); sdReady=true;
  ui.show(Screen::RestoreWords);
  memset(ui.restoreSuggestions_,0x52,sizeof(ui.restoreSuggestions_));
  memset(ui.verifySuggestions_,0x56,sizeof(ui.verifySuggestions_));
  ui.restoreSuggestionCount_=ui.verifySuggestionCount_=3;
  ui.show(Screen::RestorePassphrase);
  zero(ui.restoreSuggestions_,sizeof(ui.restoreSuggestions_));
  zero(ui.verifySuggestions_,sizeof(ui.verifySuggestions_));
  assert(!ui.restoreSuggestionCount_ && !ui.verifySuggestionCount_);

  // A successful restore releases every old entry copy even before show()
  // destroys the input widgets. Its returned wallet remains available.
  fixture(ui); importedWallet=ui.wallet_;
  strlcpy(ui.restoreMnemonic_,ui.wallet_.mnemonic,sizeof(ui.restoreMnemonic_));
  strlcpy(ui.restoreWords_[0],"abandon",sizeof(ui.restoreWords_[0]));
  memset(ui.restoreSuggestions_,0x53,sizeof(ui.restoreSuggestions_));
  ui.restoreSuggestionCount_=3;
  assert(ui.restoreEnteredWallet() && ui.wallet_.valid);
  zero(ui.restoreWords_,sizeof(ui.restoreWords_));
  zero(ui.restoreMnemonic_,sizeof(ui.restoreMnemonic_));
  zero(ui.restoreSuggestions_,sizeof(ui.restoreSuggestions_));
  assert(!ui.restoreSuggestionCount_ && ui.wallet_.mnemonic[0]);

  // Back keeps per-word editing possible, but not an obsolete full phrase.
  ui.closeSession(); ui.show(Screen::RestoreWords);
  strlcpy(ui.restoreWords_[0],"abandon",sizeof(ui.restoreWords_[0]));
  strlcpy(ui.restoreMnemonic_,"abandon ability",sizeof(ui.restoreMnemonic_));
  click(ui,BACK_RESTORE_SETUP);
  assert(ui.screen_==Screen::RestoreSetup);
  zero(ui.restoreMnemonic_,sizeof(ui.restoreMnemonic_));
  assert(!strcmp(ui.restoreWords_[0],"abandon"));
  for(Action count:{WORD_12,WORD_15,WORD_18,WORD_21,WORD_24}) {
    strlcpy(ui.restoreWords_[0],"ability",sizeof(ui.restoreWords_[0]));
    strlcpy(ui.restoreMnemonic_,"abandon ability",sizeof(ui.restoreMnemonic_));
    click(ui,count);
    zero(ui.restoreWords_,sizeof(ui.restoreWords_));
    zero(ui.restoreMnemonic_,sizeof(ui.restoreMnemonic_));
    assert(ui.words_==12+3*(count-WORD_12));
  }
  // Clearing a stale concatenation must not break rebuilding the final input.
  ui.words_=12; ui.restoreWordIndex_=11;
  for(unsigned i=0;i<11;++i) strlcpy(ui.restoreWords_[i],"abandon",sizeof(ui.restoreWords_[i]));
  ui.show(Screen::RestoreWords);
  assert(ui.acceptRestoreWord("about"));
  assert(ui.screen_==Screen::RestorePassphrase &&
      !strcmp(ui.restoreMnemonic_,"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"));
  ui.closeSession(); assertSessionWiped(ui);
  puts("PASS: P4 word suggestions erased at screen changes and restore success; stale full phrases erased on back/count changes and reconstructed only after final word");
}

static void enterPrivatePassword(AuroraUI &ui,const char *password="test-password-only") {
  assert(ui.screen_==AuroraUI::Screen::PrivatePassword);
  lv_textarea_set_text(ui.filePasswordArea_,password);
  lv_obj_send_event(ui.keyboard_,LV_EVENT_READY,nullptr);
  if(ui.screen_==AuroraUI::Screen::FileProcessing) { mock.time+=101000; ui.tick(); }
}
static void testPasswordFileSessions(AuroraUI &ui) {
  using Screen=AuroraUI::Screen;
  AuroraPinRecord legacyPin{}; assert(auroraPinCreate("01234567",legacyPin));
  for(uint8_t version:{1,2}) {
    prepareImportedFixture(ui,legacyPin,"fixture-passphrase");
    importedData.fileVersion=version;
    if(version==1) secureZero(&importedData.pin,sizeof(importedData.pin));
    runMockImport(ui); assert(ui.screen_==Screen::Info); assertPublicFileSession(ui);
    for(Action action:{REVEAL_PRIVATE,SHOW_WORDS,SHOW_LOADED_PASSPHRASE}) {
      auto *button=actionButton(ui,action); assert(button);
      assert(lv_color_eq(lv_obj_get_style_bg_color(button,LV_PART_MAIN),DANGER));
    }
    auto *publicButton=actionButton(ui,TO_QR_ADDRESS); assert(publicButton);
    assert(lv_color_eq(lv_obj_get_style_bg_color(publicButton,LV_PART_MAIN),SUCCESS));
    snapshot(ui,"password-public-info.ppm");
    for(auto destination:{Screen::Mnemonic,Screen::Verify,Screen::PassphraseReveal,
        Screen::ExportWarning,Screen::ExportName,Screen::ExportPassword}) {
      ui.show(destination); assert(ui.screen_==Screen::PrivatePassword);
      assertPrivateStateAbsent(ui);
      ui.show(Screen::Info); assertPublicFileSession(ui);
    }
    // Public metadata stays accessible without a secret or a PIN form.
    click(ui,TO_QR_ADDRESS); assert(ui.screen_==Screen::Qr); assertPublicFileSession(ui);
    click(ui,TO_INFO);
    click(ui,SHOW_WORDS); assert(ui.screen_==Screen::PrivatePassword);
    assertPublicFileSession(ui); snapshot(ui,"private-password.ppm");
    enterPrivatePassword(ui,"wrong-password-only");
    assert(ui.screen_==Screen::PrivatePassword && ui.importStatus_[0]); assertPublicFileSession(ui);
    enterPrivatePassword(ui);
    assert(ui.screen_==Screen::Mnemonic && ui.privateLoaded_ && ui.wallet_.mnemonic[0]);
    assert(!ui.filePassword_[0]);
    assert(!actionButton(ui,BACK_MODE_WIPE) && !actionButton(ui,BACK_ENTROPY));
    auto *back=actionButton(ui,TO_INFO); assert(back);
    assert(!strcmp(lv_label_get_text(lv_obj_get_child(back,0)),"RETOUR"));
    const auto grant=ui.accessGrantedMs_;
    click(ui,MNEMONIC_NEXT); assert(ui.mnemonicPage_==1 && ui.accessGrantedMs_==grant);
    click(ui,TO_INFO); assertPublicFileSession(ui); assertDisplayReplaced(ui);
    for(Action action:{SHOW_LOADED_PASSPHRASE,REVEAL_PRIVATE,SHOW_WORDS}) {
      click(ui,action); assert(ui.screen_==Screen::PrivatePassword);
      // Cancel destroys edited password copies too.
      lv_textarea_set_text(ui.filePasswordArea_,"cancel-password-only");
      lv_obj_send_event(ui.keyboard_,LV_EVENT_CANCEL,nullptr);
      assert(ui.screen_==Screen::Info); assertPublicFileSession(ui); assertDisplayReplaced(ui);
      click(ui,action); enterPrivatePassword(ui);
      assert(ui.privateLoaded_ && !ui.filePassword_[0]);
      mock.time+=15001000; ui.tick();
      assert(ui.screen_==Screen::Info); assertPublicFileSession(ui);
    }
    // Switching private categories cannot reuse the preceding authorization.
    click(ui,SHOW_WORDS); enterPrivatePassword(ui);
    ui.show(Screen::PassphraseReveal);
    assert(ui.screen_==Screen::PrivatePassword); assertPublicFileSession(ui);
    enterPrivatePassword(ui); assert(ui.screen_==Screen::PassphraseReveal);
    click(ui,TO_INFO);
    // No SD / substituted file / authenticated but inconsistent wallet.
    click(ui,SHOW_WORDS); sdReady=false;
    lv_textarea_set_text(ui.filePasswordArea_,"test-password-only");
    ui.submitPrivatePassword(); blocked(ui); assertPrivateStateAbsent(ui);
    sdReady=true; click(ui,RETRY_SD); assert(ui.screen_==Screen::PrivatePassword);
    for(unsigned failure=0;failure<3;++failure) {
      sessionFileChanged=failure==0; sessionReadOk=failure!=1;
      const char original=importedData.accountXprv[0];
      if(failure==2) importedData.accountXprv[0]='X';
      enterPrivatePassword(ui); assert(ui.screen_==Screen::PrivatePassword);
      assertPrivateStateAbsent(ui);
      importedData.accountXprv[0]=original;
    }
    sessionFileChanged=false; sessionReadOk=true;
    enterPrivatePassword(ui); assert(ui.screen_==Screen::Mnemonic);
    click(ui,TO_INFO); assertPublicFileSession(ui);
    // Slow work cannot renew a private grant or the idle deadline.
    click(ui,SHOW_WORDS); accountXprvDelayMs=AuroraUI::SECRET_VISIBLE_MS;
    enterPrivatePassword(ui); assert(ui.screen_==Screen::Info); assertPrivateStateAbsent(ui);
    click(ui,SHOW_WORDS); accountXprvDelayMs=AuroraUI::SESSION_IDLE_MS;
    enterPrivatePassword(ui); assert(ui.screen_==Screen::Mode); assertSessionWiped(ui);
  }
  prepareImportedFixture(ui,legacyPin); runMockImport(ui);
  auto *noPassphrase=actionButton(ui,SHOW_LOADED_PASSPHRASE); assert(noPassphrase);
  assert(lv_obj_has_state(noPassphrase,LV_STATE_DISABLED));
  assert(!lv_obj_has_flag(noPassphrase,LV_OBJ_FLAG_HIDDEN));
  // A direct protected write with no password authorization must do nothing.
  const unsigned blockedWrites=exportCalls,blockedDerivations=xprvCalls;
  ui.performWalletExport();
  assert(!ui.exportSucceeded_ && exportCalls==blockedWrites && xprvCalls==blockedDerivations);
  assertPrivateStateAbsent(ui);
  // New file: password+confirmation directly schedules export, no PIN.
  ui.closeSession(); fixture(ui); importedWallet=ui.wallet_;
  strlcpy(ui.passphrase_,"fixture-passphrase",sizeof(ui.passphrase_));
  exportOk=true; sdReady=true;
  ui.show(Screen::ExportPassword);
  lv_textarea_set_text(ui.filePasswordArea_,"test-password-only");
  lv_textarea_set_text(ui.filePasswordConfirmArea_,"test-password-only");
  lv_obj_send_event(ui.keyboard_,LV_EVENT_READY,nullptr);
  assert(ui.screen_==Screen::FileProcessing);
  mock.time+=101000; ui.tick();
  assert(ui.exportSucceeded_ && ui.screen_==Screen::Info && importedData.fileVersion==1);
  assertPublicFileSession(ui);
  // Re-export needs the OLD file password, then asks separately for a NEW one.
  ui.show(Screen::ExportName); assert(ui.screen_==Screen::PrivatePassword);
  enterPrivatePassword(ui); assert(ui.screen_==Screen::ExportName);
  ui.show(Screen::ExportPassword); assert(ui.privateLoaded_);
  lv_textarea_set_text(ui.filePasswordArea_,"different-password-only");
  lv_textarea_set_text(ui.filePasswordConfirmArea_,"mismatched-password-only");
  lv_obj_send_event(ui.keyboard_,LV_EVENT_READY,nullptr);
  assert(ui.screen_==Screen::ExportPassword && !ui.filePassword_[0]);
  // Failed write and expiry both return to public, empty private state.
  exportOk=false;
  lv_textarea_set_text(ui.filePasswordArea_,"test-password-only");
  lv_textarea_set_text(ui.filePasswordConfirmArea_,"test-password-only");
  lv_obj_send_event(ui.keyboard_,LV_EVENT_READY,nullptr);
  mock.time+=101000; ui.tick(); assertPrivateStateAbsent(ui);
  ui.show(Screen::Info);
  ui.show(Screen::ExportWarning); enterPrivatePassword(ui);
  const unsigned before=exportCalls;
  accountXprvDelayMs=AuroraUI::EXPORT_AUTH_MS;
  ui.performWalletExport(); assert(exportCalls==before); assertPrivateStateAbsent(ui);
  ui.closeSession();
  // Explicit Electrum plaintext export remains available from an unsaved wallet.
  fixture(ui); ui.show(Screen::ExportName);
  ui.exportFormat_=WalletExportFormat::ElectrumPrivate; exportOk=true;
  ui.performWalletExport(); assert(ui.exportSucceeded_);
  ui.closeSession(); assertSessionWiped(ui);
  for(unsigned route=0;route<4;++route) {
    prepareImportedFixture(ui,legacyPin); runMockImport(ui);
    click(ui,SHOW_WORDS); enterPrivatePassword(ui);
    if(route==0) ui.closeSession();
    if(route==1) { mock.time+=120001000; ui.tick(); }
    if(route==2) { ui.begin(); ui.selfTestPending_=false; }
    if(route==3) { ui.emergencyWipeSecrets(); ui.clear(); }
    assertSessionWiped(ui); assertDisplayReplaced(ui);
  }
  puts("PASS: password-only V1/V2 import, private reauthentication, no retained password/PIN/key/capsule, cancellation/expiry/lock/boot cleanup");
  puts("PASS: file identity binding, SD removal/errors, new pinless export and existing Electrum exception");
}

static void testSensitiveLifecycle(AuroraUI &ui) {
  using Screen=AuroraUI::Screen;
  AuroraSensors::acknowledge=true;
  sdReady=true;
  ui.closeSession(); ui.tick();
  // Independent LVGL tick also exercises wraparound without relying on the
  // microsecond hardware stub's much shorter 32-bit range.
  lv_tick_set_cb([]() -> uint32_t { return lifecycleTick; });
  unsigned workflows=0;
  for(unsigned i=0;i<=static_cast<unsigned>(Screen::SdRequired);++i) {
    const auto screen=static_cast<Screen>(i);
    if(screen==Screen::Splash || screen==Screen::Mode || screen==Screen::Wipe ||
        screen==Screen::SecurityError) continue;
    ui.closeSession(); ui.tick(); fixture(ui);
    lifecycleTick+=1000;
    ui.show(screen);
    assert(ui.screen_==screen && ui.sensitiveStateActive_ && !ui.protectedSession_);
    // The same rule covers partially typed secrets and errors, not only a
    // successful wallet or password validation. These are public fixtures.
    if(ui.restoreWordArea_) lv_textarea_set_text(ui.restoreWordArea_,"abandon");
    if(ui.passArea_) lv_textarea_set_text(ui.passArea_,"fixture-passphrase");
    if(ui.filePasswordArea_) lv_textarea_set_text(ui.filePasswordArea_,"fixture-password");
    strlcpy(ui.restoreWords_[0],"ability",sizeof(ui.restoreWords_[0]));
    strlcpy(ui.restoreMnemonic_,"abandon ability",sizeof(ui.restoreMnemonic_));
    strlcpy(ui.umbrelRootXprv_,"test-only-xprv",sizeof(ui.umbrelRootXprv_));
    lifecycleTick+=AuroraUI::SESSION_IDLE_MS-1;
    ui.tick(); assert(ui.screen_==screen && ui.sensitiveStateActive_);
    ++lifecycleTick; ui.tick();
    assert(ui.screen_==Screen::Mode);
    assertSessionWiped(ui);
    ui.tick(); assert(!ui.sensorStopPending_);
    ++workflows;
  }
  assert(workflows==static_cast<unsigned>(Screen::SdRequired)+1-4);

  // Returning to a setup or error page is not a new session and may not renew
  // the original deadline. Genuine LVGL user activity does renew inactivity.
  ui.show(Screen::RestoreWords);
  lv_textarea_set_text(ui.restoreWordArea_,"abandon");
  assert(ui.acceptRestoreWord(lv_textarea_get_text(ui.restoreWordArea_)));
  lifecycleTick+=90000; ui.show(Screen::RestoreSetup);
  lifecycleTick+=30000; ui.tick(); assertSessionWiped(ui);
  ui.show(Screen::ImportPassword);
  lv_textarea_set_text(ui.filePasswordArea_,"fixture-password");
  lifecycleTick+=119999; lv_disp_trig_activity(nullptr);
  ++lifecycleTick; ui.tick(); assert(ui.screen_==Screen::ImportPassword);
  lifecycleTick+=119999; ui.tick(); assertSessionWiped(ui);

  lifecycleTick=UINT32_MAX-60000;
  ui.show(Screen::UmbrelPassphrase);
  lv_textarea_set_text(ui.passArea_,"fixture-passphrase");
  lifecycleTick+=119999; ui.tick(); assert(ui.screen_==Screen::UmbrelPassphrase);
  ++lifecycleTick; ui.tick(); assertSessionWiped(ui);

  // Expiration wins over a pending import/export, even if its due time has
  // elapsed. Neither storage nor derivation may run with expired credentials.
  for(auto operation:{AuroraUI::FileOperation::Import,AuroraUI::FileOperation::Export}) {
    ui.show(Screen::FileProcessing);
    ui.fileOperation_=operation; ui.fileOperationDueMs_=millis();
    const auto reads=importCalls, writes=exportCalls, derivations=xprvCalls;
    lifecycleTick+=AuroraUI::SESSION_IDLE_MS;
    ui.tick(); assertSessionWiped(ui);
    assert(importCalls==reads && exportCalls==writes && xprvCalls==derivations);
  }

  // Screen transitions must check expiration themselves, before building
  // another sensitive page, without waiting for the next periodic tick.
  for(auto destination:{Screen::Mnemonic,Screen::PrivatePassword,Screen::Info,Screen::GenerationError,Screen::SdRequired}) {
    ui.show(Screen::RestoreWords); fixture(ui);
    lifecycleTick+=AuroraUI::SESSION_IDLE_MS;
    ui.show(destination); assert(ui.screen_==Screen::Mode); assertSessionWiped(ui);
  }
  ui.show(Screen::RestoreWords); lifecycleTick+=AuroraUI::SESSION_IDLE_MS;
  ui.show(Screen::SecurityError); assert(ui.screen_==Screen::SecurityError); assertSessionWiped(ui);
  for(auto form:{Screen::Passphrase,Screen::RestorePassphrase,Screen::UmbrelPassphrase}) {
    ui.show(form); ui.entropyCollected_=true;
    lifecycleTick+=AuroraUI::SESSION_IDLE_MS;
    lv_obj_send_event(ui.keyboard_,LV_EVENT_READY,nullptr);
    assert(ui.screen_==Screen::Mode); assertSessionWiped(ui);
  }

  // Simulate a blocking operation consuming the remaining deadline. Its
  // successful return must not count as activity or briefly display secrets.
  for(auto processing:{Screen::Generating,Screen::Restoring,Screen::UmbrelProcessing}) {
    ui.closeSession(); fixture(ui); importedWallet=ui.wallet_;
    ui.umbrelRecovery_=processing==Screen::UmbrelProcessing;
    ui.show(processing); ui.entropyCollected_=true; ui.generationDueMs_=millis();
    operationDelayMs=AuroraUI::SESSION_IDLE_MS;
    ui.tick(); assert(!operationDelayMs && ui.screen_==Screen::Mode); assertSessionWiped(ui);
  }
  fixture(ui); importedWallet=ui.wallet_;
  importedData={}; importedData.fileVersion=2; importedData.wordCount=ui.words_;
  importedData.addressKind=static_cast<uint8_t>(ui.wallet_.kind);
  assert(auroraPinCreate("1234",importedData.pin));
  strlcpy(importedData.addressType,addressKindName(ui.wallet_.kind),sizeof(importedData.addressType));
  strlcpy(importedData.mnemonic,ui.wallet_.mnemonic,sizeof(importedData.mnemonic));
  strlcpy(importedData.derivationPath,ui.wallet_.path,sizeof(importedData.derivationPath));
  strlcpy(importedData.address,ui.wallet_.address,sizeof(importedData.address));
  strlcpy(importedData.accountXpub,ui.wallet_.accountXpub,sizeof(importedData.accountXpub));
  strlcpy(importedData.accountXprv,"test-xprv",sizeof(importedData.accountXprv));
  strlcpy(importedData.privateWif,ui.wallet_.privateWif,sizeof(importedData.privateWif));
  strlcpy(importedData.receiveDescriptor,ui.wallet_.watchDescriptor,sizeof(importedData.receiveDescriptor));
  for(auto operation:{AuroraUI::FileOperation::Import,AuroraUI::FileOperation::Export}) {
    for(bool succeeds:{false,true}) {
      ui.closeSession(); fixture(ui);
      importOk=exportOk=succeeds;
      ui.fileOperation_=operation; ui.show(Screen::FileProcessing); ui.fileOperationDueMs_=millis();
      const auto calls=operation==AuroraUI::FileOperation::Import?importCalls:exportCalls;
      operationDelayMs=AuroraUI::SESSION_IDLE_MS;
      ui.tick(); assert(!operationDelayMs && ui.screen_==Screen::Mode); assertSessionWiped(ui);
      assert((operation==AuroraUI::FileOperation::Import?importCalls:exportCalls)==calls+1);
    }
  }

  // A stalled sensor-stop worker cannot postpone cleanup or later resume the
  // old destination. Both explicit lock and idle expiration wipe immediately.
  for(bool explicitLock:{false,true}) {
    ui.show(Screen::Entropy); ui.onTouchSample(100,200,0);
    memset(ui.mixedEntropy_,0x42,sizeof(ui.mixedEntropy_)); ui.entropyCollected_=true;
    AuroraSensors::acknowledge=false;
    ui.show(Screen::Passphrase);
    assert(ui.sensorStopPending_ && ui.afterSensorStop_==Screen::Passphrase);
    if(explicitLock) ui.closeSession();
    else { lifecycleTick+=AuroraUI::SESSION_IDLE_MS; ui.tick(); }
    assertSessionWiped(ui);
    assert(ui.screen_==Screen::Mode && ui.sensorStopPending_);
    assert(ui.afterSensorStop_==Screen::Mode && !ui.cameraPixels_ && !mock.rngEnabled);
    assertDisplayReplaced(ui);
    click(ui,NEW_WALLET); assert(ui.screen_==Screen::Mode); // Input stays blocked.
    AuroraSensors::acknowledge=true; ui.tick();
    assert(ui.screen_==Screen::Mode && !ui.sensorStopPending_);
  }

  // A security error wipes immediately and must never expire into an enabled
  // menu. Reinitializing the UI likewise explicitly destroys the old session.
  fixture(ui); ui.show(Screen::SecurityError); assertSessionWiped(ui);
  lifecycleTick+=180000; ui.tick(); assert(ui.screen_==Screen::SecurityError);
  ui.show(Screen::RestoreWords); lv_textarea_set_text(ui.restoreWordArea_,"abandon");
  fixture(ui); ui.begin(); ui.selfTestPending_=false;
  assert(ui.screen_==Screen::Splash); assertSessionWiped(ui);
  assertDisplayReplaced(ui);

  // Emergency cleanup is memory-only and leaves the object reusable at boot.
  fixture(ui); ui.show(Screen::Info); ui.entropy_.begin();
  ui.entropy_.add(100,200,0);
  strlcpy(ui.passphrase_,"fixture-passphrase",sizeof(ui.passphrase_));
  const auto releases=wipedAllocations;
  ui.emergencyWipeSecrets();
  assertSessionWiped(ui);
  assert(wipedAllocations==releases && mock.rngEnabled);
  assert(ui.entropy_.sampleCount()==0 && ui.entropy_.previewToken()==0);
  ui.closeSession(); assert(!mock.rngEnabled);
  lv_tick_set_cb([]() -> uint32_t { return millis(); });
  lv_disp_trig_activity(nullptr);
  puts("PASS: all P4 workflows expire from entry at 120 s, including initial words/passwords, errors, back routes, deadline and tick wraparound");
  puts("PASS: explicit/idle lock wipes immediately during stalled sensor shutdown; deferred secret screen and operations cannot resume");
  puts("PASS: security-error/startup cleanup and memory-only emergency wipe of fixed owned buffers");
  puts("PASS: elapsed deadline inside blocking generation/restore/AEZEED/import/export discards returned secrets before the next screen");
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
  fixture(ui);
  lv_obj_update_layout(ui.root_);
  unsigned splashImages=0, splashTitles=0, splashVersions=0;
  auto *startButton=actionButton(ui,START);
  assert(startButton && lv_obj_get_width(startButton)==320 && lv_obj_get_height(startButton)==64);
  assert(lv_obj_get_x(startButton)==80 && lv_obj_get_y(startButton)==672);
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_image_class)) {
      assert(lv_obj_get_x(child)==0 && lv_obj_get_y(child)==280);
      assert(lv_obj_get_width(child)==320 && lv_obj_get_height(child)==240);
      assert(lv_image_get_scale(child)==384); // 320 x 240 rendered as 480 x 360.
      ++splashImages;
    } else if(lv_obj_check_type(child,&lv_label_class)) {
      const char *text=lv_label_get_text(child);
      assert(!strstr(text,"Waveshare") && !strstr(text,"ESP32-P4"));
      if(!strcmp(text,"A U R O R A")) {
        assert(lv_obj_get_y(child)==130);
        assert(lv_obj_get_style_text_font(child,LV_PART_MAIN)==&aurora_font_72); ++splashTitles;
      } else if(!strcmp(text,"SEED GENERATOR")) {
        assert(lv_obj_get_y(child)==204);
        assert(lv_obj_get_style_text_font(child,LV_PART_MAIN)==&aurora_font_24); ++splashTitles;
      } else if(!strcmp(text,"v2.0.0")) {
        ++splashVersions;
      }
    }
  }
  assert(splashImages==1 && splashTitles==2 && splashVersions==1);
  snapshot(ui, "splash.ppm");
  ui.show(Screen::Mode); snapshot(ui,"mode-portrait.ppm");
  unsigned modeButtons=0, modeLogos=0;
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_button_class)) {
      assert(lv_obj_get_width(child)==384 && lv_obj_get_height(child)==64);
      assert(lv_obj_get_x(child)==48 && lv_obj_get_y(child)==316+88*modeButtons);
      ++modeButtons;
    } else if(lv_obj_check_type(child,&lv_image_class)) {
      assert(lv_obj_get_x(child)==184 && lv_obj_get_y(child)==136);
      assert(lv_obj_get_width(child)==112 && lv_obj_get_height(child)==160);
      ++modeLogos;
    }
  }
  assert(modeButtons==4 && modeLogos==1);
  // A fifth 64-pixel row would still end at y=732, leaving a bottom margin.
  assert(316+4*88+64<=732);
  assertHeaderSeparation(ui);
  fixture(ui); ui.mnemonicPage_=0; ui.show(Screen::Mnemonic);
  lv_obj_update_layout(ui.root_);
  assert(!actionButton(ui,MNEMONIC_PREVIOUS));
  auto *next=actionButton(ui,MNEMONIC_NEXT);
  assert(next && lv_obj_get_y(next)==720 && lv_obj_get_height(next)==64);
  unsigned mnemonicEntries=0;
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_label_class) &&
       !strcmp(lv_label_get_text(child),"Écrivez ces mots dans l'ordre. Ne les photographiez jamais.")) {
      assert(lv_obj_get_style_text_font(child,LV_PART_MAIN)==&aurora_font_18);
      assert(lv_obj_get_x(child)==15 && lv_obj_get_y(child)==134 &&
             lv_obj_get_width(child)==450 && lv_obj_get_height(child)==48);
    }
    if(lv_obj_get_child_cnt(child)==24) {
      assert(lv_obj_get_x(child)==15 && lv_obj_get_y(child)==195 &&
             lv_obj_get_width(child)==450 && lv_obj_get_height(child)==495);
      assert(lv_obj_get_style_text_font(lv_obj_get_child(child,0),LV_PART_MAIN)==&aurora_font_18);
      assert(lv_obj_get_style_text_font(lv_obj_get_child(child,1),LV_PART_MAIN)==&aurora_font_30);
      mnemonicEntries=lv_obj_get_child_cnt(child)/2;
    }
  }
  assert(mnemonicEntries==12);
  ui.mnemonicPage_=1; ui.show(Screen::Mnemonic); lv_obj_update_layout(ui.root_);
  auto *previous=actionButton(ui,MNEMONIC_PREVIOUS);
  assert(previous && lv_obj_get_y(previous)==720 && lv_obj_get_height(previous)==64 && !actionButton(ui,MNEMONIC_NEXT));
  assert(actionButton(ui,NEXT_VERIFY) && lv_obj_get_y(actionButton(ui,NEXT_VERIFY))==720);
  fixture(ui); ui.protectedSession_=false; ui.manualRestore_=false; ui.loadedWallet_=false;
  ui.show(Screen::Info); lv_obj_update_layout(ui.root_);
  const char expectedInfo[]=
      "Adresse\n"
      "bc1qfixtureonlyneverusethisaddress0000000000000000\n\n"
      "Chemin : m/84'/0'/0'/0/0\n\n"
      "Clé publique étendue du compte\n"
      "zpub-fixture-only-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789";
  unsigned infoPanels=0,infoButtons=0;
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    const uintptr_t action=reinterpret_cast<uintptr_t>(lv_obj_get_user_data(child));
    if(action==TO_QR_ADDRESS || action==REVEAL_PRIVATE || action==TO_BACKUP) {
      assert(lv_obj_get_y(child)==720 && lv_obj_get_height(child)==64);
      ++infoButtons;
    }
    if(lv_obj_get_child_cnt(child)==1) {
      auto *content=lv_obj_get_child(child,0);
      if(lv_obj_check_type(content,&lv_label_class) && !strcmp(lv_label_get_text(content),expectedInfo)) {
        assert(lv_obj_get_x(child)==15 && lv_obj_get_y(child)==140 &&
               lv_obj_get_width(child)==450 && lv_obj_get_height(child)==550);
        assert(lv_obj_get_style_text_font(content,LV_PART_MAIN)==&aurora_font_18);
        ++infoPanels;
      }
    }
  }
  assert(infoPanels==1 && infoButtons==3);
  sdReady=true; fixture(ui); ui.manualRestore_=false; ui.show(Screen::Backup); lv_obj_update_layout(ui.root_);
  unsigned backupSubtitles=0;
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_label_class) &&
       !strcmp(lv_label_get_text(child),"Choisissez un format (carte FAT32).")) {
      assert(lv_obj_get_style_text_font(child,LV_PART_MAIN)==&aurora_font_18);
      ++backupSubtitles;
    }
  }
  assert(backupSubtitles==1);
  ui.show(Screen::Mode);
  sdReady=false;
  const Action modeActions[]={NEW_WALLET,OPEN_WALLET,RESTORE_WALLET,RECOVER_UMBREL};
  const Screen modeDestinations[]={Screen::Setup,Screen::ImportName,Screen::RestoreSetup,Screen::UmbrelWarning};
  for(unsigned i=0;i<4;++i) {
    ui.show(Screen::Mode); const unsigned before=sdChecks; click(ui,modeActions[i]);
    assert(ui.screen_==modeDestinations[i] && sdChecks==before);
  }
  ui.show(Screen::ImportName); lv_obj_update_layout(ui.root_);
  assert(ui.importFileDropdown_ && lv_obj_get_x(ui.importFileDropdown_)==48 &&
         lv_obj_get_y(ui.importFileDropdown_)==196 &&
         lv_obj_get_width(ui.importFileDropdown_)==384 && lv_obj_get_height(ui.importFileDropdown_)==64);
  ui.show(Screen::RestoreSetup); lv_obj_update_layout(ui.root_);
  unsigned restoreCounts=0,restoreSubtitles=0,restoreInfo=0;
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_label_class) &&
       !strcmp(lv_label_get_text(child),"Choisissez le nombre de mots de la phrase BIP39.")) {
      assert(lv_obj_get_style_text_font(child,LV_PART_MAIN)==&aurora_font_18);
      ++restoreSubtitles;
    }
    if(lv_obj_check_type(child,&lv_label_class) &&
       !strcmp(lv_label_get_text(child),
               "Les mots sont vérifiés avec la liste anglaise officielle.\n"
               "Le checksum BIP39 sera contrôlé avant toute dérivation.")) {
      assert(lv_obj_get_style_text_font(child,LV_PART_MAIN)==&aurora_font_14);
      ++restoreInfo;
    }
    const uintptr_t action=reinterpret_cast<uintptr_t>(lv_obj_get_user_data(child));
    if(action>=WORD_12 && action<=WORD_24) {
      const unsigned index=static_cast<unsigned>(action-WORD_12);
      const int expectedX=index<3 ? 72+index*120 : 132+(index-3)*120;
      const int expectedY=index<3 ? 210 : 330;
      assert(lv_obj_get_x(child)==expectedX && lv_obj_get_y(child)==expectedY);
      assert(lv_obj_get_width(child)==96 && lv_obj_get_height(child)==96);
      assert(lv_obj_get_style_text_font(lv_obj_get_child(child,0),LV_PART_MAIN)==&aurora_font_30);
      ++restoreCounts;
    }
  }
  assert(restoreCounts==5 && restoreSubtitles==1 && restoreInfo==1);
  ui.show(Screen::Setup); lv_obj_update_layout(ui.root_);
  unsigned setupCounts=0,setupTypes=0;
  unsigned setupSubtitles=0;
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_label_class) &&
       (!strcmp(lv_label_get_text(child),"Nombre de mots") ||
        !strcmp(lv_label_get_text(child),"Type d'adresse"))) {
      assert(lv_obj_get_style_text_font(child,LV_PART_MAIN)==&aurora_font_18);
      ++setupSubtitles;
    }
    const uintptr_t action=reinterpret_cast<uintptr_t>(lv_obj_get_user_data(child));
    if(action>=WORD_12 && action<=WORD_24) {
      assert(lv_obj_get_style_text_font(lv_obj_get_child(child,0),LV_PART_MAIN)==&aurora_font_30);
      ++setupCounts;
    } else if(action>=TYPE_LEGACY && action<=TYPE_TAPROOT) {
      assert(lv_obj_get_style_text_font(lv_obj_get_child(child,0),LV_PART_MAIN)==&aurora_font_20);
      ++setupTypes;
    }
  }
  assert(setupCounts==5 && setupTypes==4 && setupSubtitles==2);
  ui.show(Screen::Mode); click(ui,RECOVER_UMBREL); click(ui,UMBREL_CONTINUE);
  assert(ui.umbrelRecovery_ && ui.words_==24 && ui.screen_==Screen::RestoreWords);
  strlcpy(ui.restoreMnemonic_,"24-word aezeed test fixture",sizeof(ui.restoreMnemonic_));
  ui.show(Screen::UmbrelPassphrase);
  assert(lv_color_eq(lv_obj_get_style_bg_color(ui.keyboard_,LV_PART_MAIN),lv_color_hex(0x0A0C10)));
  assert(lv_color_eq(lv_obj_get_style_bg_color(ui.keyboard_,LV_PART_ITEMS),lv_color_hex(0x1A1D23)));
  assert(lv_color_eq(lv_obj_get_style_bg_color(ui.passArea_,LV_PART_MAIN),lv_color_hex(0xFFFFFF)));
  assert(lv_obj_get_style_text_font(ui.passArea_,LV_PART_MAIN)==&aurora_font_24);
  lv_obj_update_layout(ui.passArea_); assert(lv_obj_get_height(ui.passArea_)>=60);
  lv_obj_send_event(ui.keyboard_,LV_EVENT_READY,nullptr);
  mock.time+=101000; ui.tick(); assert(ui.screen_==Screen::UmbrelResult && ui.umbrelRootXprv_[0]);
  click(ui,UMBREL_SHOW_XPRV); assert(ui.screen_==Screen::UmbrelQr);
  strlcpy(ui.umbrelRootXprv_,
      "xprv9s21ZrQH143K3-fixture-only-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-012345",
      sizeof(ui.umbrelRootXprv_));
  ui.show(Screen::UmbrelQr);
  lv_obj_update_layout(ui.root_);
  unsigned umbrelQrCount=0,umbrelValueCount=0;
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_qrcode_class)) {
      assert(lv_obj_get_x(child)==72 && lv_obj_get_y(child)==130 &&
             lv_obj_get_width(child)==336 && lv_obj_get_height(child)==336);
      ++umbrelQrCount;
    } else if(lv_obj_check_type(child,&lv_label_class) &&
              !strcmp(lv_label_get_text(child),ui.umbrelRootXprv_)) {
      assert(lv_obj_get_y(child)==480 && lv_obj_get_width(child)==432);
      assert(lv_obj_get_style_text_font(child,LV_PART_MAIN)==&aurora_font_30);
      ++umbrelValueCount;
    }
  }
  assert(umbrelQrCount==1 && umbrelValueCount==1);
  snapshot(ui,"umbrel-qr.ppm");
  mock.time+=15001000; ui.tick();
  assert(ui.screen_==Screen::Mode && !ui.umbrelRootXprv_[0]);
  for(auto screen:{Screen::Setup,Screen::RestoreSetup,Screen::ImportName,
                  Screen::Passphrase,Screen::RestoreWords,Screen::RestorePassphrase,
                  Screen::RestoreSetup}) {
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
  fixture(ui); ui.qrContent_=AuroraUI::QrContent::AccountXpub; ui.show(Screen::Qr);
  lv_obj_update_layout(ui.root_);
  unsigned walletQrCount=0,walletQrValueCount=0;
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_qrcode_class)) {
      assert(lv_obj_get_x(child)==72 && lv_obj_get_y(child)==130 &&
             lv_obj_get_width(child)==336 && lv_obj_get_height(child)==336);
      ++walletQrCount;
    } else if(lv_obj_check_type(child,&lv_label_class) &&
              !strcmp(lv_label_get_text(child),ui.wallet_.accountXpub)) {
      assert(lv_obj_get_y(child)==480 && lv_obj_get_width(child)==432);
      assert(lv_obj_get_style_text_font(child,LV_PART_MAIN)==&aurora_font_30);
      ++walletQrValueCount;
    }
  }
  assert(walletQrCount==1 && walletQrValueCount==1);
  snapshot(ui,"qr-xpub.ppm");
  ui.qrContent_=AuroraUI::QrContent::PrivateKey; ui.show(Screen::Qr);
  snapshot(ui,"qr-private.ppm");
  for (auto screen : {Screen::Mode, Screen::Setup, Screen::Passphrase, Screen::Mnemonic, Screen::Info,
      Screen::Qr, Screen::Verify, Screen::Backup, Screen::ImportName, Screen::ImportPassword,
      Screen::RestoreSetup, Screen::RestoreWords, Screen::RestorePassphrase,
      Screen::UmbrelWarning, Screen::UmbrelPassphrase, Screen::UmbrelProcessing, Screen::UmbrelResult,
      Screen::UmbrelQr, Screen::Generating, Screen::FileProcessing, Screen::GenerationError,
      Screen::SecurityError, Screen::ExportWarning, Screen::ExportName, Screen::ExportPassword,
      Screen::Wipe, Screen::SdRequired}) {
    ui.show(screen); fixture(ui); lv_obj_update_layout(ui.root_);
    assert(lv_obj_get_width(ui.root_) == 480 && lv_obj_get_height(ui.root_) == 800);
    assertHeaderSeparation(ui);
    const bool hasHeader=screen!=Screen::Splash && screen!=Screen::Restoring &&
        screen!=Screen::UmbrelProcessing && screen!=Screen::Generating &&
        screen!=Screen::FileProcessing;
    unsigned headerLabels=0;
    for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
      auto *child=lv_obj_get_child(ui.root_,i);
      if(lv_obj_check_type(child,&lv_label_class) && lv_obj_get_y(child)==30) {
        assert(lv_obj_get_style_text_font(child,LV_PART_MAIN)==&aurora_font_20);
        ++headerLabels;
      }
    }
    assert(!hasHeader || headerLabels>=1);
    char name[40]; snprintf(name, sizeof(name), "screen-%02u.ppm", static_cast<unsigned>(screen)); snapshot(ui, name);
  }
  ui.show(Screen::Entropy); assertHeaderSeparation(ui);
  unsigned entropySubtitles=0;
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_label_class) &&
       !strcmp(lv_label_get_text(child),"Aperçu cryptographique défilant")) {
      assert(lv_obj_get_style_text_font(child,LV_PART_MAIN)==&aurora_font_18);
      ++entropySubtitles;
    }
  }
  assert(entropySubtitles==1);
  ui.onTouchSample(23, 200, 0); ui.onTouchSample(300, 341, 0);
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
  testPasswordFileSessions(ui);
  sdReady=true;
  testTransientWordCopies(ui);
  testSensitiveLifecycle(ui);
  assert(wipedAllocations>1000);
  puts("PASS: LVGL free/realloc allocations wiped before release, including earlier edited text copies");
  puts("PASS: native 480x800 screens, portrait collection, threshold, sensor stop interlock, cancel/restart");
  return 0;
}
