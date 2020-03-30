/**************************************************************************/
/*!
  @file loop.ino

  @section intro Introduction

  An Arduino sketch for running tests on an HMRC DIY ventilator. loop tab

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
    @brief Control loop of HMRC DIY Ventilator
    @param none
    @return none
*/
/**************************************************************************/
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
  static double fracCPAP = 1.0; // wide open
  static double fracPEEP = 1.0; // wide open
  static double fracDual = 0.0; // halfway between, positive opens CPAP, negative opens PEEP
  static int phaseTime = 0; // time in phase [ms] signed with flow direction, positive for inspiration, negative for expiration, 0 for transition
  static bool stoppedInspiration = false;

/*********************UPDATE MEASUREMENTS AND PARAMETERS************************/  
  uno.run();    // keep track of things
  loopConsole();// check for console input
  // Test for loop rate
  if (uno.dtAvg() > 100000) PR("Taking longer than 100 ms per loop!\n");
  // Measure current state
  v_batv = uno.getV(A_BAT) * DIV_BAT;
  v_venturiv = uno.getV(A_VENTURI);
  v_p = getP();
  v_q = getQ();

  
/********************NEW BREATH?********************************/   
  // check if it is time for the next breath of inspiration!
  endBreath = startBreath + perBreath;  // finish the current breath first
  if( endBreath - millis() > 60000      // we are past the end of expiration
      || ((v_p > 1.0) && (v_p < p_epl + p_eplTol) && p_trigEnabled && (millis() - startBreath > 250))  
      // we have a pressure and are below inspiration trigger and it's enabled
      ) {
    v_ipp = v_ipmax; v_ipl = v_ipmin; v_ipmax = 0; v_ipmin = 99.99;
    v_epp = v_epmax; v_epl = v_epmin; v_epmax = 0; v_epmin = 99.99;
    v_it = v_itr;  v_itr = 0; // store times for last breath
    v_et = v_etr;  v_etr = 0;
    startBreath = millis();       // start a new breath
    stoppedInspiration = false;    // hasn't gone over pressure yet
    perBreath = p_it + p_et;      // update perBreath at start of each breath
    v_bpm = 60000. / (v_it + v_et); // set from actual times of last breath
    // test for v_it, v_et error conditions
    if (v_it < p_itl){                              // inspiration time is too short
      if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
      v_alarm = v_alarm | VENT_ITS_ERROR;           // set the appropriate alarm bit
    } else v_alarm = v_alarm & ~VENT_ITS_ERROR;     // reset the alarm bit
    if (v_it > p_ith){                              // inspiration time is too long
      if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
      v_alarm = v_alarm | VENT_ITL_ERROR;           // set the appropriate alarm bit
    } else v_alarm = v_alarm & ~VENT_ITL_ERROR;     // reset the alarm bit
    if (v_et < p_etl){                              // expiration time is too short
      if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
      v_alarm = v_alarm | VENT_ETS_ERROR;           // set the appropriate alarm bit
    } else v_alarm = v_alarm & ~VENT_ETS_ERROR;     // reset the alarm bit
    if (v_et > p_eth){                              // expiration time is too long
      if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
      v_alarm = v_alarm | VENT_ETL_ERROR;           // set the appropriate alarm bit
    } else v_alarm = v_alarm & ~VENT_ETL_ERROR;     // reset the alarm bit
   }
  // progress through the breath sequence from 0 to 1.0 on the timed sequence
  double prog = (millis() - startBreath) / (double) perBreath;
  prog = max(prog,0.0); prog = min(prog,1.0);

/**************************INSPIRATION PHASE********************************/  
  if ((v_p > 1.0) && (v_p > p_iph - p_iphTol) && p_trigEnabled) // we have a pressure and are above expiration trigger 
    stoppedInspiration = true;
  if (v_itr > p_it) stoppedInspiration = true;  // time's up
  if (millis() - startBreath < p_it && !stoppedInspiration) {   // inhalation
    if(phaseTime < 0){ 
      startInspiration = millis(); 
      v_ie = 0;
    }
    phaseTime = millis() - startInspiration;
    fracPEEP = 0;
    fracCPAP = 1.0;
    fracDual = 1.0;
    endInspiration = millis();
    if(phaseTime > eiTime){   // no longer in transition phase
      v_ie = 1;
      v_ipmax = max(v_p,v_ipmax);
      v_ipmin = min(v_p,v_ipmin);
      if (v_p < p_ipl){                               // pressure is too low
        if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
        v_alarm = v_alarm | VENT_IPL_ERROR;           //set the appropriate alarm bit
      } else v_alarm = v_alarm & ~VENT_IPL_ERROR;     //reset the alarm bit
      if (v_p > p_iph){                               // pressure is too high
        if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
        v_alarm = v_alarm | VENT_IPH_ERROR;           //set the appropriate alarm bit
      } else v_alarm = v_alarm & ~VENT_IPH_ERROR;     //reset the alarm bit
    }
    v_itr = phaseTime;
  } 
  
/**************************EXPIRATION PHASE********************************/  
  else {            // exhalation
    if(phaseTime > 0){
      startExpiration = millis();
      v_ie = 0;
    } 
    phaseTime = -((int) millis() - startExpiration);
    fracPEEP = 1.0;
    fracCPAP = 0.0;
    fracDual = -1.0;
    if(phaseTime < -ieTime){ // no longer in transition phase
      v_ie = -1;
      v_epmax = max(v_p,v_epmax);
      v_epmin = min(v_p,v_epmin);
      if (v_p < p_epl){                               // pressure is too low
        if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
        v_alarm = v_alarm | VENT_EPL_ERROR;           //set the appropriate alarm bit
      } else v_alarm = v_alarm & ~VENT_EPL_ERROR;     //reset the alarm bit
      if (v_p > p_eph){                               // pressure is too high
        if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
        v_alarm = v_alarm | VENT_EPH_ERROR;           //set the appropriate alarm bit
      } else v_alarm = v_alarm & ~VENT_EPH_ERROR;     //reset the alarm bit
    }
    v_etr = -phaseTime;
  }

/***********************CPAP VALVE CLOSED BY DISPLAY UNIT*******************/
  if(p_closeCPAP){    // force the CPAP closed
    fracCPAP = 0.0;
    fracDual = -1.0;
    v_ie = 0;
  }

/************************TRANSLATE TO SERVO POSITIONS AND CHECK, THEN WRITE*****/  
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

/***************************RESPOND TO BUTTON(S)*******************************/  
  if (millis()-lastButton > 500 && digitalRead(BUTTON_PIN) == LOW) {
    lastButton = millis();
    if (p_trigEnabled) p_trigEnabled = false; // toggle trigger mode
    else p_trigEnabled = true;
    v_alarm = VENT_NO_ERROR;  // reset alarm conditions
    v_alarmOffTime = millis();
    v_alarmOnTime = 0;
    p_alarm = false;
  }

/*****************************UPDATE VALUES FROM CURRENT STATE****************/
  double osc = sin(2 * 3.14159 * prog);
  v_o2 = 0.21 + osc *0.01;
  v_mv = v_v * v_bpm;

/*****************************RESPOND TO ALARM CONDITIONS********************/
  if(!v_alarm){
    if(v_alarmOnTime){    // cancel an alarm that has recovered
      v_alarmOffTime = millis();
      v_alarmOnTime = 0;
    }
  }
  if (v_alarm   // there's an alarm on condition code 
      && millis() > v_alarmOnTime + ALARM_DELAY   // that has lasted longer than the delay
      && millis() < v_alarmOnTime + ALARM_LENGTH + ALARM_DELAY // and hasn't run out of time
    ) digitalWrite(ALARM_PIN,HIGH);
  else{
    digitalWrite(ALARM_PIN,LOW);
  }

/***********************SEND DATA TO CONSOLE / PLOTTER / DISPLAY UNIT**************/  
  if (millis()-lastPrint > 50 * slowPrint) {  // 50 ms for 20 Hz, or slowed down for debug
    lastPrint = millis();
    char sc[200] = {0};
    sprintf(sc, "%10lu, %5.3f, %5.2f, %5.2f, %5.2f", millis(), prog, fracCPAP, fracPEEP, fracDual);
    sprintf(sc, "%s, %5.3f, %5.2f, %5.1f", sc, v_o2, v_p, v_q);
    sprintf(sc, "%s, %5.2f, %5.2f, %5u", sc, v_ipp, v_ipl, v_it);
    sprintf(sc, "%s, %5.2f, %5.2f, %5u", sc, v_epp, v_epl, v_et);
    sprintf(sc, "%s, %5.2f, %5.2f, %5.2f, %lu", sc, v_bpm, v_v, v_mv, v_alarm);
    sprintf(sc, "%s, %2d", sc, v_ie);
    sprintf(sc, "%s\n", sc);
    Serial1.print(sc);
    if(plotterMode){
      PL("pSet, Pressure[cmH2O], HighLimit, LowLimit, InspTime, ExpTime, Phase");
      if(fracDual > 0) P(p_iph); else P(p_epl);
      PCS(v_p);    // use with Serial plotter to visualize the pressure output
      PCS(p_iph - p_iphTol);
      PCS(p_epl + p_eplTol);
      PCS(v_itr/1000.);
      PCS(v_etr/1000.);
      PCS(v_ie + 10);
      PL();
    } else PR(sc);   // print the whole string to the console
  }
}
