
#ifndef RGBLED_H
#define RGBLED_H

#include "FastLED.h"
#include <Arduino.h>
// код библиотеки



const byte TURN_OFF = 0;
const byte PERPETUAL = 1;
const byte BLINK = 2;

class RGBLed{
public:
  CRGB led[1];


  RGBLed(int aPin);
  void setMode(int mode);
  void setBlinkingInterval(unsigned long interval);
  void setColor (uint32_t aColor);
  int getPin();
  void process();

private:
  unsigned long _color;
  byte _mode;
  unsigned long _blinking_interval;
  unsigned long _last_blinking_time;
  int _pin;
  bool _is_turned = true;

  void _turnOff();

  void _turnOn();
};

#endif
