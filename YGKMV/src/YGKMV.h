/**************************************************************************/
/*!
  @file YGKMV.h
*/
#ifndef _YGKMV_h  // avoid including multiple times
#define _YGKMV_h

#define YGKMV_VERSION "0.5.0"  // release / code freeze, e.g. 4 for Vent_4 / update
#include "RWS_UNO.h"    // https://github.com/sellensr/RWS_UNO
#include <Servo.h>
#include <SPI.h>
#include <SdFat.h>                // https://github.com/adafruit/SdFat
#include <Adafruit_SPIFlash.h>    // https://github.com/adafruit/Adafruit_SPIFlash

#define CPAP    0 ///< index number for the CPAP servo or flow pressure
#define PEEP    1 ///< index number for the PEEP servo or flow pressure
#define DUAL    2 ///< index number for the Dual servo
#define PATIENT 2 ///< index number for the patient pressure
#define BUTTON  3 ///< index number for the primary blue button
#define BATTERY 3 ///< index number for the battery analog voltage
#define ALARM   4 ///< index number for the alarm

// stackable error codes that will fit into v_alarm
#define YGKMV_NO_ERROR   0b0                 ///<     0 There is no Error
#define YGKMV_IPL_ERROR  0b0000000000000001  ///<     1 Inspiration Pressure Low < p_ipl
#define YGKMV_IPH_ERROR  0b0000000000000010  ///<     2 Inspiration Pressure High > p_iph
#define YGKMV_ITS_ERROR  0b0000000000000100  ///<     4 Inspiration Time Short < p_itl
#define YGKMV_ITL_ERROR  0b0000000000001000  ///<     8 Inspiration Time Long > p_ith
#define YGKMV_EPL_ERROR  0b0000000000010000  ///<    16 Expiration Pressure Low < p_epl
#define YGKMV_EPH_ERROR  0b0000000000100000  ///<    32 Expiration Pressure High > p_eph
#define YGKMV_ETS_ERROR  0b0000000001000000  ///<    64 Expiration Time Short < p_etl
#define YGKMV_ETL_ERROR  0b0000000010000000  ///<   128 Expiration Time Long > p_eth
#define YGKMV_STOP_ERROR 0b0000100000000000  ///<  2048 We have been in stop mode longer than ALARM_STOP
#define YGKMV_STOP_WARN  0b0001000000000000  ///<  4096 Going back to run mode soon
#define YGKMV_SLOW_ERROR 0b0010000000000000  ///<  8192 Loop is not executing in under ALARM_DELAY_LOOP
#define YGKMV_DISP_ERROR 0b0100000000000000  ///< 16384 Display/Console Incognito longer than ALARM_DELAY_DISPLAY
#define YGKMV_EXT_ERROR  0b1000000000000000  ///< 32768 External Error
#define YGKMV_BUZ_ERROR  0b1111100000000000  ///< Only make a local buzzer noise for these error states

#define BLUE_BUTTON_PIN     12  ///< pin with blue button pulled low when pushed
#define YELLOW_BUTTON_PIN    3  ///< pin with yellow button pulled low when pushed
#define RED_BUTTON_PIN       4  ///< pin with red button pulled low when pushed
#define BLOWER_SPEED_PIN    A0  ///< analogWrite() between about 350 and 750 for speed
#define BLOWER_MIN         350
#define BLOWER_MID         550
#define BLOWER_MAX         750         
#define BLOWER_SWITCH_PIN    7  ///< set high to send power to the blower (n channel MOSFET)
#define BLOWER_GAIN        0.1  ///< proportional control gain
#define MAX_COMMAND_LENGTH 200  ///< no lines longer than this for commands or output

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
#define EPLTOL_MAX 10   ///< Max value for the tolerance

#define IQ_MAX 50       ///< Inspiration flow max [lpm]
#define EQ_MAX 50       ///< Expiration flow max [lpm]
#define IQ_MIN 0        ///< Inspiration flow min [lpm]
#define EQ_MIN 0        ///< Expiration flow min [lpm]

#define STOP_MAX 600000000L ///< Max time in stop mode, set really long to limit auto restart capability, 600000000L is about a week
#define OUTPUT_INTERVAL 50  ///< minimum [ms] between output lines

#define ALARM_DELAY         3000  ///< [ms] don't alarm until the condition has lasted this long
#define ALARM_LENGTH       10000  ///< [ms] don't make an alarm sound longer than this, set short only during debugging
#define ALARM_DELAY_DISPLAY 3000  ///< [ms] don't set the alarm condition until the display has been silent this long
#define ALARM_DELAY_LOOP     100  ///< [ms] set the alarm condition if loop is taking longer than this
#define ALARM_AUTO_REPEAT  30000  ///< [ms] reset the alarm so it sounds again after about this long 
#define ALARM_HOLIDAY      40000  ///< [ms] don't alarm if millis() is less than this, since display has not woken up
#define ALARM_STOP         30000  ///< [ms] alarm if stopped for longer than this

#define YGKMV_STARTUP      60000  ///< [ms] before we consider ourselves in normal operation

/**************************************************************************/
/*!
    @brief  The YGKMV class
*/
class YGKMV{
  public:
    YGKMV(int model = 3, HardwareSerial *disp = NULL, unsigned long spd = 115200);
    virtual ~YGKMV();
    int begin();
    int status();
    int run(bool reset = false);  ///< execute every time through the loop, run or idling
    void setRun();  ///< Switch to Run mode, breathing the ventilator
    void listConsoleCommands();
    bool loopConsole();
    boolean doConsoleCommand(String cmd);
    boolean readConsoleCommand(String *consoleIn);
    boolean readDisplayCommand(String *consoleIn);
    char parseConsoleCommand(String cmd, float val[], int maxVals);
    void showVoltages(int n = 1000, bool setOffsets = false);
    void showFlows(int n = 1000);
    void setupP();
    double getP();
    void setupQ();
    double getQ();
    double getQCPAP();
    double getQPEEP();
    int setupFlash();
    int writeCalFlash();
    int readCalFlash();
    void delCalFlash();
    void wipeCalFlash();
    int writePatFlash();
    int readPatFlash();
    void delPatFlash();
    void wipePatFlash();
    void loopButtons();
    void loopOut();
    
  private:
    RWS_UNO uno = RWS_UNO();
    Servo servoCPAP, servoPEEP, servoDual;
    int dPins[10] = {11, 10, 9, 12, 5}; ///< pins for CPAP, PEEP, Dual Servos, Button, Alarm
    // Analog Inputs / Outputs as arrays replace individual variables.
    // Offsets are in volts measured from the analog input pins.
    // Scales are in units of cmH20 / volt for PX137 patient pressure sensor
    // or (l/min) / volt for input from flow measurement elements
    int aPins[6] = {A3, A4, A5, A2};    ///< pins for CPAP, PEEP, Patient pressures, bat
    double scale[6] = {1.0, 1.0, 1.0, 5.0, 1.0, 1.0};  ///< scale factors for pressure, etc.
    double offset[6] = {1.0, 1.0, 1.0, 0.0, 1.0, 1.0}; ///< voltage offsets for pressure, etc
    int pinBlower = 5;
    int verbosity = 10;
    int model = 3;
    HardwareSerial *display = NULL;
    unsigned long displaySpeed = 115200;
    unsigned long consoleSpeed = 115200;
    int aMinCPAP = 90;    ///< CPAP valve servo degrees, closed for minimum flow
    int aMaxCPAP = 90;    ///< CPAP valve servo degrees, open for maximum flow
    int aMinPEEP = 90;    ///< PEEP valve servo degrees, closed for minimum flow
    int aMaxPEEP = 90;    ///< PEEP valve servo degrees, open for maximum flow
    int aCloseCPAP = 90;  ///< Dual valve servo degrees for CPAP closed 
    int aClosePEEP = 90;  ///< Dual valve servo degrees for PEEP closed
    int aMid = (aCloseCPAP + aClosePEEP) / 2.0; ///< servo degrees for both open

    // Detecting the time to end a phase does not instantly open valves and change flows.
    // We need to wait a little while before we say we are in the next phase.
    int ieTime = 400;   ///< transition between end of inspiration and start of expiration phase
    int eiTime = 400;   ///< transition time between end of expiration phase and start of inspiration phase

    double fracCPAP = 1.0;  ///< target opening fraction for the CPAP valve. 0 for closed, 1.0 for wide open
    double fracPEEP = 1.0;  ///< target opening fraction for the CPAP valve. 0 for closed, 1.0 for wide open
    double fracDual = 0.0;  ///< target position for Dual Valve. 0.0 for halfway between, 1.0 fully opens CPAP, -1.0 fully opens PEEP
    int blowerSpeed = BLOWER_MIN;   ///< target speed setting for the blower in analog output units
    double prog = 0;        ///< progress through the current scheduled breath
    
    // Class Global Variables from UI definition + a bit more
    // v_ for all elements that are measured or calculated from actual operations
    // Not necessarily declared in order of output to the display unit. Check the output code.
    double v_o2 = 0.0;            ///< measured instantaneous oxygen volume fraction [0 to 1.0]
    double v_p = 0.0;             ///< current instantaneous pressure [cm H2O]
    double v_pSet = 0.0;          ///< current set point pressure for controls to target [cm H2O]
    double v_q = 0.0;             ///< current instantaneous flow to patient [l/min]
    double v_qSetHigh = IQ_MAX;   ///< current set point high limit for instantaneous flow to patient [l/min]
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
    unsigned long v_alarm = 0;    ///< status code, normally YGKMV_NO_ERROR, YGKMV_EXT_ERROR if externally imposed
    double v_batv = 0.;           ///< measured battery voltage, should be over 13 for powered, over 12 for charge remaining
    double v_venturiv = 0.;       ///< measured venturi voltage
    double v_px137v = 0.;         ///< measured patient pressure voltage
    double v_CPAPv = 0.;          ///< measured CPAP side flow element voltage
    double v_PEEPv = 0.;          ///< measured PEEP side flow element voltage
    int v_ie = 0;                 ///< set to 0 when between phases, 1 for inspiration phase, -1 for expiration phase
    int v_ieEntered = 0;          ///< like v_ie, except no transition. Always set to phase entering or in, only 0 on stop
    unsigned long v_alarmOnTime = 0;    ///< time of the first alarm state that occurred since all alarms were clear
    unsigned long v_alarmOffTime = 0;   ///< time that alarms were last cleared
    unsigned long v_lastStop = 0; ///< set to millis() when the last Stop Command input was received
    unsigned long v_firstRun = 0; ///< set to millis() when the first setRun() call takes place
    unsigned long v_lastPatChange = 0; ///< set to millis() when the patient data changes, then set to zero when patient file written
    double v_tauW = 0.001;        ///< the smoothing weight factor to use this cycle for time constant p_tau
    bool v_patientSet = false;    ///< set true if a patient data file is found, or if patient parameters have been set
    bool v_justStarted = true;    ///< set true to start, then set false until power down
    bool v_calFile = false;       ///< set true if a calibration and configuration file exists
    
    // p_ for all elements that are set parameters for desired performance
    double p_iph = IP_MAX;        ///< the inspiration pressure upper bound.
    double p_ipl = 0.0;           ///< the inspiration pressure lower bound -- PEEP setting, no action
    double p_iphTol = 0.5;        ///< difference from p_eph required to trigger start of expiration if P > p_iph - p_iphTol or alarm if beyond
    double p_eph = EP_MAX;        ///< the expiration pressure upper bound, no action
    double p_epl = 0.0;           ///< the expiration pressure lower bound -- PEEP setting.
    double p_eplTol = 2.0;        ///< difference from p_epl required to trigger start of new breath if P < p_epl - p_eplTol or alarm if beyond
    int p_it = PB_DEF * INF_DEF;  ///< inspiration time setting, high/low limits
    int p_ith = IT_MAX;
    int p_itl = IT_MIN;
    int p_et = PB_DEF - p_it;     ///< expiration time setting, high/low limits
    int p_eth = ET_MAX;
    int p_etl = ET_MIN;
    bool p_trigEnabled = false;   ///< enable triggering on pressure limits
    bool p_patFlashEnabled = false; ///< set true to enable saving patient settings to flash
    bool p_patFlashWritten = false; ///< patient flash has been written at least once this cycle
    bool p_closeCPAP = true;      ///< set true to close the CPAP valve, must be set false for normal running
    bool p_openAll = false;       ///< set true to open all the valves, must be set false for normal running
    bool p_stopped = true;        ///< set true to enable menu items that could interfere with normal running
    bool p_config = false;        ///< set true to prevent return to run mode during configuration processes
    bool p_alarm = false;         ///< set true for an alarm condition imposed externally
    bool p_plotterMode = false;   ///< set true for output visualization using arduino ide plotter mode
    bool p_printConsole = true;   ///< set false to turn off console data output, notmally true
    double p_tau = 0.10;          ///< instrumentation smoothing time constant [s]
    int p_modelNumber = 3;        ///< Hardware model number, 1 was abandoned, 2 was single servo and venturi, 
                                  //   3 is single or double servo gates with flow elements in both feeds 
    int p_serialNumber = 30000001;///< Hardware serial number is model number * 10000000 + unique integer



};

#endif  // _YGKMV_h
