#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/board.h"
#include "device/usbd.h"
#include "tusb.h"

#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 0,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void hid_task(void);
//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) { blink_interval_ms = BLINK_MOUNTED; }

// Invoked when device is unmounted
void tud_umount_cb(void) { blink_interval_ms = BLINK_NOT_MOUNTED; }

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  (void)remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) { blink_interval_ms = BLINK_MOUNTED; }

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

static void send_hid_report(bool key1, bool key2) {
  // skip if hid is not ready yet
  if (!tud_hid_ready())
    return;

  static bool has_keyboard_key = false;
  uint8_t keycode[6] = {0};

  if (key1) {
    keycode[0] = HID_KEY_S;
  }

  if (key2) {
    keycode[1] = HID_KEY_D;
  }

  if (key1 || key2) {
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
    has_keyboard_key = true;
  }

  if (has_keyboard_key && !(key1 || key2)) {
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
    has_keyboard_key = false;
  }
}

void hid_task(bool key1, bool key2) {
  // Poll every 10ms
  const uint32_t interval_ms = 5;
  static uint32_t usb_start_ms = 0;

  if (board_millis() - usb_start_ms < interval_ms)
    return; // not enough time
  usb_start_ms += interval_ms;

  // Remote wakeup
  if (tud_suspended() && (key1 || key2)) {
    tud_remote_wakeup();
  } else {
    send_hid_report(key1, key2);
  }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report,
                                uint16_t len) {
  (void)instance;
  (void)len;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  // TODO not Implemented
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void) {
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // blink is disabled
  if (!blink_interval_ms)
    return;

  // Blink every interval ms
  if (board_millis() - start_ms < blink_interval_ms)
    return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}
