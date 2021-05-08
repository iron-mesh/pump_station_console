

#include "RGBLed.h"


RGBLed::RGBLed(int aPin){
  _pin = aPin;
  _last_blinking_time = 0;
  _blinking_interval = 1000;
}

void RGBLed::setMode(int mode){
  _mode = mode;
  switch (mode){
      case TURN_OFF: _turnOff();
      break;
      case PERPETUAL: _turnOn();
      break;
  }
}

 void RGBLed::setBlinkingInterval(unsigned long interval){
   _blinking_interval = interval;
}

void RGBLed::setColor (uint32_t aColor){
  _color = aColor;
  if (_mode == PERPETUAL) _turnOn();
}

void RGBLed::process(){
  if (_mode == BLINK && (millis() - _last_blinking_time) >= _blinking_interval){
    if(_is_turned) _turnOff(); else _turnOn ();
    _is_turned = !_is_turned;
    _last_blinking_time = millis();
  }
}

int RGBLed::getPin(){
  return _pin;
}

void RGBLed::_turnOff(){
  led[0] = 0;
  FastLED.show();
}

void RGBLed::_turnOn(){
  led[0] = _color;
  FastLED.show();
}
