void loop()
{
  // use unsigned long for millis() values, but int for short times so positive/negative differences calculate correctly
  static unsigned long lastPrint = 0;
  static unsigned long lastButton = 0;
  static int perBreath = PB_DEF;
  static unsigned long startBreath = 0;
  static unsigned long startInspiration = 0;
  static unsigned long startExpiration = 0;
  static unsigned long endInspiration = 0;
  static unsigned long endBreath = 0;
  static double dP = 0.0;
  static double fracCPAP = 1.0; // wide open
  static double fracPEEP = 1.0; // wide open
  static double fracDual = 0.0; // halfway between, positive opens CPAP, negative opens PEEP
//  static double setPEEP = 5.0;  // PEEP threshold
  double TA = 0, PA = 0, TV = 0, PV = 0;
  static int phaseTime = 0; // time in phase [ms] signed with flow direction, positive for inspiration, negative for expiration, 0 for transition
  static bool stoppedInspiration = false;
  
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
  
  // check if it is time for the next breath of inspiration!
  endBreath = startBreath + perBreath;  // finish the current breath first
  if( endBreath - millis() > 60000      // we are past the end of expiration
      || ((v_p > 1.0) && (v_p < p_epl - p_eplTol) && p_trigEnabled)  
      // we have a pressure and are below inspiration trigger and it's enabled
      ) {
    v_it = endInspiration - startBreath;  // store times for last breath
    v_et = endBreath - endInspiration;
    startBreath = millis();       // start a new breath
    stoppedInspiration = false;    // hasn't gone over pressure yet
    perBreath = p_it + p_et;      // update perBreath at start of each breath
    v_bpm = 60000. / (v_it + v_et); // set from actual times of last breath
  }
  if ((v_p > 1.0) && (v_p > p_iph + p_iphTol) && p_trigEnabled) // we have a pressure and are above expiration trigger 
    stoppedInspiration = true;
  // progress through the breath sequence from 0 to 1.0
  double prog = (millis() - startBreath) / (double) perBreath;
  prog = max(prog,0.0); prog = min(prog,1.0);
  // calculate something more complicated for the opening
  if (millis() - startBreath < p_it && !stoppedInspiration
    ) {   // inhalation
    if(phaseTime < 0){ 
      startInspiration = millis(); 
    }
    phaseTime = millis() - startInspiration;
    fracPEEP = 0;
    fracCPAP = 1.0;
    fracDual = 1.0;
    endInspiration = millis();
  } else {            // exhalation
    if(phaseTime > 0){
      startExpiration = millis();
    } 
    phaseTime = -((int) millis() - startExpiration);
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
  if (millis()-lastButton > 500 && digitalRead(BUTTON_PIN) == LOW) {
    lastButton = millis();
    if (p_trigEnabled) p_trigEnabled = false;
    else p_trigEnabled = true;
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
//    PR(sc);   // print the whole string to the console
    Serial1.print(sc);
    PL("Valve, ie, Pressure[cmH2O], P_high, P_low, RefNull");
    P(fracDual * 2 + 10);
    PCS(phaseTime/1000.0 + 10);
    PCS(v_p);    // use with Serial plotter to visualize the pressure output
    PCS(p_iph + p_iphTol);
    PCS(p_epl - p_eplTol);
    PCS(10);
    PL();
  }
}
