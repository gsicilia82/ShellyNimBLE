
#ifndef ADE7953_h
#define ADE7953_h

#include "Arduino.h"
#include <Wire.h>

struct ENERGY {
  float voltage[2];             // 123.1 V
  float current[2];             // 123.123 A
  float active_power[2];        // 123.1 W
  float frequency[2];           // 123.1 Hz

  uint8_t phase_count;                          // Number of phases active

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
