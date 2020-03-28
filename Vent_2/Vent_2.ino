#include "RWS_UNO.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Servo.h>

RWS_UNO uno = RWS_UNO();
#define SEALEVELPRESSURE_HPA (1013.25)
#define PB_DEF 10000    // breathing rate default
#define PB_MAX 10000
#define PB_MIN 2500
#define INF_DEF 0.4    // first 40% of breath is inspiration
#define PAUSE_DEF 0 // pause before the next breath
#define IT_MAX 10000
#define ET_MAX 10000
#define IT_MIN 500
#define ET_MIN 1000

#define A_BAT A5
#define DIV_BAT 5.0
#define A_VENTURI A0

Adafruit_BME280 bmeA, bmeV; // I2C

Servo servoCPAP, servoPEEP, servoDual;  // create servo object to control a servo
int aMinCPAP = 91, aMaxCPAP = 125;      // independent CPAP valve position settings [servo degrees closed (Min) and open (Max)]
int aMinPEEP = 84, aMaxPEEP = 50;       // independent PEEP valve position settings [servo degrees closed (Min) and open (Max)]
int aCloseCPAP = 114, aClosePEEP = 77;  // dual valve position settings for fully closed [servo degrees]
double aMid = (aCloseCPAP + aClosePEEP) / 2.0;

bool readPBME = true;    // set false to allow testing on a machine with no BMEs or to use other P sensors instead

// Variables from UI definition + a bit more
double v_o2 = 0.0;            // measured instantaneous oxygen volume fraction [0 to 1.0]
double v_p = 99.9;            // current instantaneous pressure [cm H2O]
double v_q = 99.9;            // current instantaneous flow to patient [l/min]
double v_ipp = 99.9;          // highest pressure during inspiration phase of last breath [cm H2o]
double v_ipl = 99.9;          // lowest pressure during inspiration phase of last breath [cm H2O]
int v_it = 0;                 // inspiration time during last breath [ms]
double v_epp = 99.9;          // highest pressure during expiration phase of last breath [cm H2o]
double v_epl = 99.9;          // lowest pressure during expiration phase of last breath [cm H2O]
int v_et = 0;                 // expiration time during last breath [ms]
double v_bpm = 0.0;           // BPM for last breath
double v_v = 0.0;             // inspiration volume of last breath [ml]
double v_mv = 0.0;            // volume per minute averaged over recent breaths [l / min]
byte v_alarm = 0;             // status code
double v_batv = 0.;           // measured battery voltage, should be over 13 for powered, over 12 for charge remaining
double v_venturiv = 0.;       // measured venturi voltage

double p_iph = 13.0;          // the inspiration pressure upper bound.
double p_ipl = 6.0;           // the inspiration pressure lower bound -- PEEP setting.
double p_iphTol = 0.5;        // excursion from p_eph required to trigger start of expiration if P > p_iph + p_iphTol
double p_eph = 20.0;          // the expiration pressure upper bound.
double p_epl = 8.0;           // the expiration pressure lower bound -- PEEP setting.
double p_eplTol = 0.5;        // excursion from p_epl required to trigger start of new breath if P < p_epl - p_eplTol
int p_it = PB_DEF * INF_DEF;  // inspiration time setting
int p_et = PB_DEF - p_it;     // expiration time setting
bool p_trigEnabled = true;   // enable triggering on pressure limits

void setup()
{
  uno.begin(115200);
  Serial1.begin(115200);  // for communication to display unit or other supervisor. RX-->GND for none
  while(!Serial1 && millis() < 5000);

  Serial.print("\n\nRWS Vent_2\n\n");
  if(readPBME){
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
  servoDual.attach(11);  
  servoCPAP.attach(10);  
  servoPEEP.attach(9);
  servoDual.write(90);
  servoCPAP.write(aMaxCPAP);  
  servoPEEP.write(aMaxPEEP); 

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  delay(100); 
  //PR("Serial lines of CSV data at 115200 baud\n");
  //PR("  millis(),  prog,  CPAP,  PEEP,  Dual,  v_o2,   v_p,   v_q, v_ipp, v_ipl,  v_it, v_epp, v_epl,  v_et, v_bpm,   v_v,   v_mv, v_alarm, plus other stuff\n");
  Serial1.print("Serial lines of CSV data at 115200 baud\n");
  Serial1.print("  millis(),  prog,  CPAP,  PEEP,  Dual,  v_o2,   v_p,   v_q, v_ipp, v_ipl,  v_it, v_epp, v_epl,  v_et, v_bpm,   v_v,   v_mv, v_alarm, plus other stuff\n");
}

 
