#include <inputf.h>

template <typename T>
  T InputF::_input_number(T default_value, T min_value, T max_value, T step, T b_step){
    T current_value = default_value;
    if (default_value < min_value) current_value = min_value;
    if (default_value > max_value) current_value = max_value;

    bool value_changed=true;

    while(true){

      if (_encoder->isRight()){
        if(current_value < max_value) {
          current_value += step;
          if (current_value > max_value) current_value = max_value;
          value_changed = true;
        }
      }

      if (_encoder->isLeft()){
        if(current_value > min_value) {
          current_value -= step;
          if (current_value < min_value) current_value = min_value;
          value_changed = true;
        }
      }

      if (_encoder->isRightH()){
        if(current_value < max_value - b_step) {
          current_value += b_step;
          value_changed = true;
        }
      }

      if (_encoder->isLeftH()){
        if(current_value > min_value + b_step) {
          current_value -= b_step;
          value_changed = true;
        }
      }

      if(value_changed){
        _lcd->clear();
        _lcd->print(current_value);
        value_changed = false;
      }

       switch(_return_result()){
         case'D': return default_value; break;
         case'C': return current_value; break;
       }

    }
  }

  GButton* InputF::_button = NULL;
  Encoder* InputF::_encoder = NULL;
  LiquidCrystal_I2C* InputF::_lcd = NULL;

 void InputF::attach_lcd_i2c(LiquidCrystal_I2C &lcd){
   _lcd = &lcd;
 }


 void InputF::attach_encoder(Encoder &encoder){
   _encoder = &encoder;
 }

 void InputF::attach_button(GButton &button){
   _button = &button;
 }

 bool InputF::input_boolean(bool default_value){
   bool current_value = default_value;

   _lcd->clear();
   _lcd->setCursor(1, 0);
   _lcd->print(F("DA"));
   _lcd->setCursor(1, 1);
   _lcd->print(F("HET"));
   if (current_value) _lcd->setCursor(0, 0);
     else _lcd->setCursor(0, 1);
   _lcd->print(">");

   while(true){

     if (_encoder->isLeft() || _encoder->isRight()){
       current_value =!current_value;

       for(byte i = 0; i<2; i++){
         _lcd->setCursor(0,i);
         _lcd->print(" ");
       }

       if(current_value) _lcd->setCursor(0,0);
        else _lcd->setCursor(0,1);
       _lcd->print(">");
     }

      switch(_return_result()){
        case'D': return default_value; break;
        case'C': return current_value; break;
      }

   }
 }

 uint8_t InputF::input_mode(uint8_t default_value){
   bool current_value = (bool)default_value;

   _lcd->clear();
   _lcd->setCursor(1, 0);
   _lcd->print(F("DAT4"));
   _lcd->write(byte(1));
   _lcd->print(F("K DAB"));
   _lcd->write(byte(4));
   _lcd->print(F("EH"));
   _lcd->write(byte(1));
   _lcd->write(byte(7));

   _lcd->setCursor(1, 1);
   _lcd->print(F("PE"));
   _lcd->write(byte(4));
   _lcd->print(F("E DAB"));
   _lcd->write(byte(4));
   _lcd->print(F("EH"));
   _lcd->write(byte(1));
   _lcd->write(byte(7));

   if (current_value) _lcd->setCursor(0, 0);
     else _lcd->setCursor(0, 1);
   _lcd->print(">");

   while(true){

     if (_encoder->isLeft() || _encoder->isRight()){
       current_value =!current_value;

       for(byte i = 0; i<2; i++){
         _lcd->setCursor(0,i);
         _lcd->print(" ");
       }

       if(current_value) _lcd->setCursor(0,0);
        else _lcd->setCursor(0,1);
       _lcd->print(">");
     }

      switch(_return_result()){
        case'D': return default_value; break;
        case'C': return current_value; break;
      }
   }
 }

 float InputF::input_float(float default_value, float min_value, float max_value){
   return _input_number<float>(default_value, min_value, max_value, 0.1f, 1.0f);
 }

 int InputF::input_int(int default_value, int min_value, int max_value){
   return _input_number(default_value, min_value, max_value, 1, 10);
 }

 byte InputF::select_list_element(char *array[], byte default_index, byte arr_size){
   byte current_index = default_index;
   bool is_index_changed = true;

   while(1){

     if(is_index_changed){
       _lcd->clear();
       _lcd->setCursor(0, 0);
       _lcd->print(">");
       _lcd->print(array[current_index]);
       byte next_index = (current_index == arr_size - 1)? 0:current_index + 1;
       _lcd->setCursor(1, 1);
       _lcd->print(array[next_index]);
       is_index_changed = false;
     }

     if (_encoder->isLeft()){
       current_index = (current_index == 0)?arr_size - 1:current_index - 1;
       is_index_changed = true;
     }

     if (_encoder->isRight()){
       current_index = (current_index == arr_size - 1)?0:current_index + 1;
       is_index_changed = true;
     }

      switch(_return_result()){
        case'D': return default_index; break;
        case'C': return current_index; break;
      }

   }
 }

 uint32_t InputF::input_time_s(uint32_t default_value, uint32_t min_value, uint32_t max_value){

   uint8_t cur_pos = 2;

   uint32_t cur_time = default_value;
   if (default_value < min_value) cur_time = min_value;
   if (default_value > max_value) cur_time = max_value;

   uint8_t h, m, s;
   bool update_display = true;

   while(1){
     if(update_display){
       h = cur_time / 3600;
       m = cur_time % 3600 / 60;
       s = cur_time % 3600 % 60;

       _lcd->clear();
       _lcd->setCursor(3,0);
       _lcd->write((cur_pos == 0)?'>':' ');
       if(h < 10) _lcd->write('0');
       _lcd->print(h);
       _lcd->write((cur_pos == 1)?'>':' ');
       if(m < 10) _lcd->write('0');
       _lcd->print(m);
       _lcd->write((cur_pos == 2)?'>':' ');
       if(s < 10) _lcd->write('0');
       _lcd->print(s);
       update_display = false;
     }

     if(_encoder->isRelease()){
       cur_pos = (cur_pos == 2) ? 0 : cur_pos + 1;
       update_display = true;
     }

     if(_encoder->isLeft()){
       switch(cur_pos){
         case 0:
           cur_time = ((min_value + 3600) < cur_time)? cur_time - 3600 : min_value;
           break;
         case 1:
           cur_time = ((min_value + 60) < cur_time)? cur_time - 60 : min_value;
           break;
         case 2:
           cur_time = ((min_value + 1) < cur_time)? cur_time - 1 : min_value;
           break;
       }
       update_display = true;
     }

     if(_encoder->isRight()){
       switch(cur_pos){
         case 0:
           cur_time = ((cur_time + 3600) < max_value)? cur_time + 3600 : max_value;
           break;
         case 1:
            cur_time = ((cur_time + 60) < max_value)? cur_time + 60 : max_value;
           break;
         case 2:
             cur_time = ((cur_time + 1) < max_value)? cur_time + 1 : max_value;
           break;
       }
       update_display = true;
     }

    switch(_return_result()){
      case'D': return default_value; break;
      case'C': return cur_time; break;
    }

   }

 }

 void InputF::_show_message(const __FlashStringHelper* mes, uint16_t a_delay=1000){
   _lcd->clear();
   _lcd->print(mes);
   delay(a_delay);
   _button->resetStates();
 }

 char InputF::_return_result(){
   if(_button->isRelease()) {
     _show_message(F("OTMEHA"), 1000);
     return 'D';
   }

   if(_button->isHolded()){
     _show_message(F("COXPAHEHO"), 1000);
     return 'C';
   }

   return 'N';
}
