/**************************************************************************/
/*!
  @file YGKMVcon.cpp

  @section intro Introduction

  @subsection author Author

  Written by Rick Sellens.

  @subsection license License

  MIT license
*/
/**************************************************************************/
#include "YGKMV.h"
/**************************************************************************/
/*!
    @brief Show a list of possible commands
    @param none
    @return none
*/
/**************************************************************************/
void YGKMV::listConsoleCommands() {
  P("\nApplication specific commands include:\n");
  P("  a - read and display (a)nalog voltages, averaging over n values, e.g. a10\n      Set offset values if n is less than 0, e.g. a-1\n");
  P("  A - set (A)larm condition on (positive argument),  off (negative argument),\n      or just show condition (0 argument), e.g. A-1\n");
  P("  C - set desired (C)alibration offsets and scale factors for patient pressure, CPAP flow, and PEEP flow\n      e.g. C1.2435,1.2532,1.3121,90.3,50.4,42.1\n");
  P("  D - set desired (D)amping time constant for noise reduction [s], e.g. D0.1\n");
  P("  e - set desired patient (e)xpiratory times target, high/low limits [ms], e.g. e2500,4500,1000\n");
  P("  E - set desired patient (E)xpiratory pressures high/low/trig tol [cm H2O], e.g. E28.2,6.3,1.0\n");
  P("  f - read and display (f)low values, averaging over n values, e.g. f10\n");
  P("  i - set desired patient (i)nspiratory times target, high/low limits [ms], e.g. i2000,3500,1200\n");
  P("  I - set desired patient (I)nspiratory pressures high/low/trig tol [cm H2O], e.g. I38.2,16.3,1.0\n");
  P("  M - set desired hardware (M)odel and serial numbers, e.g. M3,30000001\n");
  P("  P - set print mode, positive for plotter mode on, negative for no console output, \n        0 for plotter mode off, e.g. P1\n");
  P("  r - (r)ead in the calibration, servo angles, and other settings from the file, e.g. r\n");
  P("  R - set to normal (R)un mode, e.g. R\n");
  P("  s - set closed/open settings for CPAP and PEEP valve (S)ervos interactively, e.g. s\n");
  P("  S - set closed/open settings for CPAP and PEEP valve (S)ervos, e.g. S130,180,90,140,66,98\n");
  P("  t - set desired inspiration/expiration (t)imes [ms], e.g. t1000,2000\n");
  P("  T - set breath Triggering, positive for triggering on, negative for triggering off, e.g. T1\n");
  P("  w - (w)rite out the calibration, servo angles, and other settings to the file, e.g. w\n");
  P("  W - (W)ipe out the calibration, servo angles, and other settings and return to defaults, e.g. W99\n");
  P("  x - open all valves and enter config mode, will not auto-return to run mode, e.g. x\n");
  P("  X - close the CPAP valve and enter stop mode, will auto return to run mode after reaching a time limit, e.g. X\n");
  P("  Z - do nothing, can be sent as a heartbeat, e.g. Z");
  P("\nNormal data lines start with a numeral. All command lines received will generate at least\n");
  P("one line of text in return. A line starting with ACK indicates a recognized command was received\n");
  P("and acted on, as described in the remainder of the line. It does not necessarily mean values were\n");
  P("changed, as some or all may have been outside permitted limits. A line starting with NOACK\n");
  P("indicates an unrecognized line was received. Other human readable lines can be ignored.\n\n");
}

/**************************************************************************/
/*!
    @brief Handle console input/output functions on USB and on Serial1 for
            display unit.
    @param none
    @return true if a command line was received, false otherwise
*/
/**************************************************************************/
bool YGKMV::loopConsole(){
  static String ci = "";
  bool ret = false;
  if (readConsoleCommand(
          &ci)) { // returns false quickly if there has been no EOL yet
    ret = true;
    P("\nFrom Console:");
    PL(ci);
    // or just send the whole String to one of these functions for parsing and
    // action
    if (!doConsoleCommand(ci)) {
      P("NOACK Not an application specific command: ");
      PL(ci);
      listConsoleCommands();
    }
    ci = ""; // don't call readConsoleCommand() again until you have cleared the
             // string
  }

  if(display){   // ignore display if it doesn't exist
    // exactly the same, except for commands input from Serial1 to ci1
    static String ci1 = "";
    if (readDisplayCommand(&ci1)) {
      ret = true;
      P("\nFrom Display Unit:");
      PL(ci1);
      // or just send the whole String to one of these functions for parsing and
      // action
      if (!doConsoleCommand(ci1)) {
        P("NOACK Not an application specific command: ");
        PL(ci1);
        listConsoleCommands();
      }
      ci1 = ""; // don't call readConsoleCommand() again until you have cleared the
               // string
    }
  }
  return ret;
}

/**************************************************************************/
/*!
    @brief Respond to console commands
    @param cmd a String containing the command
    @return true if successful, false if we couldn't respond properly
*/
/**************************************************************************/
boolean YGKMV::doConsoleCommand(String cmd) {
  // an application specific version that acts the same as the library function
  boolean ret = false;
  float val[10] = {0};
  char c = parseConsoleCommand(cmd, val, 10);
  String s = cmd.substring(1); // the whole string after the command letter
  s.trim();                    // trimmed of white space
  int ival = val[0];           // an integer version of the first float arg
  unsigned long logPeriod;
  int n = 1, pDelta = 1;
  int wr = 0;
  switch (c) {
  case 'a': // show analog voltages
    if(!p_stopped){ 
      P("ACK You must be stopped to run this command! Use X or x to enter stop mode.\n");
      ret = true;
      break;
    }
    P("ACK Calling showVoltages().\n");
    if(val[0] < 1) n = 10000;
    n = min(n,10000);
    if(val[0] < 0) showVoltages(n, true);
    else showVoltages(n, false);
    ret = true;
    break;
  case 'A': // alarm condition
    if (val[0] > 0){ 
      p_alarm = true;
      if(!v_alarmOnTime) v_alarmOnTime = millis();
      v_alarm += YGKMV_EXT_ERROR;
     }
    if (val[0] < 0){ 
      p_alarm = false;
      v_alarmOffTime = millis();
      v_alarmOnTime = 0;
      v_alarm = YGKMV_NO_ERROR;
    }
    P("ACK Alarm set to: ");
    if(p_alarm) P("True, code: ");
    else P("False, code: ");
    PL(v_alarm);
    ret = true;
    break;
  case 'C': // Calibration Values
    if (val[0] != 0) offset[PATIENT] = val[0];
    if (val[1] != 0) offset[CPAP] = val[1];
    if (val[2] != 0) offset[PEEP] = val[2];
    if (val[3] != 0) scale[PATIENT] = val[3];
    if (val[4] != 0) scale[CPAP] = val[4];
    if (val[5] != 0) scale[PEEP] = val[5];
    P("ACK Offsets and Scales set to\n");
    P("     Pressure: "); P(offset[PATIENT],4);     P("V / "); P(scale[PATIENT]);      P(" cmH2O / V\n"); 
    P("    CPAP Flow: "); P(offset[CPAP],4); P("V / "); P(scale[CPAP]);  P(" lpm / V\n");
    P("    PEEP Flow: "); P(offset[PEEP],4); P("V / "); P(scale[PEEP]);  P(" lpm / V\n");
    ret = true;
    break;
  case 'D': // Damping time constant
    if (val[0] > 0) p_tau = min(val[0],1.0);
    P("ACK Damping time constant set to: ");
    P(p_tau,3); P(" seconds\n");
    ret = true;
    break;
  case 'e': // Expiratory Times
    if (val[0] >= ET_MIN) p_et = min(val[0], ET_MAX);
    if (val[1] >= ET_MIN) p_eth = min(val[1], ET_MAX);
    if (val[2] >= ET_MIN) p_etl = min(val[2], ET_MAX);
    P("ACK Expiration Times set to: ");
    P(p_et); P(" target "); P(p_eth);  P(" / "); P(p_etl); P(" high/low ms\n");
    if(p_patFlashEnabled) writePatFlash();
    ret = true;
    break;
  case 'E': // Expiratory Pressures
    if (val[0] >= EP_MIN) p_eph = min(val[0], EP_MAX);
    if (val[1] >= EP_MIN) p_epl = min(val[1], EP_MAX);
    if (abs(val[2]) >= 0) p_eplTol = min(abs(val[2]), EPLTOL_MAX);
    P("ACK Expiration Pressures set to: ");
    P(p_eph); P(" / "); P(p_epl);  P(" / "); P(p_eplTol); P(" cm H2O\n");
    if(p_patFlashEnabled) writePatFlash();
    ret = true;
    break;
  case 'f': // show flow values
    if(!p_stopped){ 
      P("ACK You must be stopped to run this command! Use X or x to enter stop mode.\n");
      ret = true;
      break;
    }
    if(val[0] < 1) n = 10000;
    n = min(n,10000);
    showFlows(n);
    ret = true;
    break;
  case 'i': // Inspiratory Times
    if (val[0] >= IT_MIN) p_it = min(val[0], IT_MAX);
    if (val[1] >= IT_MIN) p_ith = min(val[1], IT_MAX);
    if (val[2] >= IT_MIN) p_itl = min(val[2], IT_MAX);
    P("ACK Inspiration Times set to: ");
    P(p_it); P(" target "); P(p_ith);  P(" / "); P(p_itl); P(" high/low ms\n");
    if(p_patFlashEnabled) writePatFlash();
    ret = true;
    break;
  case 'I': // Inspiratory Pressures
    if (val[0] >= IP_MIN) p_iph = min(val[0], IP_MAX);
    if (val[1] >= IP_MIN) p_ipl = min(val[1], IP_MAX);
    if (val[2] >= 0) p_iphTol = min(val[2], 5);
    P("ACK Inspiration Pressures set to: ");
    P(p_iph); P(" / "); P(p_ipl);  P(" / "); P(p_iphTol); P(" cm H2O\n");
    if(p_patFlashEnabled) writePatFlash();
    ret = true;
    break;
  case 'M': // Model / Serial numbers
    if (val[0] > 0 && val[0] < 100){ 
      p_modelNumber = val[0];
      p_serialNumber = p_serialNumber % 10000000 + 10000000 * p_modelNumber;
    }
    n = val[1];
    n = n % 10000000;
    if (n > 0) p_serialNumber = 10000000 * p_modelNumber + n;
    PR("ACK Model / Serial Numbers set to: ");
    PR(p_modelNumber); PR(" / "); PL(p_serialNumber);
    PR("YGK Modular Ventilator Library Version: "); PL(YGKMV_VERSION); 
    ret = true;
    break;
  case 'P': // plotter mode
    if (val[0] > 0){
      p_plotterMode = true;
      p_printConsole = true;
    }
    if (val[0] < 0){ 
      p_plotterMode = false;
      p_printConsole = false; 
    }
    if (val[0] == 0){
      p_plotterMode = false;
      p_printConsole = true;
    }
    P("ACK Plotter Mode set to: ");
    if(p_plotterMode) P("True\n");
    else P("False\n");
    ret = true;
    break;
  case 'r': // read current calibrations
    P("ACK reading calibration settings file.\n");
    readCalFlash();
    ret = true;
    break;
  case 'R': // Run Mode -- also need to do these things at the top of the loop
//    if(p_stopped || p_closeCPAP || p_openAll) PL("ACK Taking all valves and operations to run mode.");
    PL("ACK Taking all valves and operations to run mode.");
    setRun();
//    p_closeCPAP = false;
//    p_openAll = false;
//    p_stopped = false;
//    p_config = false;
    ret = true;
    break;
  case 's': // Interactive Servo Angle Values
    if(!p_stopped){ 
      P("ACK You must be stopped to run this command! Use X or x to enter stop mode.\n");
      ret = true;
      break;
    }
    aMaxCPAP = aMaxPEEP = aMid = 90;
    aMinCPAP = aMinPEEP = aCloseCPAP = aClosePEEP = 90;
    servoDual.write(aMid);
    servoCPAP.write(aMaxCPAP);
    servoPEEP.write(aMaxPEEP);
    pDelta = 1;
    P("ACK Install valve gates in open position and hit return (enter)....\n");
    while(!Serial.available()); while(Serial.available()) Serial.read();
    while(pDelta != 0){
      P("Enter change in CPAP valve angle, or just return (enter) to set as closed position.\n");
      while(!Serial.available());
      pDelta = Serial.parseInt();
      aMinCPAP += pDelta;
      P("New CPAP Closed Position: "); PL(aMinCPAP);
      servoCPAP.write(aMinCPAP);
    }
    servoCPAP.write(aMaxCPAP);
    pDelta = 1;
    while(pDelta != 0){
      P("Enter change in PEEP valve angle, or just return (enter) to set as closed position.\n");
      while(!Serial.available());
      pDelta = Serial.parseInt();
      aMinPEEP += pDelta;
      P("New PEEP Closed Position: "); PL(aMinPEEP);
      servoPEEP.write(aMinPEEP);
    }
    servoPEEP.write(aMaxPEEP);
    pDelta = 1;
    while(pDelta != 0){
      P("Enter change in Dual valve angle, or just return (enter) to set as CPAP closed position.\n");
      while(!Serial.available());
      pDelta = Serial.parseInt();
      aCloseCPAP += pDelta;
      P("New Dual Valve, CPAP Closed Position: "); PL(aCloseCPAP);
      servoDual.write(aCloseCPAP);
    }
    servoDual.write(aMid);
    pDelta = 1;
    while(pDelta != 0){
      P("Enter change in Dual valve angle, or just return (enter) to set as PEEP closed position.\n");
      while(!Serial.available());
      pDelta = Serial.parseInt();
      aClosePEEP += pDelta;
      P("New Dual Valve, PEEP Closed Position: "); PL(aClosePEEP);
      servoDual.write(aClosePEEP);
    }
    aMid = (aCloseCPAP + aClosePEEP) / 2.0;
    servoDual.write(aMid);

    P("ACK Servo Angles set to\n");
    P("    CPAP Valve: "); P(aMinCPAP);   P(" / "); P(aMaxCPAP);  P(" degrees\n");
    P("    PEEP Valve: "); P(aMinPEEP);   P(" / "); P(aMaxPEEP);  P(" degrees\n");
    P("    Dual Valve: "); P(aCloseCPAP); P(" / "); P(aMid,0);    P(" / ");       P(aClosePEEP);  P(" degrees\n"); 
    ret = true;
    break;
  case 'S': // Servo Angle Values -- don't accept zeros!
    if (val[0] > 0) aMinCPAP = min(val[0],180.);
    if (val[1] > 0) aMaxCPAP = min(val[1],180.);
    if (val[2] > 0) aMinPEEP = min(val[2],180.);
    if (val[3] > 0) aMaxPEEP = min(val[3],180.);
    if (val[4] > 0) aCloseCPAP = min(val[4],180.);
    if (val[5] > 0) aClosePEEP = min(val[5],180.);
    aMid = (aCloseCPAP + aClosePEEP) / 2.0;
    P("ACK Servo Angles set to\n");
    P("    CPAP Valve: "); P(aMinCPAP);   P(" / "); P(aMaxCPAP);  P(" degrees\n");
    P("    PEEP Valve: "); P(aMinPEEP);   P(" / "); P(aMaxPEEP);  P(" degrees\n");
    P("    Dual Valve: "); P(aCloseCPAP); P(" / "); P(aMid,0);    P(" / ");       P(aClosePEEP);  P(" degrees\n"); 
    ret = true;
    break;
  case 't': // breath timing
    if (val[0] >= IT_MIN) p_it = min(val[0], IT_MAX);
    if (val[1] >= ET_MIN) p_et = min(val[1], ET_MAX);
    P("ACK Inspiration/Expiration Times set to: ");
    P(p_it); P(" / "); P(p_et); P(" ms\n");
    ret = true;
    break;
  case 'T': // breath triggering
    if (val[0] > 0) p_trigEnabled = true;
    if (val[0] < 0) p_trigEnabled = false;
    P("ACK Pressure Triggering set to: ");
    if(p_trigEnabled) P("True\n");
    else P("False\n");
    if(p_patFlashEnabled) writePatFlash();
    ret = true;
    break;
  case 'w': // write current calibrations
    P("ACK writing calibration settings file.\n");
    writeCalFlash();
    ret = true;
    break;
  case 'W': // wipe calibrations and return to defaults
    if (val[0] != 99){ 
      P("ACK The argument must be 99 to wipe the calibration settings!!!\n");
      ret = false;
    }
    else {
      P("ACK wiping calibration settings file and returning to pre-configuration values.\n");
      wipeCalFlash();
      ret = true;
    }
    break;
  case 'x': // open all
    v_lastStop = millis();
    p_openAll = true;
    p_closeCPAP = false;
    p_stopped = true;
    p_config = true;
    PL("ACK All valves going to open position for configuration.");
    P("Will not automatically return to (R)un mode. Use R to return.");
    ret = true;
    break;
  case 'X': // Close CPAP
    v_lastStop = millis();
    p_closeCPAP = true;
    p_openAll = false;
    p_stopped = true;
    PL("ACK CPAP valve going to closed position.");
    P("Will return to (R)un mode after "); P(STOP_MAX / 1000); PL(" seconds.");
    ret = true;
    break;
  case 'Z': // Do nothing, heartbeat
    PL("ACK Z command takes no action.");
    ret = true;
    break;
  default:
    PL("NOACK line not recognized as a command.");
    ret = false; // didn't recognize command
    break;
  }
  return ret; // true if we found a command to execute!
}

/**************************************************************************/
/*!
    @brief Read characters from the console until you have a full line. Call at
   the top of the loop().
    @param consoleIn pointer to the String to store the input line.
    @return true if we got to the end of a line, otherwise false.
*/
/**************************************************************************/
boolean YGKMV::readConsoleCommand(String *consoleIn) {
  while (Serial.available()) { // accumulate command string and return true when
                               // completed.
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if ((*consoleIn).length() != 0)
        return true; // we got to the end of the line
    } else
      *consoleIn += c;
      // If it gets to long, throw it away and make the next line a no action command
      if((*consoleIn).length() > MAX_COMMAND_LENGTH) *consoleIn = "Z";
  }
  return false;
}
/**************************************************************************/
/*!
    @brief Same as readConsoleCommand() except using Serial1
    @param consoleIn pointer to the String to store the input line.
    @return true if we got to the end of a line, otherwise false.
*/
/**************************************************************************/
boolean YGKMV::readDisplayCommand(String *consoleIn) {
  if(display){
    while (display->available()) { // accumulate command string and return true when
                                 // completed.
      char c = display->read();
      if (c == '\n' || c == '\r') {
        if ((*consoleIn).length() != 0)
          return true; // we got to the end of the line
      } else
        *consoleIn += c;
        if((*consoleIn).length() > MAX_COMMAND_LENGTH) *consoleIn = "Z";
    }
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
char YGKMV::parseConsoleCommand(String cmd, float val[], int maxVals) {
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
