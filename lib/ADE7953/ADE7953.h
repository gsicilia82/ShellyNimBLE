
#ifndef ADE7953_h
#define ADE7953_h

#include "Arduino.h"
#include <Wire.h>

struct ENERGY {
  float voltage[2] = {0,0}; // 123.1 V
  float current[2] = {0,0}; // 123.123 A
  float power[2]   = {0,0}; // 123.1 W
  float powerAcc   = 0;     // = power[0] + power[1]
};

class ADE7953 {
  public:
    ADE7953();
    void initialize( int SCL, int SDA);
    ENERGY getData();
  
  private:
  	int _SCL;
	int _SDA;
};

#endif
