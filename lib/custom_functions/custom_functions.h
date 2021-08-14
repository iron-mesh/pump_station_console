#ifndef CUSTOM_FUNCTIONS_H
#define CUSTOM_FUNCTIONS_H

float pressureADC_to_float(uint16_t a){
  return (a - 102) * 0.01481;
}

uint16_t float_to_pressureADC(float a){
  return round(a * 67.5 + 102);
}

#endif
