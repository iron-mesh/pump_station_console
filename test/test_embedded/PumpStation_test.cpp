#include <Arduino.h>
#include <unity.h>

#include <PumpStation.h>

#define P_ENCODER_SW 5  //кнопка энкодера
#define P_ENCODER_DT 4 // DT пин энкодера
#define P_ENCODER_CLK 3 //CLK пин энкодера
#define P_BUTTON1 2 //пин кнопки 1

#define P_RELAY_SWITCHER 10 //пин управления реле насоса
#define P_PRESSURE_RELAY_IN 11 //пин контроля реле давления
#define P_FLOAT_IN 12  // пин котроля за поплавком уровня воды
#define P_PRESSURE_SENSOR 1  //аналоговый пин измерения напряжения на датчике давления

PumpStation ps(P_PRESSURE_RELAY_IN, P_FLOAT_IN, P_PRESSURE_SENSOR, P_RELAY_SWITCHER);

void test_mode(/* arguments */) {
  /* code */
  ps.set_mode(SENSOR_MODE);
  TEST_ASSERT_EQUAL(SENSOR_MODE, ps.get_mode());

}

void setup() {

    Serial.begin(9600);


    UNITY_BEGIN();
     RUN_TEST(test_mode);
      //
      //
    UNITY_END();

    }


void loop() {

}
