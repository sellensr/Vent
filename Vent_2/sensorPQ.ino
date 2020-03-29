#ifdef P_BME280
void setupP(){
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
 
double getP(){
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
void setupP(){
  
}
 
double getP(){
  return 99.99;   
}
#endif

#ifdef P_NONE
void setupP(){
  
}
 
double getP(){
  return 99.99;   
}
#endif
