#pragma once
#include "sd_pwr_ctrl.h"
struct sdmmc_card_t { int fixture=1; };
struct sdmmc_host_t { int slot=0; sd_pwr_ctrl_handle_t pwr_ctrl_handle=nullptr; };
struct sdmmc_slot_config_t { int width=0; };
#define SDMMC_HOST_DEFAULT() sdmmc_host_t{}
#define SDMMC_SLOT_CONFIG_DEFAULT() sdmmc_slot_config_t{}
#define SDMMC_HOST_SLOT_0 0
#define ESP_OK 0
