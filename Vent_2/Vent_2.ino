// All this Serial.print() will get tedious -- define some shortcuts
// These will make it easy to generate CSV data
// P(thing) or PR(thing) expands to Serial.print(thing) 
#ifndef ARDUINO_ARCH_ESP32
  #define P Serial.print // (but it seems to make the ESP32 barf)
  #define PR Serial.print
#else
  #define PR Serial.print
#endif
// PL(thing) expands to Serial.println(thing)
#define PL Serial.println
// PCS(thing) expands to Serial.print(", ");Serial.print(thing)
#define PCS Serial.print(", ");Serial.print
// PCSL(thing) expands to Serial.print(", ");Serial.println(thing)
#define PCSL Serial.print(", ");Serial.println
#define BUTTON_PIN 12

#include "RWS_UNO.h"

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Servo.h>

RWS_UNO rws = RWS_UNO();
#define SEALEVELPRESSURE_HPA (1013.25)
#define PB_DEF 4000    // breathing rate defaults to 15
#define PB_MAX 10000
#define PB_MIN 2500
#define INF_DEF 0.25    // first 40% of breath is inspiration
#define PAUSE_DEF 0 // pause before the next breath

#define A_BAT A5
#define DIV_BAT 5.0
#define A_VENTURI A0

Adafruit_BME280 bmeA, bmeV; // I2C

Servo servoCPAP, servoPEEP, servoDual;  // create servo object to control a servo
int aMinCPAP = 91, aMaxCPAP = 125;
int aMinPEEP = 84, aMaxPEEP = 50;
int aCloseCPAP = 118, aClosePEEP = 72;
double aMid = (aCloseCPAP + aClosePEEP) / 2.0;

bool readP = false;

// Variables from UI definition + a bit more
double v_o2 = 0.0;    // measured instantaneous oxygen volume fraction [0 to 1.0]
double v_p = 99.9;     // current instantaneous pressure [cm H2O]
double v_q = 99.9;      // current instantaneous flow to patient [l/min]
double v_ipp = 99.9;    // highest pressure during inspiration phase of last breath [cm H2o]
double v_ipl = 99.9;    // lowest pressure during inspiration phase of last breath [cm H2O]
unsigned v_it = 0;      // inspiration time during last breath [ms]
double v_epp = 99.9;    // highest pressure during expiration phase of last breath [cm H2o]
double v_epl = 99.9;    // lowest pressure during expiration phase of last breath [cm H2O]
unsigned v_et = 0;       // expiration time during last breath [ms]
double v_bpm = 0.0;     // BPM for last breath
double v_v = 0.0;       // inspiration volume of last breath [ml]
double v_mv = 0.0;      // volume per minute averaged over recent breaths [l / min]
byte v_alarm = 0;       // status code
double v_batv = 0.;     // measured battery voltage, should be over 13 for powered, over 12 for charge remaining
double v_venturiv = 0.;     // measured venturi voltage


void setup()
{
  rws.begin(115200);
//  Serial.begin(115200);
//  while(!Serial && millis() < 5000);

  Serial.print("\n\nRWS Vent_1\n\n");
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
  PR("Serial lines of CSV data at 115200 baud\n");
  PR("millis(), prog, fracCPAP, fracPEEP, fracDual, v_o2, v_p, v_q, v_ipp, v_ipl, v_it, v_epp, v_epl, v_et, v_bpm, v_v, v_mv, v_alarm, plus other stuff\n");
}

 
void loop()
{
  static unsigned long lastPrint = 0;
  static unsigned long lastButton = 0;
  static unsigned long perBreath = PB_DEF;
  static unsigned long startBreath = 0;
  static unsigned long endInspiration = 0;
  static unsigned long endBreath = 0;
  static double dP = 0.0;
  static double fracCPAP = 1.0; // wide open
  static double fracPEEP = 1.0; // wide open
  static double fracDual = 0.0; // halfway between, positive opens CPAP, negative opens PEEP
  static double setPEEP = 5.0;  // PEEP threshold
  double TA = 0, PA = 0, TV = 0, PV = 0;

  rws.run();
  // Test for loop rate
  if (rws.dtAvg() > 100000) PR("Taking longer than 100 ms per loop!\n");
  // Measure current state
  if(readP){
    TA = bmeA.readTemperature();
    PA = bmeA.readPressure(); // Pa
    TV = bmeV.readTemperature();
    PV = bmeV.readPressure(); // Pa
  }
  v_batv = rws.getV(A_BAT) * DIV_BAT;
  v_venturiv = rws.getV(A_VENTURI);
  dP = dP * 0.9 + (PV - PA - 0.0) * 0.1; 
  v_p = dP * 100 / 998 / 9.81;    // cm H2O
  
  // check if it is time for the next breath
  endBreath = startBreath + perBreath;
  if( endBreath - millis() > 60000 ) {  // we are past the end of expiration
    v_it = endInspiration - startBreath;  // store times for last breath
    v_et = endBreath - endInspiration;
    startBreath = millis();       // start a new breath
    v_bpm = 60000. / perBreath;
  }
  // progress through the breath sequence from 0 to 1.0
  double prog = (millis() - startBreath) / (double) perBreath;
  prog = max(prog,0.0); prog = min(prog,1.0);
  // calculate something more complicated for the opening
  if (prog < INF_DEF) {   // inhalation
    fracPEEP = 0;
    fracCPAP = 1.0;
    fracDual = 1.0;
    endInspiration = millis();
  } else {            // exhalation
    fracPEEP = 1.0;
    fracCPAP = 0.0;
    fracDual = -1.0;
//    if (dP < setPEEP * 0.95){
//      fracCPAP += 0.05;
//      fracPEEP -=0.05;
//    } else if (dP > setPEEP * 1.05) {
//      fracCPAP -= 0.05;
//      fracPEEP +=0.05;      
//    }
  }
  // force fractions in range and translate to servo positions
  fracCPAP = max(fracCPAP,0.0); fracCPAP = min(fracCPAP,1.0);
  int posCPAP = aMinCPAP + (aMaxCPAP - aMinCPAP) * fracCPAP;
  fracPEEP = max(fracPEEP,0.0); fracPEEP = min(fracPEEP,1.0);
  int posPEEP = aMinPEEP + (aMaxPEEP - aMinPEEP) * fracPEEP;
  fracDual = max(fracDual,-1.0); fracDual = min(fracDual,1.0);
  int posDual = aMid + ((aClosePEEP - aCloseCPAP) / 2.0) * fracDual;
  // write the latest servo positions
  servoDual.write(posDual);
  servoCPAP.write(posCPAP);
  servoPEEP.write(posPEEP);
  // adjust the breath by button push
  if (millis()-lastButton > 50 && digitalRead(BUTTON_PIN) == LOW) {
    lastButton = millis();
    perBreath *= 1.01;
    if(perBreath > PB_MAX) perBreath = PB_MIN; 
  }

  double osc = sin(2 * 3.14159 * prog);
  v_o2 = 0.21 + osc *0.01;
  v_q = 20 * osc;
  v_ipp = 39;
  v_ipl = 31;
  v_epp = 30;
  v_epl = 23;
  v_v = 500;
  v_mv = v_v * v_bpm;
  v_alarm = 0;
  if (millis()-lastPrint > 0) {
    lastPrint = millis();
    // Don't remove any of this stuff from the output line.
    PR(millis());
    PCS(prog,3); PCS(fracCPAP); PCS(fracPEEP); PCS(fracDual);
    PCS(v_o2); PCS(v_p); PCS(v_q);
    PCS(v_ipp); PCS(v_ipl); PCS(v_it);
    PCS(v_epp); PCS(v_epl); PCS(v_et);
    PCS(v_bpm); PCS(v_v,0); PCS(v_mv,0); PCS(v_alarm);
    // print other debug stuff after this required data
//    PCS(startBreath); PCS(endInspiration); PCS(endBreath); 
  //  PCS(PV); PCS(dP); 
  //  PCS(fracCPAP); PCS(fracPEEP);
  //  PCS(posCPAP); PCS(posPEEP); PCS(posDual);
    PL();
  }
}
