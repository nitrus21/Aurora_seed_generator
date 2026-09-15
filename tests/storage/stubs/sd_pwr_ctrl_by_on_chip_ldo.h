#pragma once
#include "sdmmc_cmd.h"
struct sd_pwr_ctrl_ldo_config_t { int ldo_chan_id=0; };
int sd_pwr_ctrl_new_on_chip_ldo(const sd_pwr_ctrl_ldo_config_t *,sd_pwr_ctrl_handle_t *);
int sd_pwr_ctrl_del_on_chip_ldo(sd_pwr_ctrl_handle_t);
