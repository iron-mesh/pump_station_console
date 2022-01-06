
#ifndef PumpStation_h
#define PumpStation_h


#include <Arduino.h>
#include "custom_functions.h"
// код библиотеки

// коды статусов
#define ID_FLOAT_STATUS 0
#define ID_PRELAY_STATUS 1
#define ID_PSENSOR_OK_STATUS 2
#define ID_CRITICAL_PRESSURE_STATUS 3
#define ID_LOW_PRESSURE_STATUS 4

#define ID_GLOBAL_BLOCKING_STATUS 5
#define ID_ACTIVITY_STATUS 6

// режимы работы класса
#define PRELAY_MODE 0
#define SENSOR_MODE 1


class PumpStation{
public:
  PumpStation(int prelay_pin, int float_pin, int sensor_pin, int pumpswitch_pin){
    _prelay_pin = prelay_pin;
    _float_pin = float_pin;
    _sensor_pin = sensor_pin;
    _pumpswitch_pin = pumpswitch_pin;

    pinMode(_prelay_pin, INPUT);
    pinMode(_float_pin, INPUT);
    pinMode(_sensor_pin, INPUT);
    pinMode(_pumpswitch_pin, OUTPUT);
    _set_pumpswitch_status(LOW);

    _min_pressure_ADC = 0;
    _max_pressure_ADC = 0;
    _tank_volume = 0;
    _critical_pressure_ADC = 0;
    _corr_pressure_ADC = 0;
    _statement_changed_handler = NULL;
    _status_flags = 0;
    _mode = PRELAY_MODE;
  }

  void process(){
    if(_update_statuses()){
       _process();
      _statement_changed_handler();
    }
  }

  int activate(){
    if(bitRead(_status_flags, ID_ACTIVITY_STATUS)) return -1;
      else if ((_min_pressure_ADC < _max_pressure_ADC) &&
                (_min_pressure_ADC > 102) &&
                (_max_pressure_ADC < 912)) {
                    bitSet(_status_flags, ID_ACTIVITY_STATUS);
                    _update_statuses();
                    _process();
                    _statement_changed_handler();
                    return 0;
                } else return -1;
  }

  void deactivate(){
    _set_pumpswitch_status(LOW);
    bitClear(_status_flags, ID_ACTIVITY_STATUS);
    _statement_changed_handler();
  }

  bool is_global_blocking_conditions(){
    return (
      ((_mode == SENSOR_MODE) && !bitRead(_status_flags, ID_PSENSOR_OK_STATUS )) ||
      !bitRead(_status_flags, ID_FLOAT_STATUS) ||
      (_critical_pressure_ADC > 110 && bitRead(_status_flags, ID_CRITICAL_PRESSURE_STATUS))
    );
  }

  int deblock(){
    if (bitRead(_status_flags, ID_GLOBAL_BLOCKING_STATUS) && !is_global_blocking_conditions()){
      bitClear(_status_flags, ID_GLOBAL_BLOCKING_STATUS);
      _update_statuses();
      _process();
      _statement_changed_handler();
      return 0;
    } else return -1;
  }

  void attach_status_changed_handler(void (*handler)() = NULL){
    _statement_changed_handler = handler;
  }

  bool get_status_by_id(uint8_t id){
    return bitRead(_status_flags, id);
  }

  uint8_t get_mode(){
    return _mode;
  }

  void set_mode(uint8_t mode){
    _mode = mode;
  }

  float get_pressure_bar(){
    return pressureADC_to_float(_current_pressure_ADC);
  }

  uint16_t get_pressure_ADC(){
    return _current_pressure_ADC;
  }

  float get_min_pressure_bar(){
    return pressureADC_to_float(_min_pressure_ADC);
  }

  uint16_t get_min_pressure_ADC(){
    return _min_pressure_ADC;
  }

  float get_max_pressure_bar(){
    return pressureADC_to_float(_max_pressure_ADC);
  }

  uint16_t get_max_pressure_ADC(){
    return _max_pressure_ADC;
  }

  float get_critical_pressure_bar(){
    return pressureADC_to_float(_critical_pressure_ADC);
  }

  uint16_t get_critical_pressure_ADC(){
    return _critical_pressure_ADC;
  }

  void set_min_pressure_bar(float value){
    _min_pressure_ADC = float_to_pressureADC(value);
  }

  void set_min_pressure_ADC(uint16_t value){
    _min_pressure_ADC = value;
  }

  void set_critical_pressure_bar(float value){
    _critical_pressure_ADC = float_to_pressureADC(value);
  }

  void set_critical_pressure_ADC(uint16_t value){
    _critical_pressure_ADC = value;
  }

  void set_max_pressure_bar(float value){
    _max_pressure_ADC = float_to_pressureADC(value);
  }

  void set_max_pressure_ADC(uint16_t value){
    _max_pressure_ADC = value;
  }

  void set_press_corr_bar(float value){
    _corr_pressure_ADC = float_to_pressureADC(value);
  }

  void set_press_corr_ADC(uint16_t value){
    _corr_pressure_ADC = value;
  }

  float get_press_corr_bar(){
    return pressureADC_to_float(_corr_pressure_ADC);
  }

  uint16_t get_press_corr_ADC(){
    return _corr_pressure_ADC;
  }

  void set_tank_volume(float volume){
    _tank_volume = volume;
  }
  float get_tank_volume(){
    return _tank_volume;
  }

  float get_water_reserveL(){
    return (_current_pressure_ADC < _min_pressure_ADC) ? 0 : ((float)(_current_pressure_ADC - _min_pressure_ADC) / (_max_pressure_ADC - _min_pressure_ADC - (_corr_pressure_ADC-102))) * _tank_volume;
  }

  int get_water_reserve_percent(){
      return (_current_pressure_ADC < _min_pressure_ADC) ? 0 : trunc(((float)(_current_pressure_ADC - _min_pressure_ADC) / (_max_pressure_ADC - _min_pressure_ADC - (_corr_pressure_ADC-102))) * 100);
  }

private:
  void _set_global_blocking(){
    _set_pumpswitch_status(LOW);
    bitSet(_status_flags, ID_GLOBAL_BLOCKING_STATUS);
    _statement_changed_handler();
  }

  bool _update_statuses(){
    bool status_changed = false;
    _current_pressure_ADC = analogRead(_sensor_pin);

    switch(_mode){
      case PRELAY_MODE:
        if (bitRead(_status_flags, ID_PRELAY_STATUS) != digitalRead(_prelay_pin)){
          _status_flags ^= (1 << ID_PRELAY_STATUS);
          status_changed = true;
        }
      break;
      case SENSOR_MODE:
        const bool lps = bitRead(_status_flags, ID_LOW_PRESSURE_STATUS);
        if (((_current_pressure_ADC <= _min_pressure_ADC) && !lps )||
            (_current_pressure_ADC >= _max_pressure_ADC && lps)){
        _status_flags ^= (1 << ID_LOW_PRESSURE_STATUS);
        status_changed = true;
        }
      break;
    }

    if (bitRead(_status_flags, ID_FLOAT_STATUS) != digitalRead(_float_pin)){
      _status_flags ^= (1 << ID_FLOAT_STATUS);
      status_changed = true;
    }

    if (_critical_pressure_ADC > 110){ // 110 - в переводе около 0,12 бар
      bool cur_critical_pressure_s = (_current_pressure_ADC >= _critical_pressure_ADC);
      bool prev_critical_pressure_s = bitRead(_status_flags, ID_CRITICAL_PRESSURE_STATUS);

      if (prev_critical_pressure_s != cur_critical_pressure_s){
        _status_flags ^= (1 << ID_CRITICAL_PRESSURE_STATUS);
        status_changed = true;
          }
      }

    if (bitRead(_status_flags, ID_PSENSOR_OK_STATUS) != (_current_pressure_ADC > 95 && _current_pressure_ADC < 920)){
      _status_flags ^= (1 << ID_PSENSOR_OK_STATUS);
      status_changed = true;
    }

    return status_changed;
  }

  void _process(){
    if(!bitRead(_status_flags, ID_GLOBAL_BLOCKING_STATUS) &&
          bitRead(_status_flags, ID_ACTIVITY_STATUS)) { // обработка состояний при отсутствии блокировки
      switch(_mode){
        case PRELAY_MODE:
          _set_pumpswitch_status(bitRead(_status_flags, ID_PRELAY_STATUS));
          break;
        case SENSOR_MODE:
          _set_pumpswitch_status(bitRead(_status_flags, ID_LOW_PRESSURE_STATUS));
          break;
      }

      if(is_global_blocking_conditions()) _set_global_blocking();

    }
  }

  void _set_pumpswitch_status(bool status){
    digitalWrite(_pumpswitch_pin, status);
  }

  void (*_statement_changed_handler)(); //ссылка на обработчик, вызывающий при изменении состояния систем
  uint8_t _status_flags; // флаги: положение попалавка, реле давления, глобальный блокировки, статус работы класса
  uint16_t _current_pressure_ADC, // текущее значение давления, измеренное АЦП
           _min_pressure_ADC, //минимальное давление (в единицах АЦП)
           _max_pressure_ADC, //максимальное давление (в единицах АЦП)
           _critical_pressure_ADC, // критическое давление (в единицах АЦП)
           _corr_pressure_ADC; // корректировка давления (в единицах АЦП)

  uint8_t _prelay_pin, _float_pin, _sensor_pin, _pumpswitch_pin,  //номеры пинов для реледавления, поплавка, датчика давления, реле питания насоса
    _mode; // режим работы
  float _tank_volume; //объем бака



};
#endif
