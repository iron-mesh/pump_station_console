#ifndef CUSTOM_FUNCTIONS_H
#define CUSTOM_FUNCTIONS_H
#include "LiquidCrystal_I2C.h"
#include "math.h"

float pressureADC_to_float(uint16_t a){
  if(a <= 102) return 0.0;
  return (a - 102) * 0.01481;
}

uint16_t float_to_pressureADC(float a){
  return round(a * 67.5 + 102);
}

void create_custom_chars(LiquidCrystal_I2C &lcd){
  uint8_t letter_zh[] = {
    0b10101,
    0b10101,
    0b10101,
    0b01110,
    0b10101,
    0b10101,
    0b10101,
    0b00000};
    lcd.createChar(0,letter_zh);

    uint8_t letter_i[8]={
      0b10001,
      0b10001,
      0b10001,
      0b10011,
      0b10101,
      0b10101,
      0b11001,
      0b00000};
    lcd.createChar(1,letter_i);

    uint8_t letter_b[8]={
      0b11111,
      0b10000,
      0b10000,
      0b11110,
      0b10001,
      0b10001,
      0b11110,
      0b00000};
    lcd.createChar(2,letter_b);

    uint8_t letter_ii[8]={
      0b10001,
      0b10001,
      0b10001,
      0b11001,
      0b10101,
      0b10101,
      0b11001,
      0b00000};
    lcd.createChar(3,letter_ii);

    uint8_t letter_l[8]={
      0b00011,
      0b00101,
      0b01001,
      0b01001,
      0b01001,
      0b01001,
      0b10001,
      0b00000};
    lcd.createChar(4,letter_l);

    uint8_t letter_p[8]={
      0b11111,
      0b10001,
      0b10001,
      0b10001,
      0b10001,
      0b10001,
      0b10001,
      0b00000};
    lcd.createChar(5,letter_p);

    uint8_t letter_ss[8]={
      0b10000,
      0b10000,
      0b10000,
      0b11110,
      0b10001,
      0b10001,
      0b11110,
      0b00000};
    lcd.createChar(6,letter_ss);

    uint8_t letter_ya[8]={
      0b01111,
      0b10001,
      0b10001,
      0b01111,
      0b00101,
      0b01001,
      0b10001,
      0b00000};
    lcd.createChar(7,letter_ya);
}

#if (DEBUG_MODE == 1)
  #define DEBUG(x) Serial.println(x);
  #else
  #define DEBUG(x)
#endif

#endif
