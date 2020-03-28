#include "RWS_UNO.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Servo.h>

RWS_UNO uno = RWS_UNO();
#define SEALEVELPRESSURE_HPA (1013.25)
#define PB_DEF 6000    // breathing rate defaults to 10
#define PB_MAX 10000
#define PB_MIN 2500
#define INF_DEF 0.25    // first 40% of breath is inspiration
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
int aCloseCPAP = 110, aClosePEEP = 77;  // dual valve position settings for fully closed [servo degrees]
double aMid = (aCloseCPAP + aClosePEEP) / 2.0;

bool readPBME = false;    // set false to allow testing on a machine with no BMEs or to use other P sensors instead

// Variables from UI definition + a bit more
double v_o2 = 0.0;    // measured instantaneous oxygen volume fraction [0 to 1.0]
double v_p = 99.9;     // current instantaneous pressure [cm H2O]
double v_q = 99.9;      // current instantaneous flow to patient [l/min]
double v_ipp = 99.9;    // highest pressure during inspiration phase of last breath [cm H2o]
double v_ipl = 99.9;    // lowest pressure during inspiration phase of last breath [cm H2O]
int v_it = 0;      // inspiration time during last breath [ms]
double v_epp = 99.9;    // highest pressure during expiration phase of last breath [cm H2o]
double v_epl = 99.9;    // lowest pressure during expiration phase of last breath [cm H2O]
int v_et = 0;       // expiration time during last breath [ms]
double v_bpm = 0.0;     // BPM for last breath
double v_v = 0.0;       // inspiration volume of last breath [ml]
double v_mv = 0.0;      // volume per minute averaged over recent breaths [l / min]
byte v_alarm = 0;       // status code
double v_batv = 0.;     // measured battery voltage, should be over 13 for powered, over 12 for charge remaining
double v_venturiv = 0.;     // measured venturi voltage

double p_iph = 20.0;     // the inspiration pressure upper bound.
double p_ipl = 6.0;     // the inspiration pressure lower bound -- PEEP setting.
double p_iphTol = 1.0;  // excursion from p_eph required to trigger start of expiration if P > p_iph + p_iphTol
double p_eph = 20.0;     // the expiration pressure upper bound.
double p_epl = 6.0;     // the expiration pressure lower bound -- PEEP setting.
double p_eplTol = 1.0;  // excursion from p_epl required to trigger start of new breath if P < p_epl - p_eplTol
int p_it = PB_DEF * INF_DEF;  // inspiration time setting
int p_et = PB_DEF - p_it;     // expiration time setting

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
  PR("Serial lines of CSV data at 115200 baud\n");
  PR("  millis(),  prog,  CPAP,  PEEP,  Dual,  v_o2,   v_p,   v_q, v_ipp, v_ipl,  v_it, v_epp, v_epl,  v_et, v_bpm,   v_v,   v_mv, v_alarm, plus other stuff\n");
  Serial1.print("Serial lines of CSV data at 115200 baud\n");
  Serial1.print("  millis(),  prog,  CPAP,  PEEP,  Dual,  v_o2,   v_p,   v_q, v_ipp, v_ipl,  v_it, v_epp, v_epl,  v_et, v_bpm,   v_v,   v_mv, v_alarm, plus other stuff\n");
}

 
void loop()
{
  // use unsigned long for millis() values, but int for short times so positive/negative differences calculate correctly
  static unsigned long lastPrint = 0;
  static unsigned long lastButton = 0;
  static int perBreath = PB_DEF;
  static unsigned long startBreath = 0;
  static unsigned long endInspiration = 0;
  static unsigned long endBreath = 0;
  static double dP = 0.0;
  static double fracCPAP = 1.0; // wide open
  static double fracPEEP = 1.0; // wide open
  static double fracDual = 0.0; // halfway between, positive opens CPAP, negative opens PEEP
  static double setPEEP = 5.0;  // PEEP threshold
  double TA = 0, PA = 0, TV = 0, PV = 0;

  uno.run();    // keep track of things
  loopConsole();// check for console input
  // Test for loop rate
  if (uno.dtAvg() > 100000) PR("Taking longer than 100 ms per loop!\n");
  // Measure current state
  if(readPBME){
    TA = bmeA.readTemperature();
    PA = bmeA.readPressure(); // Pa
    TV = bmeV.readTemperature();
    PV = bmeV.readPressure(); // Pa
  }
  v_batv = uno.getV(A_BAT) * DIV_BAT;
  v_venturiv = uno.getV(A_VENTURI);
  dP = dP * 0.9 + (PV - PA - 0.0) * 0.1; 
  v_p = dP * 100 / 998 / 9.81;    // cm H2O
  
  // check if it is time for the next breath
  endBreath = startBreath + perBreath;  // finish the current breath first
  if( endBreath - millis() > 60000 ||   // we are past the end of expiration
      v_p < p_epl - p_eplTol            // we are below inspiration trigger
      ) {
    v_it = endInspiration - startBreath;  // store times for last breath
    v_et = endBreath - endInspiration;
    startBreath = millis();       // start a new breath
    perBreath = p_it + p_et;      // update perBreath at start of each breath
    v_bpm = 60000. / (v_it + v_et); // set from actual times of last breath
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
  if (millis()-lastPrint > 50) {
    lastPrint = millis();
    char sc[200] = {0};
    sprintf(sc, "%10lu, %5.3f, %5.2f, %5.2f, %5.2f", millis(), prog, fracCPAP, fracPEEP, fracDual);
    sprintf(sc, "%s, %5.3f, %5.2f, %5.1f", sc, v_o2, v_p, v_q);
    sprintf(sc, "%s, %5.2f, %5.2f, %5u", sc, v_ipp, v_ipl, v_it);
    sprintf(sc, "%s, %5.2f, %5.2f, %5u", sc, v_epp, v_epl, v_et);
    sprintf(sc, "%s, %5.2f, %5.2f, %5.2f, %5u", sc, v_bpm, v_v, v_mv, v_alarm);
    sprintf(sc, "%s\n", sc);
//    PR(sc);
    Serial1.print(sc);
    // Don't remove any of this stuff from the output line.
//    PR(millis());
//    PCS(prog,3); PCS(fracCPAP); PCS(fracPEEP); PCS(fracDual);
//    PCS(v_o2); 
    PL(v_p); 
//    PCS(v_q);
//    PCS(v_ipp); PCS(v_ipl); PCS(v_it);
//    PCS(v_epp); PCS(v_epl); PCS(v_et);
//    PCS(v_bpm); PCS(v_v,0); PCS(v_mv,0); PCS(v_alarm);
    // print other debug stuff after this required data
//    PCS(startBreath); PCS(endInspiration); PCS(endBreath); 
  //  PCS(PV); PCS(dP); 
  //  PCS(fracCPAP); PCS(fracPEEP);
  //  PCS(posCPAP); PCS(posPEEP); PCS(posDual);
//    PL();
  }
}

void loopConsole(){
    // check for lines of user input from the console
  static String ci = "";
  if (readConsoleCommand(
          &ci)) { // returns false quickly if there has been no EOL yet
    P("\nFrom Console:");
    PL(ci);
    // or just send the whole String to one of these functions for parsing and
    // action
    if (!doConsoleCommand(ci)) {
      P("Not an application specific command: ");
      PL(ci);
      listConsoleCommands();
    }
    ci = ""; // don't call readConsoleCommand() again until you have cleared the
             // string
  }

  // exactly the same, except for commands input from Serial1 to ci1
  static String ci1 = "";
  if (readConsoleCommand1(&ci1)) {
    P("\nFrom Serial1:");
    PL(ci1);
    // or just send the whole String to one of these functions for parsing and
    // action
    if (!doConsoleCommand(ci1)) {
      P("Not an application specific command: ");
      PL(ci1);
      listConsoleCommands();
    }
    ci1 = ""; // don't call readConsoleCommand() again until you have cleared the
             // string
  }


}

boolean doConsoleCommand(String cmd) {
  // an application specific version that acts the same as the library function
  boolean ret = false;
  float val[10] = {0};
  char c = parseConsoleCommand(cmd, val, 10);
  String s = cmd.substring(1); // the whole string after the command letter
  s.trim();                    // trimmed of white space
  int ival = val[0];           // an integer version of the first float arg
  unsigned long logPeriod;
  switch (c) {
  case 'E': // Expiratory Pressures
    if (val[0] >= 0) p_eph = min(val[0], 50);
    if (val[1] >= 0) p_epl = min(val[1], 20);
    if (val[2] >= 0) p_eplTol = min(val[2], 5);
    P("Inspiration Pressures set to: ");
    P(p_eph); P(" / "); P(p_epl);  P(" / "); P(p_eplTol); P(" cm H2O\n");
    ret = true;
    break;
  case 'I': // Inspiratory Pressures
    if (val[0] >= 0) p_iph = min(val[0], 50);
    if (val[1] >= 0) p_ipl = min(val[1], 20);
    if (val[2] >= 0) p_iphTol = min(val[2], 5);
    P("Inspiration Pressures set to: ");
    P(p_iph); P(" / "); P(p_ipl);  P(" / "); P(p_iphTol); P(" cm H2O\n");
    ret = true;
    break;
  case 't': // breath timing
    if (val[0] >= IT_MIN) p_it = min(val[0], IT_MAX);
    if (val[1] >= ET_MIN) p_et = min(val[1], ET_MAX);
    P("Inspiration/Expiration Times set to: ");
    P(p_it); P(" / "); P(p_et); P(" ms\n");
    ret = true;
    break;
  case 'x': // an application command
    PL("The x/X command just prints this message back to the console");
    ret = true;
    break;
  default:
    ret = false; // didn't recognize command
    break;
  }
  return ret; // true if we found a command to execute!
}

void listConsoleCommands() {
  P("\nApplication specific commands include:\n");
  P("  E - set desired patient (E)xpiratory pressures high/low/trig tol [cm H2O], e.g. E28.2,6.3,1.0\n");
  P("  I - set desired patient (I)nspiratory pressures high/low/trig tol [cm H2O], e.g. I38.2,16.3,1.0\n");
  P("  t - set desired inspiration/expiration (t)imes [ms], e.g. t1000,2000\n");
  P("  x - print an (x) message\n");
}

/**************************************************************************/
/*!
    @brief Read characters from the console until you have a full line. Call at
   the top of the loop().
    @param consoleIn pointer to the String to store the input line.
    @return true if we got to the end of a line, otherwise false.
*/
/**************************************************************************/
boolean readConsoleCommand(String *consoleIn) {
  while (Serial.available()) { // accumulate command string and return true when
                               // completed.
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if ((*consoleIn).length() != 0)
        return true; // we got to the end of the line
    } else
      *consoleIn += c;
  }
  return false;
}
boolean readConsoleCommand1(String *consoleIn) {
  while (Serial1.available()) { // accumulate command string and return true when
                               // completed.
    char c = Serial1.read();
    if (c == '\n' || c == '\r') {
      if ((*consoleIn).length() != 0)
        return true; // we got to the end of the line
    } else
      *consoleIn += c;
  }
  return false;
}

/**************************************************************************/
/*!
    @brief Parse the command into an array of floats that follow the one letter
   code. Returns the first character of the String "X1.3,5.4,6.5" and puts the
   floats in val[]. No guarantees of val[i] result if there isn't a valid float
   there.
    @param cmd String from readConsoleCommand()
    @param val An Array to store numerical values
    @param maxVals Maximum number of numerical values to store.
    @return the command letter
*/
/**************************************************************************/
char parseConsoleCommand(String cmd, float val[], int maxVals) {
  int comma[maxVals]; // find the commas, and there better not be more than
                      // maxVals!
  cmd.trim();
  comma[0] = cmd.indexOf(',');
  for (int i = 1; i < maxVals; i++) {
    val[i] = 0.0;
    comma[i] = 1 + comma[i - 1] + cmd.substring(comma[i - 1] + 1).indexOf(',');
  }
  val[0] = (cmd.substring(1, comma[0]))
               .toFloat(); // extract the values between the commas
  for (int i = 1; i < maxVals; i++) {
    if (comma[i] == comma[i - 1]) { // this is the last one so break out
      val[i] = (cmd.substring(comma[i - 1] + 1)).toFloat();
      break;
    } else
      val[i] = (cmd.substring(comma[i - 1] + 1, comma[i])).toFloat();
  }
  return cmd.charAt(0);
}
