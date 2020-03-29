#ifdef P_BME280
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
  return 99.99;   
}
#endif

#ifdef P_NONE
void setupP(){  // do any setup required for pressure measurement
  
}
 
double getP(){  // return the current value for patient pressure in cm H20
  return 99.99;   
}
#endif

#ifdef Q_NONE
void setupQ(){  // do any setup required for flow measurement
  
}

double getQ(){  // return the current value for patient flow in litres / minute
  return 9999.9;
}
#endif

#ifdef Q_PX137
void setupQ(){  // do any setup required for flow measurement
  
}

double getQ(){  // return the current value for patient flow in litres / minute
  return 999.9;
}
#endif
