/* version 2.000 */

#include <FastLED.h>

#include <TimerOne.h>

#include <CyberLib.h>
#include <TimerMs.h>


#include "custom_types.h"

#define DEBUG_MODE 0
#include "custom_functions.h"

#include <EEPROM.h>

#include "RGBLed.h"
#include "InputF.h"
#include "PumpStation.h"

#define VERSION "2.0.1"

#define P_BUTTON1 2 //пин кнопки 1
#define P_RGBLED_INDICATOR 9 // дата пин rgb светодиода
#define P_RELAY_SWITCHER 10 //пин управления реле насоса
#define P_PRESSURE_RELAY_IN 11 //пин контроля реле давления
#define P_FLOAT_IN 12  // пин котроля за поплавком уровня воды
#define P_PRESSURE_SENSOR 1  //аналоговый пин измерения напряжения на датчике давления

#define P_ENCODER_SW 5  //кнопка энкодера
#define P_ENCODER_DT 4 // DT пин энкодера
#define P_ENCODER_CLK 3 //CLK пин энкодера

#define MAIN_CYCLE_PERIOD 200 // период обработки главного цикла
#define DISPLAY_UPDATE_PERIOD 1000 // период обновления дисплея

#define INIT_ADDR 1023  // номер резервной ячейки EEPROM
#define INIT_KEY 69    // ключ первого запуска. 0-254, на выбор

#define MAX_UNBLOCKING_DELAY 1440 // максимальное время задержки разблокировки мин
#define DELTA_P 0.1  //минимальная разница между максимальным и минимальным давлениями

//flags


//адреса eeprom
uint16_t EEMEM press_corr_ADC_addr;
uint32_t EEMEM show_page_duration_addr; // время показа 1 стр. состояния системы, сек
uint32_t EEMEM show_statement_duration_addr; //время показа состояния системы, сек
uint16_t EEMEM critical_pressure_ADC_addr;
uint8_t EEMEM operation_mode_addr; // режим работы подпрограммы PumpStation
uint32_t EEMEM  unblock_timeout_addr; //таймаут снятия блокировки
float EEMEM tank_volume_addr;
uint16_t EEMEM min_pressure_ADC_addr;
uint16_t EEMEM max_pressure_ADC_addr;

//global variables
RGBLed led_ind = RGBLed(P_RGBLED_INDICATOR);
LiquidCrystal_I2C lcd(0x27,16,2);
GButton button1 (P_BUTTON1);
Encoder encoder1 (P_ENCODER_CLK, P_ENCODER_DT, P_ENCODER_SW, TYPE2);
uint32_t unblock_timeout_s; // задержка выполнения разболокировки
uint32_t time_until_deblock = 0; // оставшееся время до снятия блокировки
uint32_t show_page_duration; // время показа 1 стр. состояния системы, сек
uint32_t show_statement_duration; //время показа состояния системы, сек
bool show_statement_noblock = false;
PumpStation ps = PumpStation(P_PRESSURE_RELAY_IN, P_FLOAT_IN, P_PRESSURE_SENSOR, P_RELAY_SWITCHER);

//объявление функций

void button_handler();
void process_display_statement(char scroll_dir = '0');
void input_parameters();
void buttons_int();
void try_to_unblock(UnblockingModes mode = UB_AUTO);
void on_statement_changed();
void render_page(uint8_t page_num);

void setup() {
  // put your setup code here, to run once:

  pinMode(P_RGBLED_INDICATOR, OUTPUT);

  FastLED.addLeds<WS2811, P_RGBLED_INDICATOR, GRB>(led_ind.led, 1).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(25);

  #if (DEBUG_MODE == 1)
    Serial.begin(9600);
    Serial.println("Ready to performing. Debug mode");
  #endif

  led_ind.setColor(0xFF0000);
  led_ind.setMode(TURN_OFF);
  led_ind.setBlinkingInterval(1000);

  lcd.init();    // initialize the lcd
  lcd.noDisplay();
  create_custom_chars(lcd);

  Timer1.initialize(5000);
  Timer1.attachInterrupt(buttons_int);

  InputF::attach_button(button1);
  InputF::attach_encoder(encoder1);
  InputF::attach_lcd_i2c(lcd);

  //encoder1.setFastTimeout(50);


  // инициализация EEPROM в первый раз, иначе чтение
  if (EEPROM.read(INIT_ADDR) != INIT_KEY){
      //Serial.println("first_call");
    EEPROM.write(INIT_ADDR, INIT_KEY);
    ps.set_press_corr_bar(0.0f);
    eeprom_write_word(&press_corr_ADC_addr, ps.get_press_corr_ADC());
    ps.set_tank_volume(5.00f);
    eeprom_write_float(&tank_volume_addr, ps.get_tank_volume());
    ps.set_min_pressure_bar(2.00f);
    eeprom_write_word(&min_pressure_ADC_addr, ps.get_min_pressure_ADC());
    ps.set_max_pressure_bar(5.00f);
    eeprom_write_word(&max_pressure_ADC_addr, ps.get_max_pressure_ADC());
    unblock_timeout_s = 0;
    eeprom_write_dword(&unblock_timeout_addr, unblock_timeout_s);
    ps.set_mode(PRELAY_MODE);
    eeprom_write_byte(&operation_mode_addr, ps.get_mode());
    ps.set_critical_pressure_bar(8.00f);
    eeprom_write_word(&critical_pressure_ADC_addr, ps.get_critical_pressure_ADC());
    show_page_duration = 3;
    eeprom_write_dword(&show_page_duration_addr, show_page_duration);
    show_statement_duration = 60;
    eeprom_write_dword(&show_statement_duration_addr, show_statement_duration);
  } else {
    ps.set_press_corr_ADC(eeprom_read_word(&press_corr_ADC_addr));
    ps.set_tank_volume(eeprom_read_float(&tank_volume_addr));
    ps.set_min_pressure_ADC(eeprom_read_word(&min_pressure_ADC_addr));
    ps.set_max_pressure_ADC(eeprom_read_word(&max_pressure_ADC_addr));
    ps.set_critical_pressure_ADC(eeprom_read_word(&critical_pressure_ADC_addr));
    ps.set_mode(eeprom_read_byte(&operation_mode_addr));
    unblock_timeout_s = eeprom_read_dword(&unblock_timeout_addr);
    show_page_duration = eeprom_read_dword(&show_page_duration_addr);
    show_statement_duration = eeprom_read_dword(&show_statement_duration_addr);
  }

  ps.attach_status_changed_handler(on_statement_changed);
  ps.activate(); //активация PumpStation

  wdt_enable(WDTO_1S);

}



void loop() {
  // put your main code here, to run repeatedly:
  static TimerMs main_cycle_timer(MAIN_CYCLE_PERIOD, 1, 0);
  static TimerMs disp_update_timer(DISPLAY_UPDATE_PERIOD, 1, 0);

  if(main_cycle_timer.tick()){

    ps.process();

    button_handler();

    led_ind.process();

    try_to_unblock();

    wdt_reset();
  }

  if(disp_update_timer.tick()){
    process_display_statement();
  }

}


//custom functions
void on_statement_changed(){
  DEBUG(F("on_statement_changed"))
  if(ps.get_status_by_id(ID_GLOBAL_BLOCKING_STATUS)){
    if(ps.is_global_blocking_conditions()) led_ind.setColor(0xFF0000);
      else led_ind.setColor(0x00FF00);
    led_ind.setMode(BLINK);
  } else {
    led_ind.setMode(TURN_OFF);
  }
}

void try_to_unblock(UnblockingModes mode = UB_AUTO){
  static TimerMs timer(1000, 0, 0);

  if(ps.get_status_by_id(ID_GLOBAL_BLOCKING_STATUS) && !ps.is_global_blocking_conditions()){

      if(!timer.active() && unblock_timeout_s > 0){
          timer.start();
          time_until_deblock = unblock_timeout_s;
        }

      if(timer.tick()) time_until_deblock --;

      if(mode == UB_MANUAL || (timer.active() && time_until_deblock == 0)){
          if (ps.deblock () == 0) {
            timer.stop();
            time_until_deblock = 0;
            }
            else if (unblock_timeout_s > 0){
              timer.start();
              time_until_deblock = unblock_timeout_s;
            }
        }
    } else if(timer.active()){
      timer.stop();
      time_until_deblock = 0;
    }
}

void button_handler(){
  //опрос кнопок
  button1.tick();
  encoder1.tick();

  if (button1.isRelease()){
    if (!ps.get_status_by_id(ID_GLOBAL_BLOCKING_STATUS)) show_statement_noblock =!show_statement_noblock;
      else try_to_unblock(UB_MANUAL);
  }

  if(encoder1.isRight()) process_display_statement('R');
  if(encoder1.isLeft()) process_display_statement('L');

  if(encoder1.isHolded()){
    //переход в режим ввода параметров пользователем, все остальные процессы блокируются до завершения
    ps.deactivate(); //деактивация станции
    wdt_disable(); //деактивация сторожевого таймера
    input_parameters();
    wdt_enable(WDTO_1S);
    ps.activate();
  }
}

void process_display_statement(char scroll_dir = '0'){
  static bool first_call = true;
  static bool manual_mode = false;
  static TimerMs page_change_prd(show_page_duration * 1000, 0, 0);
  static TimerMs message_show_tmr(show_statement_duration * 1000, 0, 1);
  static TimerMs manual_scroll_tmr(show_statement_duration * 1000, 0, 1);
  static uint8_t current_page = 0;

  if(!first_call && (!show_statement_noblock || message_show_tmr.tick()) && !ps.get_status_by_id(ID_GLOBAL_BLOCKING_STATUS)){
    first_call = true;
    show_statement_noblock = false;
    page_change_prd.stop();
    message_show_tmr.stop();
    lcd.noDisplay();
    lcd.noBacklight();
    return ;
  }

  if (show_statement_noblock || ps.get_status_by_id(ID_GLOBAL_BLOCKING_STATUS)){

    if (first_call){
      first_call = false;
      lcd.display();
      lcd.backlight();
      page_change_prd.setTime(show_page_duration * 1000);
      page_change_prd.start();
      message_show_tmr.setTime(show_statement_duration * 1000);
      message_show_tmr.start();
      current_page = 2;
      DEBUG(F("display has started"))
    }

    if(!manual_mode && scroll_dir != '0'){
      manual_mode = true;
      page_change_prd.stop();
      message_show_tmr.stop();
      manual_scroll_tmr.setTime(show_statement_duration * 1000);
      manual_scroll_tmr.start();
    } else if(manual_mode && manual_scroll_tmr.tick()){
        manual_mode = false;
        manual_scroll_tmr.stop();
        page_change_prd.start();
        message_show_tmr.resume();
    }

    if (page_change_prd.tick() || scroll_dir != '0'){   DEBUG(F("page change block"))

      if(scroll_dir =='L') {
        if(current_page > 0) current_page --;
          else current_page = 6;
      }

      if(scroll_dir == '0' || scroll_dir == 'R') {
        if(current_page < 6) current_page ++;
          else current_page = 0;
      }

      if(scroll_dir != '0' && manual_scroll_tmr.active()) manual_scroll_tmr.start();

      if(current_page == 3 && ps.get_mode()!= PRELAY_MODE) current_page++;
      if(current_page == 4 && ps.get_status_by_id(ID_PSENSOR_OK_STATUS)) current_page++;
      if(current_page == 5 && !ps.get_status_by_id(ID_GLOBAL_BLOCKING_STATUS)) current_page ++;
      if(current_page == 6 && !(time_until_deblock > 0) ) current_page = 0;
      if((current_page == 0 || current_page == 1) &&
        !ps.get_status_by_id(ID_PSENSOR_OK_STATUS)) current_page = 2;
      }

    render_page(current_page);
  }
}

void input_parameters(){
  //локальные переменные
  byte current_position = 0;
  byte max_position = 9; // на 1 меньше фактического количества пунктов
  bool refresh_current_position = true;

  lcd.display();
  lcd.backlight();
  lcd.clear();
  lcd.print(F("   YCTAHOBK"));
  lcd.write(byte(1)); //И
  delay(1500);

  while(true){
    if(refresh_current_position){ // вывод позиций, если нужно обновление
      lcd.clear();
      switch (current_position) {
        case 0:lcd.print(F("PE")); //0.	Режим \работы
          lcd.write(byte(0));
          lcd.write(byte(1));
          lcd.print(F("M"));
          lcd.setCursor(0, 1);
          lcd.print(F("PA"));
          lcd.write(byte(2));
          lcd.print(F("OT"));
          lcd.write(byte(3));
           break;
        case 1:lcd.print(F("KP")); //1.	Критическое \давление
          lcd.write(byte(1)); //И
          lcd.write('T');
          lcd.write(byte(1));//И
          lcd.print(F("4ECKOE"));
          lcd.setCursor(0, 1);
          lcd.print(F("DAB"));
          lcd.write(byte(4));//Л
          lcd.print(F("EH"));
          lcd.write(byte(1));//И
          lcd.write('E');
          break;
        case 2:lcd.print(F("MAKC")); //2.	МаксИмаЛЬное \давЛенИе
          lcd.write(byte(1)); //И
          lcd.print(F("MA"));
          lcd.write(byte(4));//Л
          lcd.write(byte(6));//Ь
          lcd.print(F("HOE"));
          lcd.setCursor(0, 1);
          lcd.print(F("DAB"));
          lcd.write(byte(4));//Л
          lcd.print(F("EH"));
          lcd.write(byte(1));//И
          lcd.write('E');
          break;
        case 3: lcd.write('M'); //3.	Минимальное \давление
          lcd.write(byte(1)); //И
          lcd.write('H');
          lcd.write(byte(1)); //И
          lcd.print(F("MA"));
          lcd.write(byte(4));//Л
          lcd.write(byte(6));//Ь
          lcd.print(F("HOE"));
          lcd.setCursor(0, 1);
          lcd.print(F("DAB"));
          lcd.write(byte(4));//Л
          lcd.print(F("EH"));
          lcd.write(byte(1));//И
          lcd.write('E');
          break;
        case 4: lcd.print(F("PA3MEP"));//4.	Размер \резервуара
          lcd.setCursor(0, 1);
          lcd.print(F("PE3EPBYAPA"));
          break;
        case 5: lcd.print(F("BPEM"));//5.	Время \разблокировки
          lcd.write(byte(7)); //Я
          lcd.setCursor(0, 1);
          lcd.print(F("PA3"));
          lcd.write(byte(2)); //Б
          lcd.write(byte(4));//Л
          lcd.print(F("OK"));
          lcd.write(byte(1)); //И
          lcd.print(F("POBK"));
          lcd.write(byte(1)); //И
          break;
        case 6:lcd.print(F("BPEM"));//	6.	Время показа \ состояния
          lcd.write(byte(7)); //Я
          lcd.write(' ');
          lcd.write(byte(5)); //П
          lcd.print(F("OKA3A"));
          lcd.setCursor(0, 1);
          lcd.print(F("COCTO"));
          lcd.write(byte(7)); //Я
          lcd.write('H');
          lcd.write(byte(1)); //И
          lcd.write(byte(7)); //Я
          break;
        case 7:lcd.print(F("BPEM"));//	7.	Время показа \ 1 стр.
          lcd.write(byte(7)); //Я
          lcd.write(' ');
          lcd.write(byte(5)); //П
          lcd.print(F("OKA3A"));
          lcd.setCursor(0, 1);
          lcd.print(F("1 CTP."));
          break;
        case 8:lcd.print(F("KOPPEKT"));//	8.	КОРРЕКТИРОВКА ДАВЛЕНИЯ
          lcd.write(byte(1)); //И
          lcd.print(F("POBKA"));
          lcd.setCursor(0, 1);lcd.print(F("DAB"));
          lcd.write(byte(4));//Л
          lcd.print(F("EH"));
          lcd.write(byte(1));//И
          lcd.write(byte(7)); //Я
          break;
        case 9:lcd.write('O'); // 0.	КОРРЕКТИРОВКА ДАВЛЕНИЯ
          lcd.write(byte(2)); //Б
          lcd.print(F(" A"));
          lcd.write(byte(5)); //П
          lcd.write(byte(5)); //П
          lcd.print(F("APATE"));
          break;

      }
      refresh_current_position = false;
    }

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
        case(0): ps.set_mode(InputF::input_mode(ps.get_mode()));
        if (ps.get_mode() != eeprom_read_byte(&operation_mode_addr)) eeprom_update_byte(&operation_mode_addr, ps.get_mode());
        break;

        case(1): ps.set_critical_pressure_bar(InputF::input_float(ps.get_critical_pressure_bar(), 0, 12));
        if(ps.get_critical_pressure_ADC() != eeprom_read_word(&critical_pressure_ADC_addr)) eeprom_update_word(&critical_pressure_ADC_addr, ps.get_critical_pressure_ADC());
        break;

        case(2): ps.set_max_pressure_bar(InputF::input_float(ps.get_max_pressure_bar(), ps.get_min_pressure_bar() + DELTA_P, 12));
        if(ps.get_max_pressure_ADC() != eeprom_read_word(&max_pressure_ADC_addr)) eeprom_update_word(&max_pressure_ADC_addr, ps.get_max_pressure_ADC());
        break;

        case(3): ps.set_min_pressure_bar(InputF::input_float(ps.get_min_pressure_bar(), 0, ps.get_max_pressure_bar() - DELTA_P));
        if(ps.get_min_pressure_ADC() != eeprom_read_word(&min_pressure_ADC_addr)) eeprom_update_word(&min_pressure_ADC_addr, ps.get_min_pressure_ADC());
        break;

        case(4): ps.set_tank_volume(InputF::input_float(ps.get_tank_volume(), 5, 500));
        if (abs(eeprom_read_float(&tank_volume_addr) - ps.get_tank_volume()) > 0.1) eeprom_update_float(&tank_volume_addr, ps.get_tank_volume());
        break;

        case(5): unblock_timeout_s = InputF::input_time_s(unblock_timeout_s, 0, 86400);
        if (unblock_timeout_s != eeprom_read_dword(&unblock_timeout_addr)) eeprom_update_dword(&unblock_timeout_addr, unblock_timeout_s);
        break;

        case(6): show_statement_duration = InputF::input_time_s(show_statement_duration, show_page_duration, 86400);
        if (show_statement_duration != eeprom_read_dword(&show_statement_duration_addr)) eeprom_update_dword(&show_statement_duration_addr, show_statement_duration);
        break;

        case(7): show_page_duration = InputF::input_time_s(show_page_duration, 1, show_statement_duration);
        if (show_page_duration != eeprom_read_dword(&show_page_duration_addr)) eeprom_update_dword(&show_page_duration_addr, show_page_duration);
        break;

        case(8): ps.set_press_corr_bar(InputF::input_float(ps.get_press_corr_bar(), 0, ps.get_max_pressure_bar() - ps.get_min_pressure_bar()));
        if(ps.get_press_corr_ADC() != eeprom_read_word(&press_corr_ADC_addr)) eeprom_update_word(&press_corr_ADC_addr, ps.get_press_corr_ADC());
        break;

        case 9: lcd.clear();
        lcd.print("BEPC");
        lcd.write(byte(1));//И
        lcd.write(byte(7)); //Я
        lcd.write(' ');
        lcd.write(byte(5)); //П
        lcd.write('O');
        lcd.setCursor(0,1);
        lcd.print(VERSION);
        while(1){
          if(button1.isRelease()) break;
        }
        break;

      }

      refresh_current_position = true;
    }

    if(button1.isHolded()) {
      if(!show_statement_noblock && !ps.get_status_by_id(ID_GLOBAL_BLOCKING_STATUS)){
        lcd.noDisplay();
        lcd.noBacklight();
      }
      return ;
    }


  }
}

void buttons_int(){
  button1.tick();
  encoder1.tick();
}

void render_page(uint8_t page_num){
  lcd.clear();

  switch(page_num){
    case 0:lcd.print(F("DAB"));
    lcd.write(byte(4));//Л
    lcd.print(F("EH"));
    lcd.write(byte(1));//И
    lcd.write('E');
    if (ps.get_status_by_id(ID_CRITICAL_PRESSURE_STATUS)){ // вывести предупреждение, если давление выше критического
      lcd.print(F(" (!KP"));
      lcd.write(byte(1));//И
      lcd.print(F("T)"));
    }
    lcd.setCursor(0,1);
    lcd.print(ps.get_pressure_bar(), 2);
    lcd.write(' ');
    lcd.write(byte(2)); //Б;
    lcd.print(F("AP"));
    break;

    case 1:lcd.print(F("3APAC BOD"));
    lcd.write(byte(3));//Ы
    lcd.setCursor(0,1);
    lcd.print(ps.get_water_reserveL());
    lcd.write(' ');
    lcd.write(byte(4));//Л
    lcd.print(" (");
    lcd.print(ps.get_water_reserve_percent());
    lcd.print(F("%)"));
    break;

    case 2:lcd.print(F("YPOBEH"));
    lcd.write(byte(6));//Ь
    lcd.print(F(" BOD"));
    lcd.write(byte(3));//Ы
    lcd.setCursor(0,1);
    if(ps.get_status_by_id(ID_FLOAT_STATUS)){
      lcd.print("B");
      lcd.write(byte(3));//Ы
      lcd.print(F("COK."));
    }
      else{
        lcd.write('H');
        lcd.write(byte(1));//И
        lcd.print(F("3K."));
      }
    break;

    case 3:lcd.print(F("PE"));
    lcd.write(byte(4));//Л
    lcd.print(F("E DAB"));
    lcd.write(byte(4));//Л
    lcd.print(F("EH"));
    lcd.write(byte(1));//И
    lcd.write(byte(7)); //Я
    lcd.setCursor(0,1);
    if(ps.get_status_by_id(ID_PRELAY_STATUS)){
      lcd.print(F("BK"));
    } else{
      lcd.print(F("OTK"));
    }
    lcd.write(byte(4));//Л
    lcd.write('.');
    break;

    case 4:lcd.print(F("DAT4"));
    lcd.write(byte(1));//И
    lcd.print("K");
    lcd.print(F(" DAB"));
    lcd.write(byte(4));//Л
    lcd.print(F("EH"));
    lcd.write(byte(1));//И
    lcd.write(byte(7)); //Я
    lcd.setCursor(0,1);
    lcd.print(F("HE "));
    lcd.write(byte(1));//И
    lcd.write('C');
    lcd.write(byte(5)); //П
    lcd.print(F("PABEH"));
    break;

    case 5:lcd.write(byte(2)); //Б
    lcd.write(byte(4));//Л
    lcd.print(F("OK"));
    lcd.write(byte(1)); //И
    lcd.print(F("POBKA"));
    if(!ps.is_global_blocking_conditions()){
      lcd.setCursor(0,1);
      lcd.print(F("(DOCTY"));
      lcd.write(byte(5)); //П
      lcd.print(F("HO OTK"));
      lcd.write(byte(4));//Л
      lcd.print(F(".)"));
    }
    break;

    case 6:lcd.print(F("DO PA3"));
    lcd.write(byte(2)); //Б
    lcd.write(byte(4));//Л
    lcd.print(F("OK"));
    lcd.write(byte(1)); //И
    lcd.print(F("POBK"));
    lcd.write(byte(1)); //И
    lcd.setCursor(0,1);
    lcd.print(F("    "));
    uint16_t tmp = time_until_deblock / 3600; // часы
    if(tmp < 10) lcd.write('0');
    lcd.print(tmp);
    lcd.write(':');
    tmp = time_until_deblock % 3600 / 60; // минуты
    if(tmp < 10) lcd.write('0');
    lcd.print(tmp);
    lcd.write(':');
    tmp = time_until_deblock % 3600 % 60; //секунды
    if(tmp < 10) lcd.write('0');
    lcd.print(tmp);
    break;
}
}
