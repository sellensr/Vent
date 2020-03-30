/**************************************************************************/
/*!
  @file sensorPQ.ino

  @section intro Introduction

  An Arduino sketch for running tests on an HMRC DIY ventilator. sensorPQ tab.
  Provides functions to measure patient pressure and flow rate.

  @section author Author

  Written by

  @section license License

  CCBY license

  Code provided only for example purposes. Must not be used for any purpose 
  critical to life or health, or protection of property.
*/
/**************************************************************************/
#ifdef P_BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bmeA, bmeV; // I2C

/**************************************************************************/
/*!
    @brief Do any setup required for patient pressure measurement
    @param none
    @return none
*/
/**************************************************************************/
void setupP(){  // do any setup required for pressure measurement
  unsigned status = bmeA.begin(0x77);  
  PR("bmeA started\n");
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print("SensorID was: 0x"); Serial.println(bmeA.sensorID(),16);
      Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print("        ID of 0x60 represents a BME 280.\n");
      Serial.print("        ID of 0x61 represents a BME 680.\n");
  }
  status = bmeV.begin(0x76);  
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print("SensorID was: 0x"); Serial.println(bmeV.sensorID(),16);
      Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print("        ID of 0x60 represents a BME 280.\n");
      Serial.print("        ID of 0x61 represents a BME 680.\n");
  }
}
 
/**************************************************************************/
/*!
    @brief Get a new value for patient pressure
    @param none
    @return the current value for patient pressure in cm H20
*/
/**************************************************************************/
double getP(){  // return the current value for patient pressure in cm H20
  static double dP = 0.0;
  double TA = 0, PA = 0, TV = 0, PV = 0;
  // Measure current state
  TA = bmeA.readTemperature();
  PA = bmeA.readPressure(); // Pa
  TV = bmeV.readTemperature();
  PV = bmeV.readPressure(); // Pa
  dP = dP * 0.9 + (PV - PA - 0.0) * 0.1; 
  return dP * 100 / 998 / 9.81;    // cm H2O   
}
#endif

#ifdef P_PX137
void setupP(){  // do any setup required for pressure measurement
  
}
 
double getP(){  // return the current value for patient pressure in cm H20
  double v = uno.getV(A_PX137);
  v_px137v = v_px137v * 0.9 + v * 0.1; 
  return 0.42;   
}
#endif

#ifdef P_NONE
void setupP(){  // do any setup required for pressure measurement
  
}
 
double getP(){  // return the current value for patient pressure in cm H20
  return 0.42;   
}
#endif

#ifdef Q_NONE
/**************************************************************************/
/*!
    @brief Do any setup required for patient flow measurement
    @param none
    @return none
*/
/**************************************************************************/
void setupQ(){  // do any setup required for flow measurement
  
}

/**************************************************************************/
/*!
    @brief Get a new value for patient flow
    @param none
    @return the current value for patient flow in litres / minute
*/
/**************************************************************************/
double getQ(){  // return the current value for patient flow in litres / minute
  return 0.42;
}
#endif

#ifdef Q_PX137
void setupQ(){  // do any setup required for flow measurement
  
}

double getQ(){  // return the current value for patient flow in litres / minute
  double v = uno.getV(A_VENTURI);
  double w = 0.1;
  v_venturiv = v_venturiv * (1-w) + v * w; 
  double q = pow(abs(v_venturiv - OFFSET_VENTURI),0.5) * SCALE_VENTURI;
  // do this supression somewhere else like at the display
  // if(q < MINQ_VENTURI) q = 0; // supress small noisy values for optics!
  return q;
}
#endif
