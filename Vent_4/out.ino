/**************************************************************************/
/*!
    @brief Handle output
    @param none
    @return none
*/
/**************************************************************************/
void loopOut()
{
  // use unsigned long for millis() values, but int for short times so positive/negative differences calculate correctly
  static unsigned long lastPrint = 0;           // set to millis() when the last output was sent to Serial1
  static unsigned long lastConsole = 0;         // set to millis() when the last output was sent to Serial

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
    sprintf(sc, "%s, %5.2f, %5.2f", sc, v_pp, v_pl);
    sprintf(sc, "%s\n", sc);
    Serial1.print(sc);
    if(p_printConsole){
      lastConsole = millis();
      if(p_plotterMode){
        PL("pSet, Pressure[cmH2O], HighLimit, LowLimit, InspTime, ExpTime, Phase, v_q/10, v_vr/100, v_mv, v_bpms");
        if(fracDual > 0) P(p_iph); else P(p_epl);
        PCS(v_p);    // use with Serial plotter to visualize the pressure output
        PCS(p_iph - p_iphTol);
        PCS(p_epl + p_eplTol);
        PCS(v_itr/1000.);
        PCS(v_etr/1000.);
        PCS(v_ie + 10);
        PCS(v_q/10);
        PCS(v_vr/100);
        PCS(v_mv);
        PCS(v_bpms);
        PL();
      } else PR(sc);   // print the whole string to the console
    }
  }
}
