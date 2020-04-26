/**************************************************************************/
/*!
  @file YGKMVrun.cpp

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
    @brief Operate the ventilator. Call run() at the top of the loop.
    @param reset Reinitialize all parameters if true
    @return integer status code
*/
/**************************************************************************/
int YGKMV::run(bool reset){
  // use unsigned long for millis() values, but int for short times so positive/negative differences calculate correctly
  static unsigned long lastPrint = 0;           // set to millis() when the last output was sent to display port
  static unsigned long lastConsole = 0;         // set to millis() when the last output was sent to Serial
  static unsigned long lastCommand = 0;         // set to millis() when the last Command input was received
  static int perBreath = PB_DEF;                // the number of ms per timed breath, updated at the start of every breath
  static unsigned long startBreath = 0;         // set to millis() at the beginning of each breath
  // All these times are set to zero at the beginning of each breath, then set to a millis() value as the breath progresses
  static unsigned long startInspiration = 0;    // set to millis() during the first loop of inspiration
  static unsigned long endInspiration = 0;      // set to millis() during every loop of inspiration, including the last one
  static unsigned long startExpiration = 0;     // set to millis() during the first loop of expiration
  static unsigned long endExpiration = 0;       // set to millis() during every loop of expiration, including the last one
  static unsigned long endBreath = 0;           // set to millis() value that is projected for the end of the breath, unless triggered sooner
  static int phaseTime = 0;                     // time in phase [ms] signed with flow direction, positive for inspiration, negative for expiration, never reset to 0
  static bool startedInspiration = false;       // set false at start of breath, then true once we have inspiration at pressure
  static bool stoppedInspiration = false;       // set false at start of breath, then true once inspiration is stopped

/*********************UPDATE MEASUREMENTS AND PARAMETERS************************/ 

  // if(millis() < 10000) delay(2000);  // force a slow loop() error on startup as a test
 
  uno.run();    // keep track of things

  // If things are running well after startup and there has been a patient change, 
  // then write the patient file, but not too often as flash has limited cycles.
  // Setting up patient parameters at power on means it will write a patient file
  // every time the power comes up and stays up for a while.
  if(!v_justStarted            // we are well started
    && !p_stopped              // we are running
    && v_lastPatChange != 0    // there is an unrecorded patient change
    && millis() - v_lastPatChange > YGKMV_STARTUP * 10  // but not recently
    ) writePatFlash();

  if(millis() > YGKMV_STARTUP && !p_stopped) v_justStarted = false; // out of startup phase

  if(millis() - v_lastStop > STOP_MAX && !p_config) setRun(); // too long a stop

  if(millis() > ALARM_HOLIDAY && v_patientSet) setRun(); // there's a patient to get back to and display is sleeping

  if (millis() - v_lastStop > ALARM_STOP && p_stopped){
      if(!v_alarmOnTime) v_alarmOnTime = millis();    // set alarm time if not already
      v_alarm = v_alarm | YGKMV_STOP_ERROR;            // set the appropriate alarm bit
  } else v_alarm = v_alarm & ~YGKMV_STOP_ERROR;        // reset the alarm bit

  if (millis() - v_lastStop > STOP_MAX - 2 * ALARM_DELAY_DISPLAY
      && p_stopped){  // back to run mode soon
      if(!v_alarmOnTime) v_alarmOnTime = millis();    // set alarm time if not already
      v_alarm = v_alarm | YGKMV_STOP_WARN;            // set the appropriate alarm bit
  } else v_alarm = v_alarm & ~YGKMV_STOP_WARN;        // reset the alarm bit

  if(loopConsole()) lastCommand = millis();           // check for console input and note time
  if (millis() - lastCommand > ALARM_DELAY_DISPLAY){  // display is incognito
      if(!v_alarmOnTime) v_alarmOnTime = millis();    // set alarm time if not already
      v_alarm = v_alarm | YGKMV_DISP_ERROR;            // set the appropriate alarm bit
  } else v_alarm = v_alarm & ~YGKMV_DISP_ERROR;        // reset the alarm bit

  if (uno.dtAvg()/1000 > ALARM_DELAY_LOOP             // check for rolling average loop() rate too slow 
    || uno.dt()/1000 > ALARM_DELAY_LOOP * 5){         // be more tolerant of one slow loop()
      if(!v_alarmOnTime) v_alarmOnTime = millis();    // set alarm time if not already
      v_alarm = v_alarm | YGKMV_SLOW_ERROR;            // set the appropriate alarm bit
  } else v_alarm = v_alarm & ~YGKMV_SLOW_ERROR;        // reset the alarm bit

  v_tauW = min(1.,(double) uno.dt() / p_tau / 1000000.);   // weight to give the latest reading of a in smoothing

  // Measure current state
  v_batv = (uno.getV(aPins[BATTERY]) - offset[BATTERY]) * scale[BATTERY];   // Battery voltage from a voltage divider circuit
  v_p = getP();                         // Current pressure to the patient in cm H2O
  v_q = getQ();                         // Current flow rate to the patient in litres per minute
  v_o2 = 0.21;                          // No sensor, so assume it is room air

  
/********************NEW BREATH?********************************/   
  // check if it is time for the next breath of inspiration!
  endBreath = startBreath + perBreath;  // scheduled end of the current breath
  if( endBreath - millis() > 60000      // if we are past the projected end of the scheduled breath
      || v_etr >= p_et                  // or we have been on expiration too long
          // or we have a pressure and are below inspiration trigger and it's enabled
      || ((v_p > 1.0) && (v_p < p_epl - p_eplTol) && p_trigEnabled 
          && startedInspiration && v_etr > p_etl) // and we have been inspiring and expiring long enough
      ) {

    // Record Pressures from last breath and reset accumulated min/max values for next breath    
    v_pp = v_pmax; v_pl = v_pmin; v_pmax = 0; v_pmin = 99.99;
    v_ipp = v_ipmax; v_ipl = v_ipmin; v_ipmax = 0; v_ipmin = 99.99;
    v_epp = v_epmax; v_epl = v_epmin; v_epmax = 0; v_epmin = 99.99;

/**************UGLY UGLY FIX LATER**************************/
//v_epl = v_pl; v_ipp = v_pp;

    // Record Times for last breath, and rest rolling times
    v_it = v_itr;  v_itr = 0;   // store times for last breath and reset rolling times
    v_et = v_etr;  v_etr = 0;
    startBreath = millis();       // start a new breath
    startedInspiration = false;   // hasn't had a good breath yet
    stoppedInspiration = false;   // hasn't gone over pressure or finished yet
    startInspiration = millis();
    endInspiration = startExpiration = endExpiration = 0;  // reset times 
    perBreath = p_it + p_et;    // update perBreath at start of each breath
    if(v_it + v_et > 0) 
      v_bpm = 60000. / (v_it + v_et); // set from actual times of last breath
    else v_bpm = 0;                   // set to zero if times are stupid

    // Record Volumes for last breath, and reset rolling volume
    v_v  = v_vr;   v_vr = 0;  // restart rolling estimate
    double mv = v_v * v_bpm / 1000.; // the latest minute volume
    double w = 0.5;
    if(v_mv > 0) v_mv = w * mv + (1-w) * v_mv;    // smoothed minute ventilation
    else v_mv = mv;
    if(v_bpms > 0 && v_bpms < 10000) v_bpms = w * v_bpm + (1-w) * v_bpms;    // smoothed bpm
    else v_bpms = v_bpm;

    // test for v_it, v_et error conditions
    if (v_it < p_itl){                              // inspiration time is too short
      if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
      v_alarm = v_alarm | YGKMV_ITS_ERROR;           // set the appropriate alarm bit
    } else v_alarm = v_alarm & ~YGKMV_ITS_ERROR;     // reset the alarm bit
    if (v_it > p_ith){                              // inspiration time is too long
      if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
      v_alarm = v_alarm | YGKMV_ITL_ERROR;           // set the appropriate alarm bit
    } else v_alarm = v_alarm & ~YGKMV_ITL_ERROR;     // reset the alarm bit
    if (v_et < p_etl){                              // expiration time is too short
      if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
      v_alarm = v_alarm | YGKMV_ETS_ERROR;           // set the appropriate alarm bit
    } else v_alarm = v_alarm & ~YGKMV_ETS_ERROR;     // reset the alarm bit
    if (v_et > p_eth){                              // expiration time is too long
      if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
      v_alarm = v_alarm | YGKMV_ETL_ERROR;           // set the appropriate alarm bit
    } else v_alarm = v_alarm & ~YGKMV_ETL_ERROR;     // reset the alarm bit
   }
  // progress through the breath sequence from 0 to 1.0 on the timed sequence
//  double prog = (millis() - startBreath) / (double) perBreath;
  prog = (millis() - startBreath) / (double) perBreath;
  prog = max(prog,0.0); prog = min(prog,1.0);

/**************************ALL PHASEs of BREATH********************************/  
  v_pmax = max(v_p,v_pmax);
  v_pmin = min(v_p,v_pmin);
  if ((v_p > 1.0)               // we have a pressure 
    && (v_p > p_iph - p_iphTol) // and are above expiration trigger 
    && p_trigEnabled            // and triggering is enabled
    && v_itr > p_itl)           // and we have been in inspiration phase long enough to trigger
    stoppedInspiration = true;
  if (v_itr > p_it)             // normal time limit is up
    stoppedInspiration = true;  
  // Count all positive flow, regardless of phase
  double newVol = v_q * 1000. / 60.;  // convert to ml/s
  newVol *= uno.dt() / 1000000.;      // dt is in microseconds since last time through
  if(newVol > 0) v_vr += newVol;

/**************************INSPIRATION PHASE********************************/  
  if (!stoppedInspiration) {    // inspiration until we stop
    if(phaseTime < 0){  // We just switched over from expiration
      startInspiration = millis(); 
      v_ie = 0;
    }
    phaseTime = millis() - startInspiration;
    if(phaseTime > IT_MIN) startedInspiration = true;   // we could stop now
    fracPEEP = 0;
    fracCPAP = 1.0;
    fracDual = 1.0;
    blowerSpeed = BLOWER_MID;
    endInspiration = millis();
    if(phaseTime > eiTime){   // no longer in transition phase
      v_ie = 1;
      v_ipmax = max(v_p,v_ipmax);
      v_ipmin = min(v_p,v_ipmin);
      if (v_p < p_ipl){                               // pressure is too low
        if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
        v_alarm = v_alarm | YGKMV_IPL_ERROR;           //set the appropriate alarm bit
      } else v_alarm = v_alarm & ~YGKMV_IPL_ERROR;     //reset the alarm bit
      if (v_p > p_iph){                               // pressure is too high
        if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
        v_alarm = v_alarm | YGKMV_IPH_ERROR;           //set the appropriate alarm bit
      } else v_alarm = v_alarm & ~YGKMV_IPH_ERROR;     //reset the alarm bit
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
    blowerSpeed = BLOWER_MIN;
    endExpiration = millis();
    if(phaseTime < -ieTime){ // no longer in transition phase
      v_ie = -1;
      v_epmax = max(v_p,v_epmax);
      v_epmin = min(v_p,v_epmin);
      if (v_p < p_epl){                               // pressure is too low
        if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
        v_alarm = v_alarm | YGKMV_EPL_ERROR;           //set the appropriate alarm bit
      } else v_alarm = v_alarm & ~YGKMV_EPL_ERROR;     //reset the alarm bit
      if (v_p > p_eph){                               // pressure is too high
        if(!v_alarmOnTime) v_alarmOnTime = millis();  // set alarm time if not already
        v_alarm = v_alarm | YGKMV_EPH_ERROR;           //set the appropriate alarm bit
      } else v_alarm = v_alarm & ~YGKMV_EPH_ERROR;     //reset the alarm bit
    }
    v_etr = -phaseTime;
  }

/***********************CPAP VALVE CLOSED BY DISPLAY UNIT*******************/
  if(p_closeCPAP){    // force the CPAP closed
    fracCPAP = 0.0;
    fracPEEP = 1.0;
    fracDual = -1.0;
    v_ie = 0;
  }
/***********************ALL VALVES OPENED BY DISPLAY UNIT*******************/
  if(p_openAll){
    fracCPAP = 1.0;
    fracPEEP = 1.0;
    fracDual = 0.0;
    v_ie = 0;
  }

/***TRANSLATE TO SERVO POSITIONS AND CHECK, THEN WRITE SERVOS AND BLOWER*****/  
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
  analogWrite(BLOWER_SPEED_PIN,blowerSpeed);


/***************************RESPOND TO BUTTON(S)*******************************/  
  loopButtons();

/*****************************RESPOND TO ALARM CONDITIONS********************/
  if(!v_alarm){
    if(v_alarmOnTime){    // cancel an alarm that has recovered
      v_alarmOffTime = millis();
      v_alarmOnTime = 0;
    }
  }
  // start the timing for the noises again if the alarm hasn't reset
  if (v_alarm && v_alarmOnTime < millis() - ALARM_AUTO_REPEAT) v_alarmOnTime = millis();
  if (v_alarm & YGKMV_BUZ_ERROR  // there's an alarm on condition code that requires buzzer sounding 
      && millis() > v_alarmOnTime + ALARM_DELAY   // that has lasted longer than the delay
      && millis() < v_alarmOnTime + ALARM_LENGTH + ALARM_DELAY // and hasn't run out of time
      && millis() > ALARM_HOLIDAY       // the display has had time to wake up
    ) digitalWrite(aPins[ALARM],HIGH);
  else{
    digitalWrite(aPins[ALARM],LOW);
  }

/***********************SEND DATA TO CONSOLE / PLOTTER / DISPLAY UNIT**************/  
  loopOut();

  return status();
  }

/**************************************************************************/
/*!
    @brief Handle pushbutton input
    @param none
    @return none
*/
/**************************************************************************/
void YGKMV::loopButtons(){
  static unsigned long lastButton = 0;    // set to millis() at the end of the last button press
  static double pDelta = 1;
  static double epDelta = 1;
  if(digitalRead(BUTTON_PIN) == LOW) setRun();
  
  if (millis()-lastButton > 500){
    lastButton = millis();
    if(digitalRead(BUTTON_PIN) == LOW) {  // main / only button
      /************** turn on plotter mode  ***************************/
      p_plotterMode = true;

      /************** Decrease the peak pressure **********************/
      p_iph = min(p_iph,24);
//      if(p_iph > 20) pDelta = -abs(pDelta);
//      if(p_iph < 10) pDelta = abs(pDelta);
//      p_iph += pDelta;
      /************** Increase the min pressure **********************/
//      if(p_epl > 12) epDelta = -abs(epDelta);
//      if(p_epl < 4) epDelta = abs(epDelta);
//      p_epl += epDelta;
      
      /******************* toggle enable / disable triggering ****************/
      if (p_trigEnabled) p_trigEnabled = false; // toggle trigger mode
      else p_trigEnabled = true;

      /******************* enable triggering ****************/
      p_trigEnabled = true;

      /******************* reset alarm conditions to turn off buzzer *******/
      v_alarm = YGKMV_NO_ERROR;  // reset alarm conditions
      v_alarmOffTime = millis();
      v_alarmOnTime = 0;
      p_alarm = false;
    }
  }  
}

/**************************************************************************/
/*!
    @brief Handle output
    @param none
    @return none
*/
/**************************************************************************/
void YGKMV::loopOut()
{
  // use unsigned long for millis() values, but int for short times so positive/negative differences calculate correctly
  static unsigned long lastPrint = 0;           // set to millis() when the last output was sent to display
  static unsigned long lastConsole = 0;         // set to millis() when the last output was sent to Serial

/***********************SEND DATA TO CONSOLE / PLOTTER / DISPLAY UNIT**************/  
  if (millis()-lastPrint > OUTPUT_INTERVAL) {  // 50 ms for 20 Hz
    lastPrint = millis();
    char sc[MAX_COMMAND_LENGTH] = {0};
    sprintf(sc, "%10lu, %5.3f, %5.2f, %5.2f, %5.2f", millis(), prog, fracCPAP, fracPEEP, fracDual);
    sprintf(sc, "%s, %5.3f, %5.2f, %5.1f", sc, v_o2, v_p, v_q);
    sprintf(sc, "%s, %5.2f, %5.2f, %5u", sc, v_ipp, v_ipl, v_it);
    sprintf(sc, "%s, %5.2f, %5.2f, %5u", sc, v_epp, v_epl, v_et);
    sprintf(sc, "%s, %5.2f, %5.2f, %5.2f, %lu", sc, v_bpm, v_v, v_mv, v_alarm);
    sprintf(sc, "%s, %2d", sc, v_ie);
    sprintf(sc, "%s, %5.2f, %5.2f", sc, v_pp, v_pl);
    sprintf(sc, "%s, %5.2f", sc, v_batv); // could be added on the end
    sprintf(sc, "%s\n", sc);
    if(display) display->print(sc);
    if(p_printConsole){
      lastConsole = millis();
      if(p_plotterMode){
        PL("pSet, Pressure[cmH2O], Phase, v_q, v_vr/100");
        if(fracDual > 0) P(p_iph); else P(p_epl);
        PCS(v_p);    // use with Serial plotter to visualize the pressure output
//        PCS(p_iph - p_iphTol);
//        PCS(p_epl + p_eplTol);
//        PCS(v_itr/1000.);
//        PCS(v_etr/1000.);
        PCS(v_ie + 10);
        PCS(v_q);
        PCS(v_vr/100);
//        PCS(v_mv);
//        PCS(v_bpms);
        PL();
      } else PR(sc);   // print the whole string to the console
    }
  }
}
