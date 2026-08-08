#pragma once

// ESP32-2432S028(R) "Cheap Yellow Display". Modifiez ici pour un clone différent.
#define AURORA_SCREEN_WIDTH  320
#define AURORA_SCREEN_HEIGHT 240
#define AURORA_TFT_ROTATION  1
#define AURORA_BACKLIGHT_PIN 21

// Lecteur microSD intégré (bus HSPI séparé sur le modèle courant).
#define AURORA_SD_CS   5
#define AURORA_SD_MOSI 23
#define AURORA_SD_MISO 19
#define AURORA_SD_CLK  18
#define AURORA_SD_FREQUENCY 10000000

// Bus tactile séparé (VSPI) du modèle ESP32-2432S028R le plus courant.
#define AURORA_TOUCH_CS   33
#define AURORA_TOUCH_IRQ  36
#define AURORA_TOUCH_MOSI 32
#define AURORA_TOUCH_MISO 39
#define AURORA_TOUCH_CLK  25

// Calibration brute XPT2046 pour rotation paysage 1.
#define AURORA_TOUCH_MIN_X 250
#define AURORA_TOUCH_MAX_X 3850
#define AURORA_TOUCH_MIN_Y 250
#define AURORA_TOUCH_MAX_Y 3850

// Mettre à 1 si les axes de votre dalle sont inversés.
#define AURORA_TOUCH_SWAP_XY 1
#define AURORA_TOUCH_INVERT_X 0
#define AURORA_TOUCH_INVERT_Y 1
