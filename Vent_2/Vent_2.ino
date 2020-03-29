#include "RWS_UNO.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Servo.h>

// define only one source for P
//#define P_NONE          // there is no patient pressure sensor
#define P_BME280        // use the paired BME280s as the patient pressure sensors
//#define P_PX137         // use a PX137 as the patient pressure sensor

// define only one source for Q
#define Q_PX137         // use a PX137 as the flow sensor

RWS_UNO uno = RWS_UNO();
#define PB_DEF 10000    // breathing rate default [ms / breath]
#define PB_MAX 10000    // slowest breathing
#define PB_MIN 2500     // fastest breathing
#define INF_DEF 0.4     // first 40% of breath is inspiration

#define IT_MAX 10000    // Inspiration time max
#define ET_MAX 10000    // Expiration time max
#define IT_MIN 500      // Inspiration Time min
#define ET_MIN 1000     // Expiration time min

// read the battery state from a voltage divider
#define A_BAT A5
#define DIV_BAT 5.0
#define A_VENTURI A0
#define ALARM_PIN 5
#define ALARM_DELAY 3000  // don't alarm until the condition has lasted this long
#define ALARM_LENGTH 100  // don't make an alarm sound longer than this

Adafruit_BME280 bmeA, bmeV; // I2C

// Servo geometry will depend on the physical assembly of each unit and variability between servos.
// These values will have to be set for each individual machine after final assembly.
Servo servoCPAP, servoPEEP, servoDual;  // create servo object to control a servo
int aMinCPAP = 91, aMaxCPAP = 125;      // independent CPAP valve position settings [servo degrees closed (Min) and open (Max)]
int aMinPEEP = 84, aMaxPEEP = 50;       // independent PEEP valve position settings [servo degrees closed (Min) and open (Max)]
int aCloseCPAP = 114, aClosePEEP = 77;  // dual valve position settings for fully closed [servo degrees]
double aMid = (aCloseCPAP + aClosePEEP) / 2.0;

// Detecting the time to end a phase does not instantly open valves and change flows.
// We need to wait a little while before we say we are in the next phase.
int ieTime = 400;   // transition between end of inspiration and start of expiration phase
int eiTime = 400;   // transition time between end of expiration phase and start of inspiration phase

// Go to different modes to allow testing
bool readPBME = true;    // set false to allow testing on a machine with no BMEs or to use other P sensors instead
bool plotterMode = false;// set true for output visualization using arduino ide plotter mode

// Variables from UI definition + a bit more
// v_ for all elements that are measured or calculated from actual operations
double v_o2 = 0.0;            // measured instantaneous oxygen volume fraction [0 to 1.0]
double v_p = 99.9;            // current instantaneous pressure [cm H2O]
double v_q = 0.0;             // current instantaneous flow to patient [l/min]
double v_ipp = 99.9;          // highest pressure during inspiration phase of last breath [cm H2o]
double v_ipl = 99.9;          // lowest pressure during inspiration phase of last breath [cm H2O]
double v_ipmax = 0.0;         // highest pressure so far during this inspiration phase [cm H2o]
double v_ipmin = 99.9;        // lowest pressure so far during this inspiration phase [cm H2O]
int v_it = 0;                 // inspiration time during last breath [ms]
int v_itr = 0;                // rolling inspiration time during current breath [ms]
double v_epp = 99.9;          // highest pressure during expiration phase of last breath [cm H2o]
double v_epl = 99.9;          // lowest pressure during expiration phase of last breath [cm H2O]
double v_epmax = 0.0;         // highest pressure so far during this expiration phase [cm H2o]
double v_epmin = 99.9;        // lowest pressure so far during this expiration phase [cm H2O]
int v_et = 0;                 // expiration time during last breath [ms]
int v_etr = 0;                // rolling expiration time during current breath [ms]
double v_bpm = 0.0;           // BPM for last breath
double v_v = 0.0;             // inspiration volume of last breath [ml]
double v_mv = 0.0;            // volume per minute averaged over recent breaths [l / min]
unsigned long v_alarm = 0;    // status code, normally VENT_NO_ERROR, VENT_EXT_ERROR if externally imposed
double v_batv = 0.;           // measured battery voltage, should be over 13 for powered, over 12 for charge remaining
double v_venturiv = 0.;       // measured venturi voltage
int v_ie = 0;                 // set to 0 when between phases, 1 for inspiration phase, -1 for expiration phase
unsigned long v_alarmOnTime = 0;
unsigned long v_alarmOffTime = 0;

// p_ for all elements that are set parameters for desired performance
double p_iph = 13.0;          // the inspiration pressure upper bound.
double p_ipl = 6.0;           // the inspiration pressure lower bound -- PEEP setting.
double p_iphTol = 0.5;        // excursion from p_eph required to trigger start of expiration if P > p_iph + p_iphTol
double p_eph = 20.0;          // the expiration pressure upper bound.
double p_epl = 8.0;           // the expiration pressure lower bound -- PEEP setting.
double p_eplTol = 0.5;        // excursion from p_epl required to trigger start of new breath if P < p_epl - p_eplTol
int p_it = PB_DEF * INF_DEF;  // inspiration time setting
int p_et = PB_DEF - p_it;     // expiration time setting
bool p_trigEnabled = true;    // enable triggering on pressure limits
bool p_closeCPAP = false;     // set true to close the CPAP valve, set false for normal running
bool p_alarm = false;         // set true for an alarm condition imposed externally

#define VENT_NO_ERROR   0b0  ///< There is no Error
#define VENT_IPL_ERROR  0b1  ///< Inspiration Pressure Low
#define VENT_IPH_ERROR  0b10  ///< Inspiration Pressure High
#define VENT_ITS_ERROR  0b100  ///< Inspiration Time Short
#define VENT_ITL_ERROR  0b1000  ///< Inspiration Time Long
#define VENT_EPL_ERROR  0b10000  ///< Expiration Pressure Low
#define VENT_EPH_ERROR  0b100000  ///< Expiration Pressure High
#define VENT_ETS_ERROR  0b1000000  ///< Expiration Time Short
#define VENT_ETL_ERROR  0b10000000  ///< Expiration Time Long
#define VENT_EXT_ERROR  0b1000000000000000  ///< External Error

void setup()
{
  uno.begin(115200);      // Start the RWS_UNO library object
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
  servoDual.attach(11);  // actuates both valve bodies alternately
  servoCPAP.attach(10);  // actuates CPAP valve only
  servoPEEP.attach(9);   // actuates PEEP valve only
  servoDual.write(aMid);
  servoCPAP.write(aMaxCPAP);  
  servoPEEP.write(aMaxPEEP); 

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, HIGH);
  delay(5); // just long enough to make a little squeak
  digitalWrite(ALARM_PIN, LOW);
  delay(100); 
  if(!plotterMode){
    PR("Serial lines of CSV data at 115200 baud\n");
    PR("  millis(),  prog,  CPAP,  PEEP,  Dual,  v_o2,   v_p,   v_q, v_ipp, v_ipl,  v_it, v_epp, v_epl,  v_et, v_bpm,   v_v,   v_mv, v_alarm, v_ie, plus other stuff\n");
  }
  Serial1.print("Serial lines of CSV data at 115200 baud\n");
  Serial1.print("  millis(),  prog,  CPAP,  PEEP,  Dual,  v_o2,   v_p,   v_q, v_ipp, v_ipl,  v_it, v_epp, v_epl,  v_et, v_bpm,   v_v,   v_mv, v_alarm, v_ie, plus other stuff\n");
}

 
