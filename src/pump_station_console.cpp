/* version 2.000 */

#include "FastLED.h"
#include "GyverEncoder.h"
#include "GyverButton.h"
#include "TimerOne.h"
#include <RGBLed.h>
#include "LiquidCrystal_I2C.h" //https://github.com/johnrickman/LiquidCrystal_I2C/blob/master/LiquidCrystal_I2C.h
#include <EEPROM.h>
#include <custom_types.h>
#include <custom_functions.h>

#define P_BUTTON1 2 //пин кнопки 1
#define P_RGBLED_INDICATOR 9 // дата пин rgb светодиода
#define P_RELAY_SWITCHER 10 //пин управления реле насоса
#define P_PRESSURE_RELAY_IN 11 //пин контроля реле давления
#define P_FLOAT_IN 12  // пин котроля за поплавком уровня воды
#define P_PRESSURE_SENSOR 1  //аналоговый пин измерения напряжения на датчике давления

#define P_ENCODER_SW 5  //кнопка энкодера
#define P_ENCODER_DT 4 // DT пин энкодера
#define P_ENCODER_CLK 3 //CLK пин энкодера

#define SHOW_STATEMENT_DURATION 60000 // продолжительность вывода на дисплей состояния системы (мс)
#define SHOW_PAGE_DURATION 3000 // продолжительность вывода на дисплей одной страницы (мс)

#define INIT_ADDR 1023  // номер резервной ячейки EEPROM
#define INIT_KEY 114    // ключ первого запуска. 0-254, на выбор

#define MAX_UNBLOCKING_DELAY 1440 // максимальное время задержки разблокировки мин

//flags
bool pressure_relay_state;
bool float_state;
bool is_global_blocking = false;
bool show_statement_noblock = false; // когда true показывает состояние системы (если не установлена блокировка)
bool recording_waterlevels_in_cursession = false; // производить ли регистрацию пороговых значений реле давления

//адреса eeprom
uint16_t EEMEM  unblock_timeout_addr; //таймаут снятия блокировки
float EEMEM tank_volume_addr;
uint16_t EEMEM min_water_level_ADC_addr;
uint16_t EEMEM max_water_level_ADC_addr;

//variables
int update_delay = 100;
RGBLed led_ind = RGBLed(P_RGBLED_INDICATOR);
LiquidCrystal_I2C lcd(0x27,16,2);
GButton button1 (P_BUTTON1);
Encoder encoder1 (P_ENCODER_CLK, P_ENCODER_DT, P_ENCODER_SW, TYPE2);
float tank_volume;
uint16_t  min_water_level_ADC;
uint16_t  max_water_level_ADC;
uint16_t unblock_timeout;
uint32_t unblocking_time_ms; // время разбокировки мс (значение millis())

//объявление функций
bool check_input_statements();
void handle_statements();
void button_handler();
void process_display_statement();
String get_current_water_volume_str();
void record_pressure_thresholds_ADC();
void input_parameters();
bool input_boolean(bool const default_value);
float input_float(float const default_value, float const min_value, float const max_value);
uint16_t input_uint(uint16_t const default_value, uint16_t const min_value, uint16_t const max_value);
void buttons_int();
void encoder1_int();
void try_to_unblock(UnblockingModes mode = UB_AUTO);

void setup() {
  // put your setup code here, to run once:
  pinMode(P_BUTTON1, INPUT_PULLUP);
  pinMode(P_PRESSURE_RELAY_IN, INPUT);
  pinMode(P_FLOAT_IN, INPUT);

  pressure_relay_state = digitalRead(P_PRESSURE_RELAY_IN);
  float_state = digitalRead(P_FLOAT_IN);

  pinMode(P_RGBLED_INDICATOR, OUTPUT);
  pinMode(P_RELAY_SWITCHER, OUTPUT);


  FastLED.addLeds<WS2811, P_RGBLED_INDICATOR, GRB>(led_ind.led, 1).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(25);

  //Serial.begin(9600);
  //Serial.println("Ready to performing");

  led_ind.setColor(0xFF0000);
  led_ind.setMode(TURN_OFF);
  led_ind.setBlinkingInterval(1000);

  lcd.init();    // initialize the lcd
  lcd.noDisplay();
  //lcd.command(0b101010);

  Timer1.initialize(5000);
  Timer1.attachInterrupt(buttons_int);

  //encoder1.setFastTimeout(50);

  handle_statements();

  // инициализация EEPROM в первый раз, иначе чтение
  if (EEPROM.read(INIT_ADDR) != INIT_KEY){
      //Serial.println("first_call");
    EEPROM.write(INIT_ADDR, INIT_KEY);
    tank_volume = 5.0;
    eeprom_write_float(&tank_volume_addr, tank_volume);
    min_water_level_ADC = 102;
    eeprom_write_word(&min_water_level_ADC_addr, min_water_level_ADC);
    max_water_level_ADC = 921;
    eeprom_write_word(&max_water_level_ADC_addr, max_water_level_ADC);
    unblock_timeout = 0;
    eeprom_write_word(&unblock_timeout_addr, unblock_timeout);
  } else {
      //Serial.println("not first_call");
    tank_volume = eeprom_read_float(&tank_volume_addr);
    //Serial.println(tank_volume);
    min_water_level_ADC = eeprom_read_word(&min_water_level_ADC_addr);
    //Serial.println(min_water_level_ADC);
    max_water_level_ADC = eeprom_read_word(&max_water_level_ADC_addr);
    //Serial.println(max_water_level_ADC);
    unblock_timeout = eeprom_read_word(&unblock_timeout_addr);

  }

}



void loop() {
  // put your main code here, to run repeatedly:

if (check_input_statements()) {
  handle_statements();
  if (recording_waterlevels_in_cursession)
    record_pressure_thresholds_ADC();
}

button_handler();

led_ind.process();

process_display_statement();

try_to_unblock(UB_AUTO);

delay(update_delay);
}


//custom functions
bool check_input_statements(){

  bool is_statement_changed = false;

  if (pressure_relay_state != digitalRead(P_PRESSURE_RELAY_IN)){
    pressure_relay_state = digitalRead(P_PRESSURE_RELAY_IN);
    is_statement_changed = true;
  }

  if (float_state != digitalRead(P_FLOAT_IN)){
    float_state = digitalRead(P_FLOAT_IN);
    is_statement_changed = true;
  }

    return is_statement_changed;
}

void handle_statements(){
  //Serial.println("обработка состояний");

  if(!is_global_blocking){ // обработка состояний при отсутствии блокировки

    if(pressure_relay_state)
      digitalWrite(P_RELAY_SWITCHER, HIGH);
    else
      digitalWrite(P_RELAY_SWITCHER, LOW);

    if(!float_state){
      is_global_blocking = true;
      led_ind.setMode(BLINK);
      digitalWrite(P_RELAY_SWITCHER, LOW);
    }
  }

  if(is_global_blocking) { // обработка состояний при наличии блокировки
    if (!float_state){
      if (pressure_relay_state) led_ind.setColor(0xFF0000);
        else led_ind.setColor(0xFF9100);
    } else {
      if(pressure_relay_state) led_ind.setColor(0x0000FF);
        else led_ind.setColor(0x00FF00);
    }
  }

}

void try_to_unblock(UnblockingModes mode = UB_AUTO){
  static bool is_counting_started = false;

  if(is_global_blocking && float_state){

    if(!is_counting_started && unblock_timeout > 0){
        is_counting_started = true;
        unblocking_time_ms = unblock_timeout * 60000 + millis();
      }

      if(mode == UB_MANUAL || (unblock_timeout > 0 && millis() > unblocking_time_ms)){
        is_global_blocking = false;
        is_counting_started = false;
        led_ind.setMode(TURN_OFF);
        handle_statements();
      }
    }
}

void button_handler(){
  //опрос кнопок
  button1.tick();
  encoder1.tick();

  if (button1.isRelease()){
    if (!is_global_blocking)
      show_statement_noblock =! show_statement_noblock;
    else try_to_unblock(UB_MANUAL);

  }

  if(encoder1.isHolded()){
    //переход в режим ввода параметров пользователем, все остальные процессы блокируются до завершения
    digitalWrite(P_RELAY_SWITCHER, LOW); //превентивное отключение насоса
    input_parameters();
    handle_statements();
  }
}

void process_display_statement(){ // обработка отображения состояние системы, работает совместно с флагом show_statement_noblock

  static bool first_call = true;
  static uint32_t end_show_message_ms;
  static uint32_t page_changing_ms = 0;
  static byte current_page = 0;
  static bool print_page = true;

  if(!first_call && (!show_statement_noblock || millis() > end_show_message_ms) && !is_global_blocking){
    first_call = true;
    show_statement_noblock = false;
    lcd.noDisplay();
    lcd.noBacklight();
    return ;
  }

  if (show_statement_noblock || is_global_blocking){

    if (first_call){
      first_call = false;
      lcd.display();
      lcd.backlight();
      end_show_message_ms = millis() + SHOW_STATEMENT_DURATION;
      page_changing_ms = millis() + SHOW_PAGE_DURATION;
      current_page = 0;
    }

    if(print_page){
      switch(current_page){
      case 0: lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Davlenie");
      lcd.setCursor(0,1);
      lcd.print(pressureADC_to_float(analogRead(P_PRESSURE_SENSOR)), 2);
      lcd.print(" Bar"); break;

      case 1: lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Zapas Vody");
      lcd.setCursor(0,1);
      lcd.print(get_current_water_volume_str()); break;

      case 2: lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Uroven' Vody");
      lcd.setCursor(0,1);
      if(float_state) lcd.print("(Normal'nyj)");
        else lcd.print("(Nizkij)");
      break;

      case 3: lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Rele Davleniya");
      lcd.setCursor(0,1);
      if(pressure_relay_state) lcd.print("(Vklyucheno)");
        else lcd.print("(Otklyucheno)");
      break;

      case 4: lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("   Blokirovka!");
      lcd.setCursor(0,1);
      if(float_state) lcd.print("Dostupno Otkl-e");
        else lcd.print("    (Aktivna)");
      break;

      case 5: lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Do Razblokirovki");
      lcd.setCursor(0,1);
      lcd.print((unblocking_time_ms - millis()) / 60000);
      lcd.print(" min");
      break;
    }
      print_page = false;
  }

    if (millis() > page_changing_ms){

      if (!is_global_blocking){
        if(current_page < 1) current_page ++;
          else current_page = 0;
      } else if(unblock_timeout > 0 && float_state){
        if(current_page < 5) current_page ++;
          else current_page = 0;
      } else {if(current_page < 4) current_page ++;
          else current_page = 0;
      }

        page_changing_ms = millis() + SHOW_PAGE_DURATION;
        print_page = true;
    }

  }
}

void input_parameters(){

  //локальные переменные
  byte current_position = 0;
  byte max_position = 4;
  bool refresh_current_position = true;
  String options[][2] = {
    {"1.Ob\"em Baka", "(Litrov)"},
    {"2.Fiksaciya", "Min/Max Davl-ya"},
    {"3.Minimal'noe", "Davlenie, Bar"},
    {"4.Maksimal'noe", "Davlenie, Bar"},
    {"5.Zaderzhka", "Razblokirovki"}
  };

  lcd.display();
  lcd.backlight();
  lcd.clear();
  lcd.home();
  lcd.print("  <Nastrojki>");
  delay(1500);

  while(true){
    if(encoder1.isRight()){
        if(current_position == max_position) current_position = 0;
          else current_position ++;
          refresh_current_position = true;
    }

    if(encoder1.isLeft()){
        if(current_position == 0) current_position = max_position;
          else current_position --;
          refresh_current_position = true;
    }

    if(encoder1.isRelease()){

      switch(current_position){
        case(0): tank_volume = input_float(tank_volume, 5, 500);
        if (abs(eeprom_read_float(&tank_volume_addr) - tank_volume) > 0.1) eeprom_update_float(&tank_volume_addr, tank_volume);
        break;

        case(1): recording_waterlevels_in_cursession = input_boolean(recording_waterlevels_in_cursession);
        if (recording_waterlevels_in_cursession) record_pressure_thresholds_ADC();
        break;

        case(2): min_water_level_ADC = float_to_pressureADC(input_float(pressureADC_to_float(min_water_level_ADC), 0, pressureADC_to_float(max_water_level_ADC)));
        if (eeprom_read_word(&min_water_level_ADC_addr) != min_water_level_ADC) eeprom_update_word(&min_water_level_ADC_addr, min_water_level_ADC);
        break;

        case(3): max_water_level_ADC = float_to_pressureADC(input_float(pressureADC_to_float(max_water_level_ADC), pressureADC_to_float(min_water_level_ADC), 12));
        if (eeprom_read_word(&max_water_level_ADC_addr) != max_water_level_ADC) eeprom_update_word(&max_water_level_ADC_addr, max_water_level_ADC);
        break;

        case(4): unblock_timeout = input_uint(unblock_timeout, 0, MAX_UNBLOCKING_DELAY);
        if(eeprom_read_word(&unblock_timeout_addr) != unblock_timeout) eeprom_update_word(&unblock_timeout_addr, unblock_timeout);
        break;
      }

      refresh_current_position = true;
    }

    if(encoder1.isHolded()) {
      if(!show_statement_noblock && !is_global_blocking){
        lcd.noDisplay();
        lcd.noBacklight();
      }
      return ;
    }

    if(refresh_current_position){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(options[current_position][0]);
      lcd.setCursor(0,1);
      lcd.print(options[current_position][1]);
      refresh_current_position = false;
    }

  }
}

float input_float(float const default_value, float const min_value, float const max_value){
  float current_value = default_value;
  bool value_changed = false;
  lcd.clear();
  lcd.print(current_value, 1);

  while(true){

    if (encoder1.isRight()){
      if(current_value < max_value) {
        current_value += 0.1;
        value_changed = true;}
    }

    if (encoder1.isLeft()){
      if(current_value > min_value) {
        current_value -= 0.1;
        value_changed = true;}
    }

    if (encoder1.isFastR()){
      if(current_value < max_value - 5) {
        current_value += 5;
        value_changed = true;}
    }

    if (encoder1.isFastL()){
      if(current_value > min_value + 5) {
        current_value -= 5;
        value_changed = true;}
    }

    if(value_changed){
      lcd.clear();
      lcd.print(current_value, 1);
      value_changed = false;
    }

    if(button1.isRelease()) return default_value;

    if(encoder1.isRelease()) return current_value;

  }

}

uint16_t input_uint(uint16_t const default_value, uint16_t const min_value, uint16_t const max_value){
  uint16_t current_value = default_value;
  bool value_changed = true;

  while(true){

    if (encoder1.isRight()){
      if(current_value < max_value) {
        current_value += 1;
        value_changed = true;}
    }

    if (encoder1.isLeft()){
      if(current_value > min_value) {
        current_value -= 1;
        value_changed = true;}
    }

    if (encoder1.isFastR()){
      if(current_value < max_value - 10) {
        current_value += 10;
        value_changed = true;}
    }

    if (encoder1.isFastL()){
      if(current_value > min_value + 10) {
        current_value -= 10;
        value_changed = true;}
    }

    if(value_changed){
      lcd.clear();
      lcd.print(current_value);
      value_changed = false;
    }

    if(button1.isRelease()) return default_value;

    if(encoder1.isRelease()) return current_value;

  }

}

bool input_boolean(bool const default_value){
  bool current_value = default_value;

  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Da");
  lcd.setCursor(1, 1);
  lcd.print("Net");
  if (current_value) lcd.setCursor(0, 0);
    else lcd.setCursor(0, 1);
  lcd.print(">");

  while(true){

    if (encoder1.isLeft() || encoder1.isRight()){
      current_value =!current_value;

      if(current_value) {
        lcd.setCursor(0,0);
        lcd.print(">");
        lcd.setCursor(0,1);
        lcd.print(" ");
      } else {
        lcd.setCursor(0,0);
        lcd.print(" ");
        lcd.setCursor(0,1);
        lcd.print(">");
      }
    }

    if(button1.isRelease()) return default_value;

    if(encoder1.isRelease()) return current_value;

  }

}



String get_current_water_volume_str(){ //расчет оставшегося обЪема воды в баке проценты и литры
  uint16_t sensor_val = analogRead(P_PRESSURE_SENSOR);
  float water_volume_fraction = (sensor_val < min_water_level_ADC) ? 0 : (float)(sensor_val - min_water_level_ADC) / (max_water_level_ADC - min_water_level_ADC);
  return String(tank_volume * water_volume_fraction, 1) + "L (" + String(water_volume_fraction * 100, 0) + "%)";
}

void buttons_int(){
  button1.tick();
  encoder1.tick();
}

void record_pressure_thresholds_ADC(){ //регистрация пороговый значений давления реле
  static uint16_t min_pressure_fixed_ADC = 0;
  static uint16_t max_pressure_fixed_ADC = 0;
  static bool is_first_call = true;
  static bool current_status;

  if(is_first_call){
    current_status = pressure_relay_state;
    is_first_call = false;
  }else{

    if (!current_status && pressure_relay_state){
      min_pressure_fixed_ADC = analogRead(P_PRESSURE_SENSOR);
      current_status = pressure_relay_state;
    }

    if(current_status && !pressure_relay_state){
      max_pressure_fixed_ADC = analogRead(P_PRESSURE_SENSOR);
      current_status = pressure_relay_state;
    }

    if (min_pressure_fixed_ADC > 0 && max_pressure_fixed_ADC > 0){
      min_water_level_ADC = min_pressure_fixed_ADC;
      max_water_level_ADC = max_pressure_fixed_ADC;
      eeprom_update_word(&min_water_level_ADC_addr, min_water_level_ADC);
      eeprom_update_word(&max_water_level_ADC_addr, max_water_level_ADC);
      recording_waterlevels_in_cursession = false;
      is_first_call = true;
    }
  }

}
