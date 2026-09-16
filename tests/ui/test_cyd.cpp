#include <array>
#include <cassert>
#include <cstdio>
#include "Arduino.h"
#include "Hash.h"
#include "lvgl.h"
#include "secure_lvgl_memory.h"
extern "C" __declspec(noreturn) void auroraCydUiFailure(void) { abort(); }
#define private public
#include "ui.h"
#undef private

extern "C" uint32_t native_millis(void) { return millis(); }
static unsigned wipes=0;
static bool sdReady=true;
static unsigned sdChecks=0;
bool auroraSdReady() { ++sdChecks; return sdReady; }
extern "C" void auroraUiWipeAudit(const void *pointer,size_t size) {
  for(size_t i=0;i<size;++i) assert(static_cast<const uint8_t *>(pointer)[i]==0);
  ++wipes;
}
WalletEngine::WalletEngine() {}
WalletSelfTest WalletEngine::selfTest() { return WalletSelfTest::Ok; }
bool WalletEngine::create(uint8_t,AddressKind,const char *,const uint8_t *,WalletOutput &out) { strlcpy(out.mnemonic,"PUBLIC-FIXTURE",sizeof(out.mnemonic)); return true; }
bool WalletEngine::restore(const char *,uint8_t,AddressKind,const char *,WalletOutput &) { return true; }
bool WalletEngine::accountXprv(const WalletOutput &,const char *,char *out,size_t) { out[0]=0; return true; }
bool WalletEngine::rootXprvFromSeed(const uint8_t *,size_t,char *,size_t) { return false; }
AezeedResult AezeedEngine::decode(const char *,const char *,AezeedDecoded &) { return AezeedResult::MemoryFailed; }
void AezeedEngine::wipe(AezeedDecoded &out) { secureZero(&out,sizeof(out)); }
void WalletEngine::wipe(WalletOutput &wallet) { secureZero(&wallet,sizeof(wallet)); }
bool WalletEngine::bip39Word(const char *) { return true; }
uint8_t WalletEngine::bip39Suggestions(const char *,char *,size_t,uint8_t) { return 0; }
const char *walletExportSuffix(WalletExportFormat) { return ".aurora"; }
bool auroraWalletCryptoSelfTest() { return true; }
void wipeAuroraWalletData(AuroraWalletData &data) { secureZero(&data,sizeof(data)); }
WalletExportResult writeWalletExportFile(WalletExportFormat,const char *,const char *,const WalletExportData &,char *,size_t) { return WalletExportResult::NoCard; }
AuroraWalletReadResult readAuroraWalletFile(const char *,const char *,AuroraWalletData &) { return AuroraWalletReadResult::NoCard; }
AuroraWalletListResult listAuroraWalletFiles(char *out,size_t size,uint16_t &count) { if(size)out[0]=0; count=0; return AuroraWalletListResult::NoCard; }
static unsigned reads=0;
static bool changedFile=false;
static uint32_t cryptoTime=0;
AuroraWalletReadResult readAuroraWalletFileChecked(const char *,const char *password,
    AuroraWalletData &out,uint8_t *fingerprint,const uint8_t *expected) {
  ++reads; mock.time+=cryptoTime;
  if(!sdReady || strcmp(password,"PUBLIC-TEST-PASSWORD") || (expected && changedFile))
    return AuroraWalletReadResult::AuthenticationFailed;
  secureZero(&out,sizeof(out)); out.wordCount=12; out.addressKind=2; out.fileVersion=2;
  strlcpy(out.addressType,"native-segwit-p2wpkh",sizeof(out.addressType));
  strlcpy(out.passphrase,"PUBLIC-TEST-PASSPHRASE",sizeof(out.passphrase));
  if(fingerprint) memset(fingerprint,0x42,32);
  return AuroraWalletReadResult::Ok;
}
WalletExportResult writeAuroraWalletFileVerified(const char *,const char *,const WalletExportData &data,
    char *,size_t,uint8_t *) { assert(!data.pin); return WalletExportResult::NoCard; }
#include "../../src/ui.cpp"
#include "../../src/pin_security.cpp"

static std::array<lv_color_t,320*240> frame;
static void expectKeyboardColors175(lv_obj_t *keyboard) {
  // 1.7.5 used lv_keyboard_create without palette overrides; the pinned
  // LVGL 8.4 theme/configuration is unchanged. No textarea or secret here.
  lv_obj_t *reference=lv_keyboard_create(lv_obj_get_parent(keyboard));
  lv_obj_add_flag(reference,LV_OBJ_FLAG_HIDDEN);
  for(lv_state_t state:{LV_STATE_DEFAULT,LV_STATE_PRESSED,LV_STATE_CHECKED}) {
    lv_obj_clear_state(keyboard,LV_STATE_ANY);
    lv_obj_clear_state(reference,LV_STATE_ANY);
    lv_obj_add_state(keyboard,state);
    lv_obj_add_state(reference,state);
    for(auto part:{LV_PART_MAIN,LV_PART_ITEMS}) {
      assert(lv_obj_get_style_bg_color(keyboard,part).full==lv_obj_get_style_bg_color(reference,part).full);
      assert(lv_obj_get_style_text_color(keyboard,part).full==lv_obj_get_style_text_color(reference,part).full);
      assert(lv_obj_get_style_border_color(keyboard,part).full==lv_obj_get_style_border_color(reference,part).full);
    }
  }
  lv_obj_clear_state(keyboard,LV_STATE_ANY);
  lv_obj_del(reference);
}
static void flush(lv_disp_drv_t *display,const lv_area_t *area,lv_color_t *data) {
  for(int y=area->y1;y<=area->y2;++y) for(int x=area->x1;x<=area->x2;++x) frame[y*320+x]=*data++;
  lv_disp_flush_ready(display);
}
static void snapshot(AuroraUI &ui,const char *name) {
  if(ui.keyboard_) expectKeyboardColors175(ui.keyboard_);
  lv_obj_update_layout(ui.root_); lv_refr_now(nullptr);
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    lv_obj_t *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_btn_class) || lv_obj_check_type(child,&lv_textarea_class)) {
      lv_area_t area; lv_obj_get_coords(child,&area);
      if (!(area.x1>=0 && area.x2<320 && area.y1>=0 && area.y2<240)) {
        fprintf(stderr,"%s child %u: %d,%d..%d,%d\n",name,i,area.x1,area.y1,area.x2,area.y2);
      }
      assert(area.x1>=0 && area.x2<320 && area.y1>=0 && area.y2<240);
    }
  }
  FILE *file=fopen(name,"wb"); assert(file); fprintf(file,"P6\n320 240\n255\n");
  for(auto color:frame) {
    uint16_t pixel=color.full;
    const uint8_t rgb[]={static_cast<uint8_t>(((pixel>>11)&31)*255/31),
      static_cast<uint8_t>(((pixel>>5)&63)*255/63),static_cast<uint8_t>((pixel&31)*255/31)};
    fwrite(rgb,1,3,file);
  }
  fclose(file);
}
static void click(AuroraUI &ui,Action action) {
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_btn_class) &&
       reinterpret_cast<uintptr_t>(lv_obj_get_user_data(child))==action) {
      lv_event_send(child,LV_EVENT_CLICKED,nullptr); return;
    }
  }
  assert(false && "Expected button missing");
}
static lv_obj_t *findLabel(lv_obj_t *root,const char *text) {
  if(lv_obj_check_type(root,&lv_label_class) && !strcmp(lv_label_get_text(root),text)) return root;
  for(uint32_t i=0;i<lv_obj_get_child_cnt(root);++i)
    if(auto *found=findLabel(lv_obj_get_child(root,i),text)) return found;
  return nullptr;
}
static void expectFont(AuroraUI &ui,const char *text,const lv_font_t *font) {
  auto *object=findLabel(ui.root_,text);
  if(!object) fprintf(stderr,"Missing typography label: %s\n",text);
  assert(object && lv_obj_get_style_text_font(object,LV_PART_MAIN)==font);
}
static void typography175(AuroraUI &ui) {
  // Font sizes from src/ui.cpp at the official 1.7.5 commit ecb63fe.
  // Only presentation is matched: retain all current session/security logic.
  using Screen=AuroraUI::Screen;
  expectFont(ui,"A U R O R A",&aurora_font_20);
  expectFont(ui,"SEED GENERATOR",&aurora_font_10);
  ui.show(Screen::Setup);
  expectFont(ui,"Configuration du portefeuille",&aurora_font_14);
  expectFont(ui,"Nombre de mots",&aurora_font_10);
  expectFont(ui,"Type d'adresse",&aurora_font_10);
  for(const char *count:{"12","15","18","21","24"}) expectFont(ui,count,&aurora_font_10);
  for(const char *kind:{"Legacy\nm/44'/0'/0'/0/0","Nested SegWit\nm/49'/0'/0'/0/0",
      "Native SegWit\nm/84'/0'/0'/0/0","Taproot\nm/86'/0'/0'/0/0"}) expectFont(ui,kind,&aurora_font_10);
  expectFont(ui,"CONTINUER",&aurora_font_12); snapshot(ui,"setup-1.9.5.ppm");
  ui.show(Screen::ImportName);
  assert(lv_obj_get_style_text_font(ui.importFileDropdown_,0)==&aurora_font_12);
  ui.show(Screen::ImportPassword);
  expectFont(ui,"Mot de passe Aurora Wallet",&aurora_font_12);
  assert(lv_obj_get_style_text_font(ui.filePasswordArea_,0)==&aurora_font_12);
  snapshot(ui,"password-1.9.5.ppm");
  ui.show(Screen::RestoreSetup);
  expectFont(ui,"Choisissez le nombre de mots de la phrase BIP39.",&aurora_font_10);
  for(const char *count:{"12","15","18","21","24"}) expectFont(ui,count,&aurora_font_12);
  snapshot(ui,"restore-setup-1.9.5.ppm");
  ui.show(Screen::RestoreWords);
  assert(lv_obj_get_style_text_font(ui.restoreWordArea_,0)==&aurora_font_14);
  for(auto screen:{Screen::Passphrase,Screen::RestorePassphrase}) {
    ui.show(screen);
    assert(ui.passArea_ && ui.passConfirmArea_); // Keep the 1.9.3 confirmation.
    assert(lv_obj_get_style_text_font(ui.passArea_,0)==&aurora_font_12);
    assert(lv_obj_get_style_text_font(ui.passConfirmArea_,0)==&aurora_font_12);
    assert(lv_obj_get_style_text_font(ui.keyboard_,LV_PART_ITEMS)==&lv_font_montserrat_14);
    expectKeyboardColors175(ui.keyboard_);
  }
  snapshot(ui,"passphrase-1.9.5.ppm");
  ui.show(Screen::Entropy);
  expectFont(ui,"Bougez votre doigt dans le cadre",&aurora_font_14);
  assert(lv_obj_get_style_text_font(ui.entropyStatus_,0)==&aurora_font_10);
  assert(TouchEntropy::REQUIRED_SAMPLES==320); // No return to the old entropy workflow.
  snapshot(ui,"entropy-1.9.5.ppm");
  ui.show(Screen::Mode);
  ui.words_=12;
  strlcpy(ui.wallet_.mnemonic,"abandon ability able about above absent absorb abstract absurd abuse access accident",sizeof(ui.wallet_.mnemonic));
  ui.show(Screen::Mnemonic);
  expectFont(ui,"abandon",&aurora_font_16); expectFont(ui,"01",&aurora_font_12);
  snapshot(ui,"words-1.9.5.ppm");
  ui.show(Screen::Info);
  expectFont(ui,"CODES QR",&aurora_font_12);
  snapshot(ui,"info-initial-1.9.5.ppm"); // All actions must fit the CYD, not y=720.
  ui.show(Screen::Backup);
  expectFont(ui,"Choisissez un format (carte FAT32).",&aurora_font_10);
  ui.show(Screen::ExportName);
  assert(lv_obj_get_style_text_font(ui.exportNameArea_,0)==&aurora_font_12);
  ui.show(Screen::ExportPassword);
  assert(lv_obj_get_style_text_font(ui.filePasswordArea_,0)==&aurora_font_10);
  assert(lv_obj_get_style_text_font(ui.filePasswordConfirmArea_,0)==&aurora_font_10);
  ui.closeSession(); sdChecks=0;
  puts("PASS: CYD 1.9.5 matches 1.7.5 typography and keyboard colors, retaining current fields/actions/entropy");
}
int main() {
  lv_init(); static lv_color_t buffer[320*40]; static lv_disp_draw_buf_t draw;
  lv_disp_draw_buf_init(&draw,buffer,nullptr,320*40);
  static lv_disp_drv_t display; lv_disp_drv_init(&display);
  display.hor_res=320; display.ver_res=240; display.flush_cb=flush; display.draw_buf=&draw;
  lv_disp_drv_register(&display);
  static AuroraUI ui; ui.begin(); ui.selfTestPending_=false;
  typography175(ui);
  using Screen=AuroraUI::Screen;
  ui.show(Screen::Mode); snapshot(ui,"mode-cyd.ppm");
  unsigned modeButtons=0, modeLogos=0;
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    auto *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_btn_class)) {
      assert(lv_obj_get_width(child)==178 && lv_obj_get_height(child)==44);
      assert(lv_obj_get_x(child)==12 && lv_obj_get_y(child)==48+60*modeButtons);
      ++modeButtons;
    } else if(lv_obj_check_type(child,&lv_img_class)) {
      assert(lv_obj_get_x(child)==200 && lv_obj_get_y(child)==43); ++modeLogos;
    }
  }
  assert(modeButtons==3 && modeLogos==1); // Original CYD arrangement untouched.
  sdReady=false;
  const Action actions[]={NEW_WALLET,OPEN_WALLET,RESTORE_WALLET};
  const Screen destinations[]={Screen::Setup,Screen::ImportName,Screen::RestoreSetup};
  for(unsigned i=0;i<3;++i) {
    ui.show(Screen::Mode); click(ui,actions[i]); assert(ui.screen_==destinations[i]);
  }
  for(auto screen:{Screen::Setup,Screen::Passphrase,Screen::RestoreWords,Screen::RestorePassphrase}) {
    ui.show(screen); assert(ui.screen_==screen && sdChecks==0);
  }
  for(auto screen:{Screen::Backup,Screen::ExportName,Screen::ExportPassword}) {
    ui.show(screen);
    assert(ui.screen_==Screen::SdRequired && ui.afterSd_==screen);
    assert(!ui.keyboard_ && !ui.passArea_ && !ui.restoreWordArea_);
  }
  snapshot(ui,"sd-required.ppm"); click(ui,RETRY_SD); assert(ui.screen_==Screen::SdRequired);
  sdReady=true; click(ui,RETRY_SD); assert(ui.screen_==Screen::ExportPassword);
  ui.show(Screen::Mode);
  sdReady=false; const unsigned offlineChecks=sdChecks;
  ui.show(Screen::Passphrase); snapshot(ui,"passphrase.ppm");
  assert(ui.passArea_ && ui.passConfirmArea_);
  lv_textarea_set_text(ui.passArea_,"same"); lv_textarea_set_text(ui.passConfirmArea_,"same");
  assert(ui.confirmPassphrase() && !strcmp(ui.passphrase_,"same"));
  ui.show(Screen::Mode); sdReady=true;
  for(auto screen:{Screen::RestoreWords,Screen::ImportPassword,Screen::Passphrase}) {
    ui.show(screen);
    if(ui.restoreWordArea_) lv_textarea_set_text(ui.restoreWordArea_,"abandon");
    if(ui.filePasswordArea_) lv_textarea_set_text(ui.filePasswordArea_,"PUBLIC-TEST-PASSWORD");
    mock.time+=121000000; ui.tick();
    assert(ui.screen_==Screen::Mode && !ui.sensitiveStateActive_ && !ui.filePassword_[0]);
    assert(!ui.wallet_.mnemonic[0] && !ui.restoreWords_[0][0]);
  }
  ui.show(Screen::Setup); ui.entropyCollected_=true; assert(ui.generate());
  ui.show(Screen::Mnemonic);
  assert(!ui.secretCountdown_ && ui.visibleSecret_==AuroraUI::Access::None);
  mock.time+=121000000; ui.tick();
  assert(ui.screen_==Screen::Mode && !ui.wallet_.mnemonic[0]);
  for(bool restored:{false,true}) {
    ui.show(Screen::Setup); ui.entropyCollected_=true; assert(ui.generate());
    ui.manualRestore_=restored; ui.loadedWallet_=restored;
    ui.qrContent_=AuroraUI::QrContent::PrivateKey; ui.show(Screen::Qr);
    assert(!ui.secretCountdown_ && ui.visibleSecret_==AuroraUI::Access::None);
    mock.time+=61000000; ui.tick(); assert(ui.screen_==Screen::Qr);
    click(ui,TO_INFO); assert(ui.screen_==Screen::Info && !ui.fileSession_);
    ui.closeSession();
  }
  ui.show(Screen::ImportPassword);
  lv_textarea_set_text(ui.filePasswordArea_,"PUBLIC-TEST-PASSWORD");
  lv_event_send(ui.keyboard_,LV_EVENT_READY,nullptr);
  mock.time+=200000; ui.tick();
  assert(ui.screen_==Screen::Info && ui.fileSession_ && ui.protectedSession_);
  assert(!ui.wallet_.mnemonic[0] && !ui.wallet_.privateWif[0] && !ui.passphrase_[0] && !ui.filePassword_[0]);
  assert(ui.hasPassphrase()); snapshot(ui,"info.ppm");
  const unsigned firstRead=reads;
  for(unsigned round=0;round<3;++round) {
    ui.show(Screen::Mnemonic); assert(ui.screen_==Screen::PrivatePassword);
    snapshot(ui,"private-password.ppm");
    lv_textarea_set_text(ui.filePasswordArea_,"PUBLIC-TEST-PASSWORD");
    lv_event_send(ui.keyboard_,LV_EVENT_READY,nullptr);
    mock.time+=200000; ui.tick();
    assert(ui.screen_==Screen::Mnemonic && ui.privateLoaded_ && !ui.filePassword_[0]);
    if(round==0) {
      ui.words_=24;
      strlcpy(ui.wallet_.mnemonic,"abandon ability able about above absent absorb abstract absurd abuse access accident account accuse achieve acid acoustic acquire across act action actor actress actual",sizeof(ui.wallet_.mnemonic));
      ui.show(Screen::Mnemonic);
      const auto start=ui.visibleSecretStartedMs_;
      mock.time+=61000000; ui.tick();
      assert(!strcmp(lv_label_get_text(ui.secretCountdown_),"01:59"));
      click(ui,MNEMONIC_NEXT);
      assert(ui.mnemonicPage_==1 && ui.visibleSecretStartedMs_==start);
      assert(!strcmp(lv_label_get_text(ui.secretCountdown_),"01:59"));
      snapshot(ui,"words-countdown-page-2.ppm");
      click(ui,MNEMONIC_PREVIOUS);
      assert(ui.mnemonicPage_==0 && ui.visibleSecretStartedMs_==start);
    }
    // Public marker deliberately injected after successful mock derivation.
    strlcpy(ui.wallet_.mnemonic,"PUBLIC-TEST-WORDS",sizeof(ui.wallet_.mnemonic));
    strlcpy(ui.wallet_.privateWif,"PUBLIC-TEST-WIF",sizeof(ui.wallet_.privateWif));
    if(round==0) ui.show(Screen::Info);
    else if(round==1) { mock.time+=180000000; ui.tick(); }
    else { click(ui,LOCK_SESSION); }
    assert(!ui.wallet_.mnemonic[0] && !ui.wallet_.privateWif[0] && !ui.privateLoaded_ && !ui.passphrase_[0]);
    if(round!=0) {
      assert(ui.screen_==Screen::Mode && !ui.fileSession_);
      if(round!=2) {
        strlcpy(ui.filePassword_,"PUBLIC-TEST-PASSWORD",sizeof(ui.filePassword_));
        assert(ui.performWalletImport()); ui.show(Screen::Info);
      }
    }
  }
  assert(reads==firstRead+4);
  for(unsigned mode=0;mode<3;++mode) {
    ui.show(Screen::Mode); ui.show(Screen::ImportPassword);
    strlcpy(ui.filePassword_,"PUBLIC-TEST-PASSWORD",sizeof(ui.filePassword_));
    assert(ui.performWalletImport()); ui.show(Screen::Info); ui.show(Screen::Mnemonic);
    changedFile=mode==0; cryptoTime=mode==1?16000000:(mode==2?121000000:0);
    lv_textarea_set_text(ui.filePasswordArea_,"PUBLIC-TEST-PASSWORD");
    lv_event_send(ui.keyboard_,LV_EVENT_READY,nullptr); mock.time+=200000; ui.tick();
    if(mode==1) {
      assert(ui.screen_==Screen::Mnemonic && ui.privateLoaded_);
      assert(!strcmp(lv_label_get_text(ui.secretCountdown_),"03:00"));
      click(ui,LOCK_SESSION);
    } else assert(ui.screen_!=Screen::Mnemonic && !ui.wallet_.mnemonic[0]);
    assert(!ui.filePassword_[0]);
    changedFile=false; cryptoTime=0;
  }
  for(bool returnEarly:{false,true}) {
    ui.closeSession();
    strlcpy(ui.filePassword_,"PUBLIC-TEST-PASSWORD",sizeof(ui.filePassword_));
    assert(ui.performWalletImport()); ui.show(Screen::Info);
    click(ui,REVEAL_PRIVATE);
    cryptoTime=65000000;
    lv_textarea_set_text(ui.filePasswordArea_,"PUBLIC-TEST-PASSWORD");
    lv_event_send(ui.keyboard_,LV_EVENT_READY,nullptr); mock.time+=200000; ui.tick();
    cryptoTime=0;
    assert(ui.screen_==Screen::Qr && ui.privateLoaded_);
    assert(!strcmp(lv_label_get_text(ui.secretCountdown_),"01:00"));
    if(returnEarly) click(ui,LOCK_SESSION);
    else {
      mock.time+=59999000; ui.tick();
      assert(ui.screen_==Screen::Qr);
      assert(!strcmp(lv_label_get_text(ui.secretCountdown_),"00:01"));
      mock.time+=1000; ui.tick();
    }
    assert(ui.screen_==Screen::Mode && !ui.fileSession_ && !ui.privateLoaded_);
    assert(!ui.wallet_.mnemonic[0] && !ui.wallet_.privateWif[0] && !ui.filePassword_[0]);
  }
  ui.show(Screen::Mode); ui.show(Screen::ImportPassword);
  lv_textarea_set_text(ui.filePasswordArea_,"PUBLIC-TEST-PASSWORD");
  sdReady=false; lv_event_send(ui.keyboard_,LV_EVENT_READY,nullptr);
  assert(ui.screen_==Screen::SdRequired && !ui.filePassword_[0]);
  ui.show(Screen::Mode); ui.show(Screen::Entropy); snapshot(ui,"entropy.ppm"); ui.show(Screen::Mode);
  assert(wipes>100);
  // Terminal ownership walk: no LVGL operations are legal after this point.
  void *first=auroraUiAlloc(256), *second=auroraUiAlloc(512);
  assert(first && second); memset(first,0xa5,256); memset(second,0x5a,512);
  strlcpy(ui.filePassword_,"PUBLIC-TEST-PASSWORD",sizeof(ui.filePassword_));
  ui.emergencyWipeSecrets(); assert(!ui.filePassword_[0]);
  assert(auroraUiTryFreezeAllocations()); auroraUiWipeFrozenAllocations();
  for(unsigned i=0;i<256;++i) assert(static_cast<uint8_t *>(first)[i]==0);
  for(unsigned i=0;i<512;++i) assert(static_cast<uint8_t *>(second)[i]==0);
  puts("PASS: CYD LVGL8 password-only sessions, public-only idle state, 120s workflow / 180s words expiry, display time excludes crypto, Return locks, replaced file, SD removal, wiped UI allocations");
}
