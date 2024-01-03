#include "key.hpp"
#include <stdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

#include "usb_descriptors.h"

#define CALIBRATION_BUTTON_PIN 5

void led_blinking_task(void);
void hid_task(bool key1, bool key2);

inline void calibration_task(uint64_t current_time, struct Key *keys,
                             uint32_t keycount) {
  static bool calibrating = false;
  static uint64_t calibration_time_start;

  if (gpio_get(CALIBRATION_BUTTON_PIN) == 0) {
    calibrating = true;
    calibration_time_start = current_time;
    for (uint i = 0; i < keycount; i++) {
      keys[i].start_calibration();
    }
  }

  if (calibrating) {
    if (current_time - calibration_time_start > 10 * 1000 * 1000) {
      calibrating = false;
      for (uint i = 0; i < keycount; i++) {
        keys[i].stop_calibration();
      }
    }
  }
}

int main() {
  board_init();
  tusb_init();

  adc_init();
  stdio_init_all();

  gpio_init(CALIBRATION_BUTTON_PIN);
  gpio_set_dir(CALIBRATION_BUTTON_PIN, GPIO_IN);
  gpio_pull_up(CALIBRATION_BUTTON_PIN);

  struct Key keys[] = {{17, 7, 27}, {16, 10, 26}};
  const uint KEYCOUNT = sizeof(keys) / sizeof(*keys);

  while (true) {
    uint64_t current_time = time_us_64();

    calibration_task(current_time, keys, KEYCOUNT);

    for (uint i = 0; i < KEYCOUNT; i++) {
      keys[i].run_task(current_time);
    }

    tud_task();
    led_blinking_task();
    hid_task(keys[0].is_pressed(), keys[1].is_pressed());
  }
}
