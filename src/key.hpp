#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdint.h>

class Key {
public:
  enum class CalibrationState { Calibrated, Uncalibrated, Calibrating };
  enum class RapidTriggerState { Deadzone, Touchdown, Liftup };

  const uint m_led_pin;
  const uint m_switch_pin;
  const uint m_analog_pin;

  uint64_t m_last_read_time;
  uint64_t m_calibration_time_start;
  CalibrationState m_calibration_state;

  bool m_button_state;
  uint32_t m_analog_min, m_analog_max;
  uint32_t m_analog_mid_samples, m_analog_mid;
  uint32_t m_analog_value;

  // Rapid Trigger stuff
  RapidTriggerState m_rapid_trigger_state;
  uint32_t m_deadzone_leave_threshold;
  uint32_t m_deadzone_enter_threshold;
  uint32_t m_state_toggle_distance;
  uint32_t m_checkpoint;

  Key(uint led_pin, uint switch_pin, uint analog_pin);

  void run_task(uint64_t current_time);
  void start_calibration();
  void stop_calibration();

  uint32_t get_mapped_value();
  bool is_pressed();
};
