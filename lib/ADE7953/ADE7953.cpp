/*
  This script is based on Tasmota ADE7953 implementation and was modified to get it work as simple as possible.
  https://github.com/arendst/Tasmota
*/

#include "Arduino.h"
#include <Wire.h>
#include <ADE7953.h>

#define ADE7953_ADDR              0x38

/*********************************************************************************************/

#define ADE7953_PREF              1540       // 4194304 / (1540 / 1000) = 2723574 (= WGAIN, VAGAIN and VARGAIN)
#define ADE7953_UREF              26000      // 4194304 / (26000 / 10000) = 1613194 (= VGAIN)
#define ADE7953_IREF              10000      // 4194304 / (10000 / 10000) = 4194303 (= IGAIN, needs to be different than 4194304 in order to use calib.dat)

// Default calibration parameters can be overridden by a rule as documented above.
#define ADE7953_GAIN_DEFAULT      4194304    // = 0x400000 range 2097152 (min) to 6291456 (max)
#define ADE7953_PHCAL_DEFAULT     0          // = range -383 to 383 - Default phase calibration for Shunts
#define ADE7953_PHCAL_DEFAULT_CT  200        // = range -383 to 383 - Default phase calibration for Current Transformers (Shelly EM)

enum Ade7953_8BitRegisters {
  // Register Name                    Addres  R/W  Bt  Ty  Default     Description
  // ----------------------------     ------  ---  --  --  ----------  --------------------------------------------------------------------
  ADE7953_SAGCYC = 0x000,          // 0x000   R/W  8   U   0x00        Sag line cycles
  ADE7953_DISNOLOAD,               // 0x001   R/W  8   U   0x00        No-load detection disable (see Table 16)
  ADE7953_RESERVED_0X002,          // 0x002
  ADE7953_RESERVED_0X003,          // 0x003
  ADE7953_LCYCMODE,                // 0x004   R/W  8   U   0x40        Line cycle accumulation mode configuration (see Table 17)
  ADE7953_RESERVED_0X005,          // 0x005
  ADE7953_RESERVED_0X006,          // 0x006
  ADE7953_PGA_V,                   // 0x007   R/W  8   U   0x00        Voltage channel gain configuration (Bits[2:0])
  ADE7953_PGA_IA,                  // 0x008   R/W  8   U   0x00        Current Channel A gain configuration (Bits[2:0])
  ADE7953_PGA_IB                   // 0x009   R/W  8   U   0x00        Current Channel B gain configuration (Bits[2:0])
};

enum Ade7953_16BitRegisters {
  // Register Name                    Addres  R/W  Bt  Ty  Default     Description
  // ----------------------------     ------  ---  --  --  ----------  --------------------------------------------------------------------
  ADE7953_ZXTOUT = 0x100,          // 0x100   R/W  16  U   0xFFFF      Zero-crossing timeout
  ADE7953_LINECYC,                 // 0x101   R/W  16  U   0x0000      Number of half line cycles for line cycle energy accumulation mode
  ADE7953_CONFIG,                  // 0x102   R/W  16  U   0x8004      Configuration register (see Table 18)
  ADE7953_CF1DEN,                  // 0x103   R/W  16  U   0x003F      CF1 frequency divider denominator. When modifying this register, two sequential write operations must be performed to ensure that the write is successful.
  ADE7953_CF2DEN,                  // 0x104   R/W  16  U   0x003F      CF2 frequency divider denominator. When modifying this register, two sequential write operations must be performed to ensure that the write is successful.
  ADE7953_RESERVED_0X105,          // 0x105
  ADE7953_RESERVED_0X106,          // 0x106
  ADE7953_CFMODE,                  // 0x107   R/W  16  U   0x0300      CF output selection (see Table 19)
  ADE7943_PHCALA,                  // 0x108   R/W  16  S   0x0000      Phase calibration register (Current Channel A). This register is in sign magnitude format.
  ADE7943_PHCALB,                  // 0x109   R/W  16  S   0x0000      Phase calibration register (Current Channel B). This register is in sign magnitude format.
  ADE7943_PFA,                     // 0x10A   R    16  S   0x0000      Power factor (Current Channel A)
  ADE7943_PFB,                     // 0x10B   R    16  S   0x0000      Power factor (Current Channel B)
  ADE7943_ANGLE_A,                 // 0x10C   R    16  S   0x0000      Angle between the voltage input and the Current Channel A input
  ADE7943_ANGLE_B,                 // 0x10D   R    16  S   0x0000      Angle between the voltage input and the Current Channel B input
  ADE7943_Period                   // 0x10E   R    16  U   0x0000      Period register
};

enum Ade7953_32BitRegisters {
  // Register Name                    Addres  R/W  Bt  Ty  Default     Description
  // ----------------------------     ------  ---  --  --  ----------  --------------------------------------------------------------------
  ADE7953_ACCMODE = 0x301,         // 0x301   R/W  24  U   0x000000    Accumulation mode (see Table 21)

  ADE7953_AVA = 0x310,             // 0x310   R    24  S   0x000000    Instantaneous apparent power (Current Channel A)
  ADE7953_BVA,                     // 0x311   R    24  S   0x000000    Instantaneous apparent power (Current Channel B)
  ADE7953_AWATT,                   // 0x312   R    24  S   0x000000    Instantaneous active power (Current Channel A)
  ADE7953_BWATT,                   // 0x313   R    24  S   0x000000    Instantaneous active power (Current Channel B)
  ADE7953_AVAR,                    // 0x314   R    24  S   0x000000    Instantaneous reactive power (Current Channel A)
  ADE7953_BVAR,                    // 0x315   R    24  S   0x000000    Instantaneous reactive power (Current Channel B)
  ADE7953_IA,                      // 0x316   R    24  S   0x000000    Instantaneous current (Current Channel A)
  ADE7953_IB,                      // 0x317   R    24  S   0x000000    Instantaneous current (Current Channel B)
  ADE7953_V,                       // 0x318   R    24  S   0x000000    Instantaneous voltage (voltage channel)
  ADE7953_RESERVED_0X319,          // 0x319
  ADE7953_IRMSA,                   // 0x31A   R    24  U   0x000000    IRMS register (Current Channel A)
  ADE7953_IRMSB,                   // 0x31B   R    24  U   0x000000    IRMS register (Current Channel B)
  ADE7953_VRMS,                    // 0x31C   R    24  U   0x000000    VRMS register
  ADE7953_RESERVED_0X31D,          // 0x31D
  ADE7953_AENERGYA,                // 0x31E   R    24  S   0x000000    Active energy (Current Channel A)
  ADE7953_AENERGYB,                // 0x31F   R    24  S   0x000000    Active energy (Current Channel B)
  ADE7953_RENERGYA,                // 0x320   R    24  S   0x000000    Reactive energy (Current Channel A)
  ADE7953_RENERGYB,                // 0x321   R    24  S   0x000000    Reactive energy (Current Channel B)
  ADE7953_APENERGYA,               // 0x322   R    24  S   0x000000    Apparent energy (Current Channel A)
  ADE7953_APENERGYB,               // 0x323   R    24  S   0x000000    Apparent energy (Current Channel B)
  ADE7953_OVLVL,                   // 0x324   R/W  24  U   0xFFFFFF    Overvoltage level
  ADE7953_OILVL,                   // 0x325   R/W  24  U   0xFFFFFF    Overcurrent level
  ADE7953_VPEAK,                   // 0x326   R    24  U   0x000000    Voltage channel peak
  ADE7953_RSTVPEAK,                // 0x327   R    24  U   0x000000    Read voltage peak with reset
  ADE7953_IAPEAK,                  // 0x328   R    24  U   0x000000    Current Channel A peak
  ADE7953_RSTIAPEAK,               // 0x329   R    24  U   0x000000    Read Current Channel A peak with reset
  ADE7953_IBPEAK,                  // 0x32A   R    24  U   0x000000    Current Channel B peak
  ADE7953_RSTIBPEAK,               // 0x32B   R    24  U   0x000000    Read Current Channel B peak with reset
  ADE7953_IRQENA,                  // 0x32C   R/W  24  U   0x100000    Interrupt enable (Current Channel A, see Table 22)
  ADE7953_IRQSTATA,                // 0x32D   R    24  U   0x000000    Interrupt status (Current Channel A, see Table 23)
  ADE7953_RSTIRQSTATA,             // 0x32E   R    24  U   0x000000    Reset interrupt status (Current Channel A)
  ADE7953_IRQENB,                  // 0x32F   R/W  24  U   0x000000    Interrupt enable (Current Channel B, see Table 24)
  ADE7953_IRQSTATB,                // 0x330   R    24  U   0x000000    Interrupt status (Current Channel B, see Table 25)
  ADE7953_RSTIRQSTATB,             // 0x331   R    24  U   0x000000    Reset interrupt status (Current Channel B)

  ADE7953_CRC = 0x37F,             // 0x37F   R    32  U   0xFFFFFFFF  Checksum
  ADE7953_AIGAIN,                  // 0x380   R/W  24  U   0x400000    Current channel gain (Current Channel A)
  ADE7953_AVGAIN,                  // 0x381   R/W  24  U   0x400000    Voltage channel gain
  ADE7953_AWGAIN,                  // 0x382   R/W  24  U   0x400000    Active power gain (Current Channel A)
  ADE7953_AVARGAIN,                // 0x383   R/W  24  U   0x400000    Reactive power gain (Current Channel A)
  ADE7953_AVAGAIN,                 // 0x384   R/W  24  U   0x400000    Apparent power gain (Current Channel A)
  ADE7953_RESERVED_0X385,          // 0x385
  ADE7953_AIRMSOS,                 // 0x386   R/W  24  S   0x000000    IRMS offset (Current Channel A)
  ADE7953_RESERVED_0X387,          // 0x387
  ADE7953_VRMSOS,                  // 0x388   R/W  24  S   0x000000    VRMS offset
  ADE7953_AWATTOS,                 // 0x389   R/W  24  S   0x000000    Active power offset correction (Current Channel A)
  ADE7953_AVAROS,                  // 0x38A   R/W  24  S   0x000000    Reactive power offset correction (Current Channel A)
  ADE7953_AVAOS,                   // 0x38B   R/W  24  S   0x000000    Apparent power offset correction (Current Channel A)
  ADE7953_BIGAIN,                  // 0x38C   R/W  24  U   0x400000    Current channel gain (Current Channel B)
  ADE7953_BVGAIN,                  // 0x38D   R/W  24  U   0x400000    This register should not be modified.
  ADE7953_BWGAIN,                  // 0x38E   R/W  24  U   0x400000    Active power gain (Current Channel B)
  ADE7953_BVARGAIN,                // 0x38F   R/W  24  U   0x400000    Reactive power gain (Current Channel B)
  ADE7953_BVAGAIN,                 // 0x390   R/W  24  U   0x400000    Apparent power gain (Current Channel B)
  ADE7953_RESERVED_0X391,          // 0x391
  ADE7953_BIRMSOS,                 // 0x392   R/W  24  S   0x000000    IRMS offset (Current Channel B)
  ADE7953_RESERVED_0X393,          // 0x393
  ADE7953_RESERVED_0X394,          // 0x394
  ADE7953_BWATTOS,                 // 0x395   R/W  24  S   0x000000    Active power offset correction (Current Channel B)
  ADE7953_BVAROS,                  // 0x396   R/W  24  S   0x000000    Reactive power offset correction (Current Channel B)
  ADE7953_BVAOS                    // 0x397   R/W  24  S   0x000000    Apparent power offset correction (Current Channel B)
};

enum Ade7953CalibrationRegisters {
  ADE7953_CAL_VGAIN,
  ADE7953_CAL_IGAIN,
  ADE7953_CAL_WGAIN,
  ADE7953_CAL_VAGAIN,
  ADE7953_CAL_VARGAIN,
  ADE7943_CAL_PHCAL
};

ENERGY Energy;

const uint8_t  ADE7953_CALIBREGS = 6;
const uint16_t Ade7953CalibRegs[2][ADE7953_CALIBREGS] {
  { ADE7953_AVGAIN, ADE7953_AIGAIN, ADE7953_AWGAIN, ADE7953_AVAGAIN, ADE7953_AVARGAIN, ADE7943_PHCALA },
  { ADE7953_BVGAIN, ADE7953_BIGAIN, ADE7953_BWGAIN, ADE7953_BVAGAIN, ADE7953_BVARGAIN, ADE7943_PHCALB }
};

const uint8_t  ADE7953_REGISTERS = 6;
const uint16_t Ade7953Registers[2][ADE7953_REGISTERS] {
  { ADE7953_IRMSA, ADE7953_AENERGYA, ADE7953_APENERGYA, ADE7953_RENERGYA, ADE7953_VRMS, ADE7943_Period },
  { ADE7953_IRMSB, ADE7953_AENERGYB, ADE7953_APENERGYB, ADE7953_RENERGYB, ADE7953_VRMS, ADE7943_Period }

};


const float ADE7953_LSB_PER_WATTSECOND = 2.5;
const float ADE7953_POWER_CORRECTION = 23.41494;  // See https://github.com/arendst/Tasmota/pull/16941

enum EnergyCalibration { ENERGY_POWER_CALIBRATION, ENERGY_VOLTAGE_CALIBRATION, ENERGY_CURRENT_CALIBRATION, ENERGY_FREQUENCY_CALIBRATION };

struct Ade7953 {
  uint32_t voltage_rms[2] = { 0, 0 };
  uint32_t current_rms[2] = { 0, 0 };
  uint32_t active_power[2] = { 0, 0 };
  int32_t calib_data[2][ADE7953_CALIBREGS];
  uint8_t init_step = 0;
  uint8_t model = 0;             // 0 = Shelly 2.5, 1 = Shelly EM, 2 = Shelly Plus 2PM, 3 = Shelly Pro 1PM, 4 = Shelly Pro 2PM
  uint8_t cs_index;
} Ade7953;

int Ade7953RegSize(uint16_t reg) {
  int size = 0;
  switch ((reg >> 8) & 0x0F) {
    case 0x03:  // 32-bit
      size++;
    case 0x02:  // 24-bit
      size++;
    case 0x01:  // 16-bit
      size++;
    case 0x00:  // 8-bit
    case 0x07:
    case 0x08:
      size++;
  }
  return size;
}

void Ade7953Write(uint16_t reg, uint32_t val) {
  int size = Ade7953RegSize(reg);
  if (size) {

//      AddLog(LOG_LEVEL_DEBUG, PSTR("DBG: Write %08X"), val);

      Wire.beginTransmission(ADE7953_ADDR);
      Wire.write((reg >> 8) & 0xFF);
      Wire.write(reg & 0xFF);
      while (size--) {
        Wire.write((val >> (8 * size)) & 0xFF);      // Write data, MSB first
      }
      Wire.endTransmission();
      delayMicroseconds(5);                          // Bus-free time minimum 4.7us
  }
}

int32_t Ade7953Read(uint16_t reg) {
  uint32_t response = 0;

  int size = Ade7953RegSize(reg);
  if (size) {
      Wire.beginTransmission(ADE7953_ADDR);
      Wire.write((reg >> 8) & 0xFF);
      Wire.write(reg & 0xFF);
      Wire.endTransmission(0);
      Wire.requestFrom(ADE7953_ADDR, size);
      if (size <= Wire.available()) {
        for (uint32_t i = 0; i < size; i++) {
          response = response << 8 | Wire.read();    // receive DATA (MSB first)
        }
      }
  }
  return response;
}

uint32_t EnergyGetCalibration(uint32_t chan, uint32_t cal_type) {
  uint32_t channel = ((1 == chan) && (2 == Energy.phase_count)) ? 1 : 0;
  if (channel) {
    switch (cal_type) {
      case ENERGY_POWER_CALIBRATION: return ADE7953_PREF;
      case ENERGY_VOLTAGE_CALIBRATION: return ADE7953_UREF;
      case ENERGY_CURRENT_CALIBRATION: return ADE7953_IREF;
    }
  } else {
    switch (cal_type) {
      case ENERGY_POWER_CALIBRATION: return ADE7953_PREF;
      case ENERGY_VOLTAGE_CALIBRATION: return ADE7953_UREF;
      case ENERGY_CURRENT_CALIBRATION: return ADE7953_IREF;
    }
  }
  return 0;
}

void Ade7953SetCalibration(uint32_t regset, uint32_t calibset) {
  Ade7953.cs_index = calibset;
  for (uint32_t i = 0; i < ADE7953_CALIBREGS; i++) {
    int32_t value = Ade7953.calib_data[calibset][i];
    if (ADE7943_CAL_PHCAL == i) {
      if (value < 0) {
        value = abs(value) + 0x200;                  // Add sign magnitude
      }
    }
    Ade7953Write(Ade7953CalibRegs[regset][i], value);
  }
}



void Ade7953Defaults() {
  for (uint32_t channel = 0; channel < 2; channel++) {
    for (uint32_t i = 0; i < ADE7953_CALIBREGS; i++) {
      if (ADE7943_CAL_PHCAL == i) {
        Ade7953.calib_data[channel][i] = ADE7953_PHCAL_DEFAULT;
      } else {
        Ade7953.calib_data[channel][i] = ADE7953_GAIN_DEFAULT;
      }
    }
  }

  // {"angles":{"angle0":180,"angle1":176}}
  // {"rms":{"current_a": 4194303,"current_b":4194303,"voltage":  1613194},                    "angles":{"angle0":0,"angle1":0},"powers":{"totactive":{"a":2723574,"b":2723574},"apparent":{"a":2723574,"b":2723574},"reactive":{"a":2723574,"b":2723574}}}
  // {"rms":{"current_a":21865738,"current_b":1558533,"voltage_a":1599149,"voltage_b":1597289},"angles":{"angle0":0,"angle1":0},"powers":{"totactive":{"a":106692616,"b":3540894}}}


  // voltage
  Ade7953.calib_data[0][ADE7953_CAL_VGAIN] = 1613194;
  Ade7953.calib_data[1][ADE7953_CAL_VGAIN] = 1613194;

    // current
  Ade7953.calib_data[0][ADE7953_CAL_IGAIN] = 4194303;
  Ade7953.calib_data[1][ADE7953_CAL_IGAIN] = 4194303;

    // angles
  Ade7953.calib_data[0][ADE7943_CAL_PHCAL] = 0;
  Ade7953.calib_data[1][ADE7943_CAL_PHCAL] = 0;
  
  // power totactive
  Ade7953.calib_data[0][ADE7953_CAL_WGAIN] = 2723574;
  Ade7953.calib_data[1][ADE7953_CAL_WGAIN] = 2723574;

  // power apparent
  Ade7953.calib_data[0][ADE7953_CAL_VAGAIN] = 2723574;
  Ade7953.calib_data[1][ADE7953_CAL_VAGAIN] = 2723574;

  // power reactive
  Ade7953.calib_data[0][ADE7953_CAL_VARGAIN] = 2723574;
  Ade7953.calib_data[1][ADE7953_CAL_VARGAIN] = 2723574;
}


void Ade7953DrvInit() {
    uint32_t pin_irq = 27;
    pinMode(pin_irq, INPUT);                         // Related to resetPins() - Must be set to input

    int pin_reset = 4;                               // -1 if not defined

    if (pin_reset >= 0) {
      digitalWrite(pin_reset, 0);
      pinMode(pin_reset, OUTPUT);                    // Reset pin ADE7953
      delay(1);                                      // To initiate a hardware reset, this pin must be brought low for a minimum of 10 μs.
      digitalWrite(pin_reset, 1);
      pinMode(pin_reset, INPUT);
    }
    delay(100);                                      // Need 100mS to init ADE7953

    Ade7953Defaults();

}

//****************Object Definition*****************
ADE7953::ADE7953(){}
//**************************************************

void ADE7953::initialize( int SCL, int SDA) {

  _SCL=SCL;
  _SDA=SDA;

  Wire.begin( _SDA, _SCL);

  Ade7953DrvInit();

  uint32_t chips = 1;
  for (uint32_t chip = 0; chip < chips; chip++) {
    Ade7953.cs_index = chip;

    Ade7953Write(ADE7953_CONFIG, 0x0004);            // Locking the communication interface (Clear bit COMM_LOCK), Enable HPF
    Ade7953Write(0x0FE, 0x00AD);                     // Unlock register 0x120
    Ade7953Write(0x120, 0x0030);                     // Configure optimum setting
  }

  Ade7953SetCalibration(0, 0);                       // First ADE7953 A registers set with calibration set 0
  Ade7953SetCalibration(1, 1);                       // First ADE7953 B register set with calibration set 1

  int32_t regs[ADE7953_CALIBREGS];
  for (uint32_t chip = 0; chip < chips; chip++) {
    Ade7953.cs_index = chip;
    for (uint32_t channel = 0; channel < 2; channel++) {
      for (uint32_t i = 0; i < ADE7953_CALIBREGS; i++) {
        regs[i] = Ade7953Read(Ade7953CalibRegs[channel][i]);
        if (ADE7943_CAL_PHCAL == i) {
          if (regs[i] >= 0x0200) {
            regs[i] &= 0x01FF;                       // Clear sign magnitude
            regs[i] *= -1;                           // Make negative
          }
        }
      }
    }
  }
}

ENERGY ADE7953::getData() {
  uint32_t acc_mode = 0;
  int32_t reg[2][ADE7953_REGISTERS];

  for (uint32_t channel = 0; channel < 2; channel++) {
    uint32_t channel_swap = channel;
    for (uint32_t i = 0; i < ADE7953_REGISTERS; i++) {
      reg[channel_swap][i] = Ade7953Read(Ade7953Registers[channel][i]);
    }
  }
  acc_mode = Ade7953Read(ADE7953_ACCMODE);         // Accumulation mode

  uint32_t apparent_power[2] = { 0, 0 };
  uint32_t reactive_power[2] = { 0, 0 };

  for (uint32_t channel = 0; channel < 2; channel++) {
    Ade7953.voltage_rms[channel] = reg[channel][4];
    Ade7953.current_rms[channel] = reg[channel][0];
    if (Ade7953.current_rms[channel] < 2000) {        // No load threshold (20mA)
      Ade7953.current_rms[channel] = 0;
      Ade7953.active_power[channel] = 0;
    } else {
      Ade7953.active_power[channel] = abs(reg[channel][1]);
      apparent_power[channel] = abs(reg[channel][2]);
      reactive_power[channel] = abs(reg[channel][3]);
    }
  }

    float divider;
    for (uint32_t channel = 0; channel < 2; channel++) {

      float power_calibration = (float)EnergyGetCalibration(channel, ENERGY_POWER_CALIBRATION) / 10;

      power_calibration /= ADE7953_POWER_CORRECTION;

      float voltage_calibration = (float)EnergyGetCalibration(channel, ENERGY_VOLTAGE_CALIBRATION);
      float current_calibration = (float)EnergyGetCalibration(channel, ENERGY_CURRENT_CALIBRATION) * 10;

      Energy.frequency[channel] = 223750.0f / ((float)reg[channel][5] + 1);

      divider = (Ade7953.calib_data[channel][ADE7953_CAL_VGAIN] != ADE7953_GAIN_DEFAULT) ? 10000 : voltage_calibration;
      Energy.voltage[channel] = (float)Ade7953.voltage_rms[channel] / divider;

      divider = (Ade7953.calib_data[channel][ADE7953_CAL_WGAIN + channel] != ADE7953_GAIN_DEFAULT) ? ADE7953_LSB_PER_WATTSECOND : power_calibration;
      Energy.active_power[channel] = (float)Ade7953.active_power[channel] / divider;

      divider = (Ade7953.calib_data[channel][ADE7953_CAL_IGAIN + channel] != ADE7953_GAIN_DEFAULT) ? 100000 : current_calibration;
      Energy.current[channel] = (float)Ade7953.current_rms[channel] / divider;
    }

    return Energy;
}

