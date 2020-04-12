/**************************************************************************/
/*!
  @file Vent_4.ino

  @section intro Introduction

  An Arduino sketch for running tests on an HMRC DIY ventilator.

  For a sense of proportion, 20 cm H2O is about 2 kPa, well within normal barometric
  pressure variation. 20 cm H20 is about 170 metres of elevation in air, so all of the 
  operating conditions we are contemplating for our CPAPs are within normal absolute 
  pressures considered when they were designed and approved for patient use.

  Vent_3 FROZEN @ 12:11 2020-04-03 as submitted for challenge. Moving on to Vent_4

  Vent requires some libraries you may not have installed. These two can be installed
  via Tools/Manage Libraries... inside the IDE. Search for sdFat and be sure to install
  the Adafruit fork of the original. 
  #include <SdFat.h>                // https://github.com/adafruit/SdFat
  #include <Adafruit_SPIFlash.h>    // https://github.com/adafruit/Adafruit_SPIFlash
  This one you will need to download from github and copy into your libraries folder.
  #include "RWS_UNO.h"    // https://github.com/sellensr/RWS_UNO

  BE SURE TO SET ALARM_LENGTH LONGER FOR PRODUCTION!!!!!


  @section author Author

  Written by

  @section license License

  CCBY license

  Code provided only for example purposes. Must not be used for any purpose 
  critical to life or health, or protection of property.
*/
/**************************************************************************/
#define VENT_VERSION "0.4.5"  // release / code freeze, e.g. 4 for Vent_4 / update
#include "RWS_UNO.h"    // https://github.com/sellensr/RWS_UNO
#include <Servo.h>
/****************SET INSTRUMENTATION TYPES HERE****************************/
// define only one source for P or else you will get a redefinition of 'xxx' error
//#define P_NONE          ///< there is no patient pressure sensor
//#define P_BME280        ///< use the paired BME280s as the patient pressure sensors
#define P_PX137         ///< use a PX137 as the patient pressure sensor

// define only one source for Q or else you will get a redefinition of 'xxx' error
//#define Q_NONE          ///< there is no patient flow sensor
//#define Q_PX137         ///< use a PX137 as the flow sensor
#define Q_CAP2          ///< use 2 Capillary sensors, one on CPAP side, one on PEEP, diff is flow

// define only one hardware prototype -- config details to be resolved for production
//#define YGK_MCL       ///< McLaughlin Hall Prototype -- Avoid using, may conflict with config file setup
//#define YGK_RWS       ///< Rick Sellens Prototype    -- Avoid using, may conflict with config file setup
#define YGK_GENERIC   ///< Generic Prototype needs settings and configuration
/***************SET PINS HERE TO MATCH HARDWARE CONFIGURATION**************/
// The calibration scale and offset values will be different for every combination of transducers.
// read the battery state from a voltage divider
#define A_BAT A2
#define DIV_BAT 5.0
#define A_BLOWER A0 ///< Use A0 as an analog output for blower speed control if using a blower for the air source.
// read the venturi as below
#define A_VENTURI A5
#define QSCALE_VENTURI  11.28   ///< (l/min) / cmH2O^0.5 to get Q = QSCALE_VENTURI * pow(p,0.5)
#define PSCALE_VENTURI  23.53   ///< cmH20 / volt for venturi pressure sensor
#define OFFSET_VENTURI   1.3209 ///< volts at zero differential pressure on venturi
// read the PX137 for patient pressure as below
#define A_PX137 A4
#define PSCALE_PX137    53.60   ///< cmH20 / volt for PX137 patient pressure sensor
#define OFFSET_PX137     1.2945 ///< volts at zero patient pressure
// read the Capillary sensors as below
#define A_CAP_CPAP A5           ///< replaces venturi
#define SCALE_CAP_CPAP 130.00   ///< (l/min) / volt for capillary input from CPAP side
#define OFFSET_CAP_CPAP  1.3192 ///< volts at zero differential pressure on CPAP side
#define A_CAP_PEEP A3           ///< new in Vent_4
#define SCALE_CAP_PEEP  80.00   ///< (l/min) / volt for capillary input from PEEP side
#define OFFSET_CAP_PEEP  1.2750 ///< volts at zero differential pressure on PEEP side

// other hardware and display settings below
#define MINQ_VENTURI 1.0  ///< minimum litre/min to display as non-zero
#define ALARM_PIN            5    // Pin 5 has 5 volt higher current output for buzzer! 
                                  // Although 3 volts is enough with sufficient current, could switch to 7 
#define BLUE_BUTTON_PIN     12
#define YELLOW_BUTTON_PIN    3
#define RED_BUTTON_PIN       4
#define MAX_COMMAND_LENGTH 200    // no lines longer than this for commands or output


RWS_UNO uno = RWS_UNO();
#define PB_DEF 10000    ///< breathing rate default [ms / breath]
#define PB_MAX 10000    ///< slowest breathing
#define PB_MIN 2500     ///< fastest breathing
#define INF_DEF 0.4     ///< first 40% of breath is inspiration

#define IT_MAX 10000    ///< Inspiration time max
#define ET_MAX 10000    ///< Expiration time max
#define IT_MIN 500      ///< Inspiration time min
#define ET_MIN 1000     ///< Expiration time min

#define IP_MAX 45       ///< Inspiration pressure max
#define EP_MAX 40       ///< Expiration pressure max
#define IP_MIN 2        ///< Inspiration pressure min
#define EP_MIN 1        ///< Expiration pressure min

#define STOP_MAX 30000  ///< Max time in stop mode

#define ALARM_DELAY         3000  ///< [ms] don't alarm until the condition has lasted this long
#define ALARM_LENGTH          20  ///< [ms] don't make an alarm sound longer than this, set short only during debugging
#define ALARM_DELAY_DISPLAY 3000  /// [ms] don't set the alarm condition until the display has been silent this long
#define ALARM_DELAY_LOOP     100  /// [ms] set the alarm condition if loop is taking longer than this

// Servo geometry will depend on the physical assembly of each unit and variability between servos.
// These values will have to be set for each individual machine after final assembly.
// Minimum values are for minimum flow, thus fully closed, Max for fully open
Servo servoCPAP, servoPEEP, servoDual;  ///< create servo object to control a servo

// These are the generic settings, which will need to be configured
#ifdef YGK_GENERIC
 int aMinCPAP = 90, aMaxCPAP = 90;
 int aMinPEEP = 90, aMaxPEEP = 90;
 int aCloseCPAP = 90, aClosePEEP = 90;  ///< dual valve position settings for fully closed [servo degrees]
#endif

// These are the settings for the 2020-03-30 configuration on Rick's Board
#ifdef YGK_RWS
 int aMinCPAP = 22, aMaxCPAP = 52;
 int aMinPEEP = 169, aMaxPEEP = 139;
 int aCloseCPAP = 66, aClosePEEP = 98;  ///< dual valve position settings for fully closed [servo degrees]
#endif

// These are the settings for the 2020-03-31 configuration on the McLaughlin Board
#ifdef YGK_MCL
 int aMinCPAP = 130, aMaxCPAP = 180;
 int aMinPEEP = 90, aMaxPEEP = 140;
 int aCloseCPAP = 66, aClosePEEP = 98;  ///< dual valve position settings for fully closed [servo degrees]
#endif

double aMid = (aCloseCPAP + aClosePEEP) / 2.0;

// Detecting the time to end a phase does not instantly open valves and change flows.
// We need to wait a little while before we say we are in the next phase.
int ieTime = 400;   ///< transition between end of inspiration and start of expiration phase
int eiTime = 400;   ///< transition time between end of expiration phase and start of inspiration phase

// Go to different modes to allow testing
int slowPrint = 1;           ///< set larger than 1 to print data more slowly

  double fracCPAP = 1.0;                 // target opening fraction for the CPAP valve. 0 for closed, 1.0 for wide open
  double fracPEEP = 1.0;                 // target opening fraction for the CPAP valve. 0 for closed, 1.0 for wide open
  double fracDual = 0.0;                 // target position for Dual Valve. 0.0 for halfway between, 1.0 fully opens CPAP, -1.0 fully opens PEEP
  double prog = 0;                       // progress through the current scheduled breath
  
// Global Variables from UI definition + a bit more
// v_ for all elements that are measured or calculated from actual operations
// Not necessarily declared in order of output to the display unit. Check the output code.
double v_o2 = 0.0;            ///< measured instantaneous oxygen volume fraction [0 to 1.0]
double v_p = 0.0;             ///< current instantaneous pressure [cm H2O]
double v_q = 0.0;             ///< current instantaneous flow to patient [l/min]
double v_qCPAP = 0.0;         ///< current instantaneous flow returning on PEEP side [l/min]
double v_qPEEP = 0.0;         ///< current instantaneous flow out on CPAP side [l/min]
double v_pp = 0.0;            ///< highest pressure during any phase of last breath [cm H2o]
double v_pl = 0.0;            ///< lowest pressure during any phase of last breath [cm H2O]
double v_pmax = 0.0;          ///< highest pressure so far during this breath [cm H2o]
double v_pmin = 0.0;          ///< lowest pressure so far during this breath [cm H2O]
double v_ipp = 0.0;           ///< highest pressure during inspiration phase of last breath [cm H2o]
double v_ipl = 0.0;           ///< lowest pressure during inspiration phase of last breath [cm H2O]
double v_ipmax = 0.0;         ///< highest pressure so far during this inspiration phase [cm H2o]
double v_ipmin = 0.0;         ///< lowest pressure so far during this inspiration phase [cm H2O]
int v_it = 0;                 ///< inspiration time during last breath [ms]
int v_itr = 0;                ///< rolling inspiration time during current breath [ms]
double v_epp = 0.0;           ///< highest pressure during expiration phase of last breath [cm H2o]
double v_epl = 0.0;           ///< lowest pressure during expiration phase of last breath [cm H2O]
double v_epmax = 0.0;         ///< highest pressure so far during this expiration phase [cm H2o]
double v_epmin = 0.0;         ///< lowest pressure so far during this expiration phase [cm H2O]
int v_et = 0;                 ///< expiration time during last breath [ms]
int v_etr = 0;                ///< rolling expiration time during current breath [ms]
double v_bpm = 0.0;           ///< BPM for last breath
double v_bpms = 0.0;          ///< BPM averaged over recent breaths
double v_v = 0.0;             ///< inspiration volume of last breath [ml]
double v_vr = 0.0;            ///< rolling inspiration volume of current breath [ml]
double v_mv = 0.0;            ///< volume per minute averaged over recent breaths [l / min]
unsigned long v_alarm = 0;    ///< status code, normally VENT_NO_ERROR, VENT_EXT_ERROR if externally imposed
double v_batv = 0.;           ///< measured battery voltage, should be over 13 for powered, over 12 for charge remaining
double v_venturiv = 0.;       ///< measured venturi voltage
double v_px137v = 0.;         ///< measured patient pressure voltage
double v_CPAPv = 0.;          ///< measured CPAP side flow element voltage
double v_PEEPv = 0.;          ///< measured PEEP side flow element voltage
int v_ie = 0;                 ///< set to 0 when between phases, 1 for inspiration phase, -1 for expiration phase
unsigned long v_alarmOnTime = 0;    ///< time of the first alarm state that occurred since all alarms were clear
unsigned long v_alarmOffTime = 0;   ///< time that alarms were last cleared
unsigned long v_lastStop = 0;       ///< set to millis() when the last Stop Command input was received
double v_tauW = 0.001;          ///< the smoothing weight factor to use this cycle for time constant p_tau

// p_ for all elements that are set parameters for desired performance
double p_iph = IP_MAX;        ///< the inspiration pressure upper bound.
double p_ipl = 0.0;           ///< the inspiration pressure lower bound -- PEEP setting, no action
double p_iphTol = 0.5;        ///< difference from p_eph required to trigger start of expiration if P > p_iph - p_iphTol or alarm if beyond
double p_eph = EP_MAX;        ///< the expiration pressure upper bound, no action
double p_epl = 0.0;           ///< the expiration pressure lower bound -- PEEP setting.
double p_eplTol = 0.5;        ///< difference from p_epl required to trigger start of new breath if P < p_epl + p_eplTol or alarm if beyond
int p_it = PB_DEF * INF_DEF;  ///< inspiration time setting, high/low limits
int p_ith = IT_MAX;
int p_itl = IT_MIN;
int p_et = PB_DEF - p_it;     ///< expiration time setting, high/low limits
int p_eth = ET_MAX;
int p_etl = ET_MIN;
bool p_trigEnabled = false;   ///< enable triggering on pressure limits
bool p_closeCPAP = false;     ///< set true to close the CPAP valve, must be set false for normal running
bool p_openAll = false;     ///< set true to open all the valves, must be set false for normal running
bool p_stopped = false;     ///< set true to enable menu items that could interfere with normal running
bool p_config = false;     ///< set true to prevent return to run mode during configuration processes
bool p_alarm = false;         ///< set true for an alarm condition imposed externally
bool p_plotterMode = false;     ///< set true for output visualization using arduino ide plotter mode
bool p_printConsole = true;     ///< set false to turn off console data output, notmally true
bool p_useSerial1 = false;      ///< set false to avoid noise on RX of Serial1 if not connected

double p_pScale = PSCALE_PX137;         ///< cmH20 / volt for PX137 patient pressure sensor
double p_pOffset = OFFSET_PX137;        ///< volts at zero patient pressure
double p_qScaleCPAP = SCALE_CAP_CPAP;   ///< (l/min) / volt for capillary input from CPAP side
double p_qOffsetCPAP = OFFSET_CAP_CPAP; ///< volts at zero differential pressure on CPAP side
double p_qScalePEEP = SCALE_CAP_PEEP;   ///< (l/min) / volt for capillary input from PEEP side
double p_qOffsetPEEP = OFFSET_CAP_PEEP; ///< volts at zero differential pressure on PEEP side
double p_tau = 0.10;                    ///< instrumentation smoothing time constant [s]

int p_modelNumber = 3;          ///< Hardware model number, 1 was abandoned, 2 was single servo and venturi, 
                                //   3 is single or double servo gates with flow elements in both feeds 
                          
int p_serialNumber = 30000001;  ///< Hardware serial number is model number * 10000000 + unique integer

// stackable error codes that will fit into v_alarm
#define VENT_NO_ERROR   0b0                 ///<     0 There is no Error
#define VENT_IPL_ERROR  0b0000000000000001  ///<     1 Inspiration Pressure Low < p_ipl
#define VENT_IPH_ERROR  0b0000000000000010  ///<     2 Inspiration Pressure High > p_iph
#define VENT_ITS_ERROR  0b0000000000000100  ///<     4 Inspiration Time Short < p_itl
#define VENT_ITL_ERROR  0b0000000000001000  ///<     8 Inspiration Time Long > p_ith
#define VENT_EPL_ERROR  0b0000000000010000  ///<    16 Expiration Pressure Low < p_epl
#define VENT_EPH_ERROR  0b0000000000100000  ///<    32 Expiration Pressure High > p_eph
#define VENT_ETS_ERROR  0b0000000001000000  ///<    64 Expiration Time Short < p_etl
#define VENT_ETL_ERROR  0b0000000010000000  ///<   128 Expiration Time Long > p_eth
#define VENT_STOP_WARN  0b0001000000000000  ///<  4096 Going back to run mode soon
#define VENT_SLOW_ERROR 0b0010000000000000  ///<  8192 Loop is not executing in under ALARM_DELAY_LOOP
#define VENT_DISP_ERROR 0b0100000000000000  ///< 16384 Display/Console Incognito longer than ALARM_DELAY_DISPLAY
#define VENT_EXT_ERROR  0b1000000000000000  ///< 32768 External Error

/**************************************************************************/
/*!
    @brief Initialization of HMRC DIY Ventilator
    @param none
    @return none
*/
/**************************************************************************/
void setup()
{
  uno.begin(115200);      ///< Start the RWS_UNO library object
  Serial1.begin(115200);  ///< for communication to display unit or other supervisor. RX-->GND for none
  while(!Serial1 && millis() < 5000);
  setupFlash();   // reads all the calibration data, hardware model and serial numbers
  PR("\n\nYGK Modular Ventilator\n\nFirmware Version: "); PR(VENT_VERSION); 
  PR("   Model: "); PR(p_modelNumber); PR("    Serial Number: "); PR(p_serialNumber);
  PR("\n\n");
  setupP();
  setupQ();
  servoDual.attach(9);    ///< actuates both valve bodies alternately 9
  servoPEEP.attach(10);   ///< actuates PEEP valve only 10
  servoCPAP.attach(11);   ///< actuates CPAP valve only 11
  servoDual.write(aMid);
  servoPEEP.write(aMaxPEEP); 
  servoCPAP.write(aMaxCPAP);  

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BLUE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(YELLOW_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, HIGH);
  delay(5); ///< just long enough to make a little squeak
  digitalWrite(ALARM_PIN, LOW);
  delay(100); 
  if(!p_plotterMode){
    PR("Serial lines of CSV data at 115200 baud\n");
    PR("  millis(),  prog,  CPAP,  PEEP,  Dual,  v_o2,   v_p,   v_q, v_ipp, v_ipl,  v_it, v_epp, v_epl,  v_et, v_bpm,   v_v,   v_mv, v_alarm, v_ie, plus other stuff\n");
  }
  Serial1.print("Serial lines of CSV data at 115200 baud\n");
  Serial1.print("  millis(),  prog,  CPAP,  PEEP,  Dual,  v_o2,   v_p,   v_q, v_ipp, v_ipl,  v_it, v_epp, v_epl,  v_et, v_bpm,   v_v,   v_mv, v_alarm, v_ie, plus other stuff\n");
}

 
