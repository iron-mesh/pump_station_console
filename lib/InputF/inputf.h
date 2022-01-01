#ifndef INPUTF_H
#define INPUTF_H

#include "GyverEncoder.h"
#include "GyverButton.h"
#include "LiquidCrystal_I2C.h" //https://github.com/johnrickman/LiquidCrystal_I2C/blob/master/LiquidCrystal_I2C.h

class InputF{

private:
  static GButton* _button;
  static Encoder* _encoder;
  static LiquidCrystal_I2C* _lcd;

  template <typename T>
    static T _input_number(T default_value, T min_value, T max_value, T step, T b_step);
  static char _return_result();
  static void _show_message(const __FlashStringHelper* mes, uint16_t a_delay=1000);


public:
  static void attach_lcd_i2c(LiquidCrystal_I2C &lcd);
  static void attach_encoder(Encoder &encoder);
  static void attach_button(GButton &button);

  static bool input_boolean(bool default_value);
  static uint8_t input_mode(uint8_t default_value);
  static float input_float(float default_value, float min_value, float max_value);
  static int input_int(int default_value, int min_value, int max_value);
  static byte select_list_element(char *array[], byte default_index, byte arr_size);
  static  uint32_t input_time_s(uint32_t default_value, uint32_t min_value, uint32_t max_value);

};


#endif
