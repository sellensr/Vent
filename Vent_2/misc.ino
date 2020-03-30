/**************************************************************************/
/*!
  @file misc.ino

  @section intro Introduction

  An Arduino sketch for running tests on an HMRC DIY ventilator. misc tab

  @section author Author

  Written by

  @section license License

  CCBY license

  Code provided only for example purposes. Must not be used for any purpose 
  critical to life or health, or protection of property.
*/
/**************************************************************************/

/**************************************************************************/
/*!
    @brief Handle console input/output functions on USB and on Serial1 for
            display unit.
    @param none
    @return none
*/
/**************************************************************************/
void loopConsole(){
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

/**************************************************************************/
/*!
    @brief Respond to console commands
    @param cmd a String containing the command
    @return true if successful, false if we couldn't respond properly
*/
/**************************************************************************/
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
  case 'A': // alarm condition
    if (val[0] > 0){ 
      p_alarm = true;
      if(!v_alarmOnTime) v_alarmOnTime = millis();
      v_alarm += VENT_EXT_ERROR;
     }
    if (val[0] < 0){ 
      p_alarm = false;
      v_alarmOffTime = millis();
      v_alarmOnTime = 0;
      v_alarm = VENT_NO_ERROR;
    }
    P("Alarm set to: ");
    if(p_alarm) P("True, code: ");
    else P("False, code: ");
    PL(v_alarm);
    ret = true;
    break;
  case 'e': // Expiratory Times
    if (val[0] >= ET_MIN) p_et = min(val[0], ET_MAX);
    if (val[1] >= ET_MIN) p_eth = min(val[1], ET_MAX);
    if (val[2] >= ET_MIN) p_etl = min(val[2], ET_MAX);
    P("Expiration Times set to: ");
    P(p_et); P(" target "); P(p_eth);  P(" / "); P(p_etl); P(" high/low ms\n");
    ret = true;
    break;
  case 'E': // Expiratory Pressures
    if (val[0] >= EP_MIN) p_eph = min(val[0], EP_MAX);
    if (val[1] >= EP_MIN) p_epl = min(val[1], EP_MAX);
    if (val[2] >= 0) p_eplTol = min(val[2], 5);
    P("Expiration Pressures set to: ");
    P(p_eph); P(" / "); P(p_epl);  P(" / "); P(p_eplTol); P(" cm H2O\n");
    ret = true;
    break;
  case 'i': // Inspiratory Times
    if (val[0] >= IT_MIN) p_it = min(val[0], IT_MAX);
    if (val[1] >= IT_MIN) p_ith = min(val[1], IT_MAX);
    if (val[2] >= IT_MIN) p_itl = min(val[2], IT_MAX);
    P("Inspiration Times set to: ");
    P(p_it); P(" target "); P(p_ith);  P(" / "); P(p_itl); P(" high/low ms\n");
    ret = true;
    break;
  case 'I': // Inspiratory Pressures
    if (val[0] >= IP_MIN) p_iph = min(val[0], IP_MAX);
    if (val[1] >= IP_MIN) p_ipl = min(val[1], IP_MAX);
    if (val[2] >= 0) p_iphTol = min(val[2], 5);
    P("Inspiration Pressures set to: ");
    P(p_iph); P(" / "); P(p_ipl);  P(" / "); P(p_iphTol); P(" cm H2O\n");
    ret = true;
    break;
  case 'P': // plotter mode
    if (val[0] > 0) p_plotterMode = true;
    if (val[0] < 0) p_plotterMode = false;
    P("Plotter Mode set to: ");
    if(p_plotterMode) P("True\n");
    else P("False\n");
    ret = true;
    break;
  case 'R': // Run Mode
    p_closeCPAP = false;
    PL("Taking CPAP valve to run mode.");
    ret = true;
    break;
  case 't': // breath timing
    if (val[0] >= IT_MIN) p_it = min(val[0], IT_MAX);
    if (val[1] >= ET_MIN) p_et = min(val[1], ET_MAX);
    P("Inspiration/Expiration Times set to: ");
    P(p_it); P(" / "); P(p_et); P(" ms\n");
    ret = true;
    break;
  case 'T': // breath triggering
    if (val[0] > 0) p_trigEnabled = true;
    if (val[0] < 0) p_trigEnabled = false;
    P("Pressure Triggering set to: ");
    if(p_trigEnabled) P("True\n");
    else P("False\n");
    ret = true;
    break;
  case 'X': // Close CPAP
    p_closeCPAP = true;
    PL("CPAP valve going to closed position.");
    ret = true;
    break;
  default:
    ret = false; // didn't recognize command
    break;
  }
  return ret; // true if we found a command to execute!
}

/**************************************************************************/
/*!
    @brief Show a list of possible commands
    @param none
    @return none
*/
/**************************************************************************/
void listConsoleCommands() {
  P("\nApplication specific commands include:\n");
  P("  e - set desired patient (e)xpiratory times target, high/low limits [ms], e.g. e2500,4500,1000\n");
  P("  E - set desired patient (E)xpiratory pressures high/low/trig tol [cm H2O], e.g. E28.2,6.3,1.0\n");
  P("  i - set desired patient (i)nspiratory times target, high/low limits [ms], e.g. i2000,3500,1200\n");
  P("  I - set desired patient (I)nspiratory pressures high/low/trig tol [cm H2O], e.g. I38.2,16.3,1.0\n");
  P("  P - set plotter mode, positive for plotter mode on, negative for plotter mode off, e.g. P1\n");
  P("  R - set to normal (R)un mode, e.g. R\n");
  P("  t - set desired inspiration/expiration (t)imes [ms], e.g. t1000,2000\n");
  P("  T - set breath Triggering, positive for triggering on, negative for triggering off, e.g. T1\n");
  P("  X - close the CPAP valve, e.g. X\n");
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
/**************************************************************************/
/*!
    @brief Same as readConsoleCommand() except using Serial1
    @param consoleIn pointer to the String to store the input line.
    @return true if we got to the end of a line, otherwise false.
*/
/**************************************************************************/
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
