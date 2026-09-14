#include <array>
#include <cassert>
#include <cstdio>
#include "Arduino.h"
#include "Hash.h"
#include "lvgl.h"
#define private public
#include "ui.h"
#undef private

extern "C" uint32_t native_millis(void) { return millis(); }
static unsigned wipes=0;
extern "C" void auroraUiWipeAudit(const void *pointer,size_t size) {
  for(size_t i=0;i<size;++i) assert(static_cast<const uint8_t *>(pointer)[i]==0);
  ++wipes;
}
WalletEngine::WalletEngine() {}
WalletSelfTest WalletEngine::selfTest() { return WalletSelfTest::Ok; }
bool WalletEngine::create(uint8_t,AddressKind,const char *,const uint8_t *,WalletOutput &) { return true; }
bool WalletEngine::restore(const char *,uint8_t,AddressKind,const char *,WalletOutput &) { return true; }
bool WalletEngine::accountXprv(const WalletOutput &,const char *,char *,size_t) { return false; }
void WalletEngine::wipe(WalletOutput &wallet) { secureZero(&wallet,sizeof(wallet)); }
bool WalletEngine::bip39Word(const char *) { return true; }
uint8_t WalletEngine::bip39Suggestions(const char *,char *,size_t,uint8_t) { return 0; }
const char *walletExportSuffix(WalletExportFormat) { return ".aurora"; }
bool auroraWalletCryptoSelfTest() { return true; }
void wipeAuroraWalletData(AuroraWalletData &data) { secureZero(&data,sizeof(data)); }
WalletExportResult writeWalletExportFile(WalletExportFormat,const char *,const char *,const WalletExportData &,char *,size_t) { return WalletExportResult::NoCard; }
AuroraWalletReadResult readAuroraWalletFile(const char *,const char *,AuroraWalletData &) { return AuroraWalletReadResult::NoCard; }
AuroraWalletListResult listAuroraWalletFiles(char *out,size_t size,uint16_t &count) { if(size)out[0]=0; count=0; return AuroraWalletListResult::NoCard; }
#include "../../src/ui.cpp"
#include "../../src/pin_security.cpp"

static std::array<lv_color_t,320*240> frame;
static void flush(lv_disp_drv_t *display,const lv_area_t *area,lv_color_t *data) {
  for(int y=area->y1;y<=area->y2;++y) for(int x=area->x1;x<=area->x2;++x) frame[y*320+x]=*data++;
  lv_disp_flush_ready(display);
}
static void snapshot(AuroraUI &ui,const char *name) {
  lv_obj_update_layout(ui.root_); lv_refr_now(nullptr);
  for(uint32_t i=0;i<lv_obj_get_child_cnt(ui.root_);++i) {
    lv_obj_t *child=lv_obj_get_child(ui.root_,i);
    if(lv_obj_check_type(child,&lv_btn_class) || lv_obj_check_type(child,&lv_textarea_class)) {
      lv_area_t area; lv_obj_get_coords(child,&area);
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
int main() {
  lv_init(); static lv_color_t buffer[320*40]; static lv_disp_draw_buf_t draw;
  lv_disp_draw_buf_init(&draw,buffer,nullptr,320*40);
  static lv_disp_drv_t display; lv_disp_drv_init(&display);
  display.hor_res=320; display.ver_res=240; display.flush_cb=flush; display.draw_buf=&draw;
  lv_disp_drv_register(&display);
  static AuroraUI ui; ui.begin(); ui.selfTestPending_=false;
  using Screen=AuroraUI::Screen;
  ui.show(Screen::Passphrase); snapshot(ui,"passphrase.ppm");
  assert(ui.passArea_ && ui.passConfirmArea_);
  lv_textarea_set_text(ui.passArea_,"same"); lv_textarea_set_text(ui.passConfirmArea_,"same");
  assert(ui.confirmPassphrase() && !strcmp(ui.passphrase_,"same"));
  ui.show(Screen::Mode); ui.protectedSession_=true; ui.show(Screen::PinSetup);
  snapshot(ui,"pin-setup.ppm");
  lv_textarea_set_text(ui.pinArea_,"1234"); lv_textarea_set_text(ui.pinConfirmArea_,"1234");
  lv_event_send(ui.keyboard_,LV_EVENT_READY,nullptr);
  assert(ui.screen_==Screen::Info && ui.pinGuard_.enabled()); snapshot(ui,"info.ppm");
  ui.qrContent_=AuroraUI::QrContent::PrivateKey; ui.show(Screen::Qr);
  assert(ui.screen_==Screen::PinUnlock); snapshot(ui,"pin-unlock.ppm");
  for(unsigned i=0;i<3;++i) {
    mock.time+=1000000; lv_textarea_set_text(ui.pinArea_,"0000");
    lv_event_send(ui.keyboard_,LV_EVENT_READY,nullptr);
  }
  assert(ui.screen_==Screen::Mode && !ui.pinGuard_.enabled() && !ui.passphrase_[0]);
  ui.show(Screen::Entropy); snapshot(ui,"entropy.ppm"); ui.show(Screen::Mode);
  assert(wipes>100);
  puts("PASS: real LVGL 8 CYD 320x240, dual passphrase, numeric PIN setup/unlock, 3 failures wipe, secure allocator");
}
