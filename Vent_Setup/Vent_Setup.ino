#include "RWS_UNO.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Servo.h>

RWS_UNO rws = RWS_UNO();
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bmeA, bmeV; // I2C

Servo servoCPAP, servoPEEP;  // create servo object to control a servo
int aMinCPAP = 93, aMaxCPAP = 125;
int aMinPEEP = 82, aMaxPEEP = 50;

void setup()
{
  rws.begin(115200);

  Serial.print("\n\nRWS UNO I2C Scan\n\n");
  unsigned status = bmeA.begin(0x77);  
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print("SensorID was: 0x"); Serial.println(bmeA.sensorID(),16);
      Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print("        ID of 0x60 represents a BME 280.\n");
      Serial.print("        ID of 0x61 represents a BME 680.\n");
      while (1);
  }
  status = bmeV.begin(0x76);  
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print("SensorID was: 0x"); Serial.println(bmeV.sensorID(),16);
      Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print("        ID of 0x60 represents a BME 280.\n");
      Serial.print("        ID of 0x61 represents a BME 680.\n");
      while (1);
  }
  servoCPAP.attach(10);  
  servoPEEP.attach(9);
  servoCPAP.write(aMaxCPAP);  
  servoPEEP.write(aMaxPEEP); 
  delay(100); 
}

 
void loop()
{
  static time_t perBreath = 10000;
  static time_t startBreath = 0;
  static time_t endBreath = 0;
  static double dP = 0.0;
  double TA = bmeA.readTemperature();
  double PA = bmeA.readPressure() / 998. / 9.81 * 100.0; // cm H20
  double TV = bmeV.readTemperature();
  double PV = bmeV.readPressure() / 998. / 9.81 * 100.0; // cm H20
  dP = dP * 0.99 + (PV - PA) * 0.01; 
  if( endBreath - millis() > 60000 ) {
    perBreath = 10000;
    startBreath = millis();
    endBreath = startBreath + perBreath;
  }
  double prog = 1.0 - (millis() - startBreath) / (double) perBreath;
  int posCPAP = 90;
  int posPEEP = 90;
  servoCPAP.write(posCPAP);
  servoPEEP.write(posPEEP);
  delay(10);
  PR(PV); PCS(PA); PCS(dP);
  PL();
}
