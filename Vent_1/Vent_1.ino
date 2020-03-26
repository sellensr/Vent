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

  Serial.print("\n\nRWS Vent_1\n\n");
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
  static unsigned long perBreath = 10000;
  static unsigned long startBreath = 0;
  static unsigned long endBreath = 0;
  static double dP = 0.0;
  static double fracCPAP = 1.0; // wide open
  static double fracPEEP = 1.0; // wide open
  static double setPEEP = 5.0;  // PEEP threshold
  double TA = bmeA.readTemperature();
  double PA = bmeA.readPressure() / 998. / 9.81 * 100.0; // cm H20
  double TV = bmeV.readTemperature();
  double PV = bmeV.readPressure() / 998. / 9.81 * 100.0; // cm H20
  dP = dP * 0.9 + (PV - PA - 0.06) * 0.1; 
  // check if it is time for the next breath
  if( endBreath - millis() > 60000 ) {
    perBreath = 10000;  // get the latest period in ms / breath
    startBreath = millis();
    endBreath = startBreath + perBreath;
    delay(1000);
  }
  // progress through the breath sequence from 0 to 1.0
  double prog = (millis() - startBreath) / (double) perBreath;
  prog = max(prog,0.0); prog = min(prog,1.0);
  // default to moving with the progress variable
//  fracCPAP = prog;
//  fracPEEP = prog;
  // calculate something more complicated for the opening
  if (prog < 0.4) {   // inhalation
    fracPEEP = 0;
    fracCPAP = 1.0;
  } else {            // exhalation
    if (dP < setPEEP * 0.95){
      fracCPAP += 0.05;
      fracPEEP -=0.05;
    } else if (dP > setPEEP * 1.05) {
      fracCPAP -= 0.05;
      fracPEEP +=0.05;      
    }
  }
  // force fractions in range and translate to servo positions
  fracCPAP = max(fracCPAP,0.0); fracCPAP = min(fracCPAP,1.0);
  int posCPAP = aMinCPAP + (aMaxCPAP - aMinCPAP) * fracCPAP;
  fracPEEP = max(fracPEEP,0.0); fracPEEP = min(fracPEEP,1.0);
  int posPEEP = aMinPEEP + (aMaxPEEP - aMinPEEP) * fracPEEP;
  // write the latest servo positions
  servoCPAP.write(posCPAP);
  servoPEEP.write(posPEEP);
  delay(10);
  PR(PV); PCS(PA); PCS(dP); 
  PCS(posCPAP); PCS(posPEEP);
//  PCS(millis()); PCS(startBreath); PCS(endBreath); 
  PCS(prog,4);
  PL();
}
