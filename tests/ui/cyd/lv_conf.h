#include "../../../include/lv_conf.h"
#undef LV_TICK_CUSTOM_INCLUDE
#define LV_TICK_CUSTOM_INCLUDE "native_tick.h"
#undef LV_TICK_CUSTOM_SYS_TIME_EXPR
#define LV_TICK_CUSTOM_SYS_TIME_EXPR native_millis()
