#include "key.hpp"
#include "pico/stdlib.h"

static uint32_t map_clamp(uint32_t value, uint32_t from_start,
                          uint32_t from_end, uint32_t to_start,
                          uint32_t to_end) {
  uint32_t result;
  if (value > from_end)
    result = to_end;
  else if (value < from_end)
    result = to_start;
  else {
    uint32_t from_length = from_end - from_start;
    uint32_t to_length = to_end - to_start;
    result = (value - from_start) * to_length / from_length + to_start;
  }

  return result;
}

void Key::run_task(uint64_t current_time) {
  adc_select_input(m_analog_pin - 26);
  bool last_button_state = m_button_state;
  m_button_state = gpio_get(m_switch_pin);
  m_analog_value = adc_read();

  switch (m_calibration_state) {
  case CalibrationState::Calibrating: {
    if (m_analog_min > m_analog_value)
      m_analog_min = m_analog_value;

    if (m_analog_max < m_analog_value)
      m_analog_max = m_analog_value;

    if (m_button_state != last_button_state && m_button_state == 0) {
      m_analog_mid_samples += 1;
      m_analog_mid += m_analog_value;
    }

    gpio_put(m_led_pin, (current_time / 500000) % 2);
  } break;

  case CalibrationState::Calibrated: {
    if (m_analog_value < m_deadzone_enter_threshold)
      m_rapid_trigger_state = RapidTriggerState::Deadzone;

    switch (m_rapid_trigger_state) {
    case RapidTriggerState::Deadzone:
      if (m_analog_value > m_deadzone_leave_threshold)
        m_rapid_trigger_state = RapidTriggerState::Liftup;
      m_checkpoint = m_analog_value;
      break;
    case RapidTriggerState::Touchdown:
      if (m_checkpoint > m_analog_value)
        m_checkpoint = m_analog_value;
      if (m_analog_value - m_checkpoint > m_state_toggle_distance)
        m_rapid_trigger_state = RapidTriggerState::Liftup;
      break;
    case RapidTriggerState::Liftup:
      if (m_checkpoint < m_analog_value)
        m_checkpoint = m_analog_value;
      if (m_checkpoint - m_analog_value > m_state_toggle_distance)
        m_rapid_trigger_state = RapidTriggerState::Touchdown;
      break;
    }
  }
  case CalibrationState::Uncalibrated: {
    gpio_put(m_led_pin, !is_pressed());
  } break;
  }

  m_last_read_time = current_time;
}

Key::Key(uint led_pin, uint switch_pin, uint analog_pin)
    : m_led_pin(led_pin), m_switch_pin(switch_pin), m_analog_pin(analog_pin),
      m_calibration_state(CalibrationState::Uncalibrated),
      m_analog_min(UINT16_MAX), m_analog_max(0), m_analog_value(0) {
  gpio_init(m_led_pin);
  gpio_init(m_switch_pin);
  gpio_set_dir(m_led_pin, GPIO_OUT);
  gpio_set_dir(m_switch_pin, GPIO_IN);
  gpio_pull_up(m_switch_pin);
  adc_gpio_init(m_analog_pin);
}

void Key::start_calibration() {
  m_analog_max = 0;
  m_analog_min = UINT16_MAX;
  m_analog_mid = 0;
  m_analog_mid_samples = 0;
  m_calibration_state = CalibrationState::Calibrating;
}

void Key::stop_calibration() {
  if (m_analog_mid_samples > 0)
    m_analog_mid /= m_analog_mid_samples;
  m_calibration_state = CalibrationState::Calibrated;
  m_deadzone_leave_threshold = m_analog_mid;
  m_deadzone_enter_threshold = m_analog_min + (m_analog_mid - m_analog_min) / 2;
  m_rapid_trigger_state = RapidTriggerState::Deadzone;
  m_state_toggle_distance = (m_analog_max - m_analog_min) / 4;
}

uint32_t Key::get_mapped_value() {
  return map_clamp(m_analog_value, m_analog_min, m_analog_max, 0, 4000);
}

bool Key::is_pressed() {
  switch (m_calibration_state) {
  case CalibrationState::Calibrated:
    return m_rapid_trigger_state == RapidTriggerState::Liftup;
  case CalibrationState::Uncalibrated:
    return !m_button_state;
  default:
    return false;
  }
}
