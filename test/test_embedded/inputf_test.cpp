#include <Arduino.h>
#include <InputF.h>
#include <unity.h>
#include <GyverButton.h>
#include <custom_functions.h>
#include <GyverEncoder.h>
#include "LiquidCrystal_I2C.h"
#include "TimerOne.h"

#define P_ENCODER_SW 5  //кнопка энкодера
#define P_ENCODER_DT 4 // DT пин энкодера
#define P_ENCODER_CLK 3 //CLK пин энкодера
#define P_BUTTON1 2 //пин кнопки 1

LiquidCrystal_I2C lcd(0x27,16,2);
GButton button1 (P_BUTTON1);
Encoder encoder1 (P_ENCODER_CLK, P_ENCODER_DT, P_ENCODER_SW, TYPE2);

void switch_test(){
  delay(2000);
  while(1){
    if(encoder1.isRelease()) break;
  }
}

void test_function_input_bool(void) {
  Serial.println("Vvedi Da");
    TEST_ASSERT_EQUAL(true, InputF::input_boolean(false));

}

void test_function_input_time(void) {
  byte h =12;
  byte m =35;
  byte s =56;
  Serial.print("Vvedi ");
  Serial.print(h);
  Serial.print(" : ");
  Serial.print(m);
  Serial.print(" : ");
  Serial.print(s);
  Serial.println(" ");
    TEST_ASSERT_EQUAL((uint32_t)h*3600+(uint32_t)m*60+(uint32_t)s, InputF::input_time_s(15,0, 86000));

}

void test_function_input_bool_2(void)
{
  Serial.println("Vvedi РЕЛЕ");
  TEST_ASSERT_EQUAL(0, InputF::input_mode(1));
}

void test_function_input_float(void){
  Serial.println("Vvedi 7.9 [1 -12]" );
  TEST_ASSERT_FLOAT_WITHIN(0.1, 7.9, InputF::input_float(3, 1, 12));
}

void test_function_input_int(void){
  Serial.println("Vvedi 1278 [78-3000]");
  TEST_ASSERT_EQUAL(1278, InputF::input_int(10, 78, 3000));
}

void test_function_select(void){
  Serial.println("Vvedi Index 7");
  char *list[] = {"Index 0",
                  "Index 1",
                  "Index 2",
                  "Index 3",
                  "Index 4",
                  "Index 5",
                  "Index 6",
                  "Index 7",
                  "Index 8",
                  "Index 9"};
  TEST_ASSERT_EQUAL(7, InputF::select_list_element(list, 2, 10));
}

void buttons_int(){
  button1.tick();
  encoder1.tick();
}

void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);
    InputF::attach_button(button1);
    InputF::attach_encoder(encoder1);
    InputF::attach_lcd_i2c(lcd);

    lcd.init();
    lcd.backlight();

    Timer1.initialize(5000);
    Timer1.attachInterrupt(buttons_int);

    Serial.begin(9600);
    create_custom_chars(lcd);


    UNITY_BEGIN();
     RUN_TEST(test_function_input_bool);
      //

      RUN_TEST(test_function_input_bool_2);


      RUN_TEST(test_function_input_float);


      RUN_TEST(test_function_input_int);


      RUN_TEST(test_function_select);


      RUN_TEST(test_function_input_time);
      //
    UNITY_END();
    lcd.home();
    for (byte i=0;i<8;i++){
      lcd.write(i);
    }
}

void loop() {
    if(button1.isRelease()){
      Serial.println("Release");
    }

    if(button1.isHolded()){
      Serial.println("Holded");
    }
}
