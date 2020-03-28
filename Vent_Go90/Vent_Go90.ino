#include "RWS_UNO.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Servo.h>

RWS_UNO rws = RWS_UNO();
#define SEALEVELPRESSURE_HPA (1013.25)
#define PB_DEF 4000    // breathing rate defaults to 15
#define PB_MAX 10000
#define PB_MIN 2500
#define INF_DEF 0.4    // first 40% of breath is inspiration
#define PAUSE_DEF 0 // pause before the next breath

Adafruit_BME280 bmeA, bmeV; // I2C

Servo servoCPAP, servoPEEP, servoDual;  // create servo object to control a servo
int aMinCPAP = 91, aMaxCPAP = 125;
int aMinPEEP = 84, aMaxPEEP = 50;
int aCloseCPAP = 94, aClosePEEP = 84;
double aMid = (aCloseCPAP + aClosePEEP) / 2.0;

double Ppat = 99.9;   // the current pressure to the patient [cm H2O]
double Qpat = 99.9;   // the current flow to the patient [l/min]
double Vlast = 9999;   // the total volume delivered on the last breath [ml]
double XO2 = 9.99;    // measured oxygen volume fraction [0 to 1.0]

void setup()
{
  rws.begin(115200);

  Serial.print("\n\nRWS Vent_1\n\n");
  PR("Serial lines of CSV data at 115200 baud\n");
  PR("millis(), prog, fracCPAP, fracPEEP, fracDual, P [cm H2O], Q [l/min], V [ml last breath], XO2 [vol fraction], BPM set, Patm [Pa], plus other stuff\n");
  unsigned status = bmeA.begin(0x77);  
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
  servoDual.attach(11);  
  servoCPAP.attach(10);  
  servoPEEP.attach(9);
  servoDual.write(90);
  servoCPAP.write(aMaxCPAP);  
  servoPEEP.write(aMaxPEEP); 

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  delay(100); 
}

 
void loop()
{
  servoDual.write(90);
  servoCPAP.write(90);
  servoPEEP.write(90);
}
