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
  if (millis()-lastPrint > 50 * slowPrint && !p_stopped) {  // 50 ms for 20 Hz, or slowed down for debug
    lastPrint = millis();
    char sc[200] = {0};
    sprintf(sc, "%10lu, %5.3f, %5.2f, %5.2f, %5.2f", millis(), prog, fracCPAP, fracPEEP, fracDual);
    sprintf(sc, "%s, %5.3f, %5.2f, %5.1f", sc, v_o2, v_p, v_q);
    sprintf(sc, "%s, %5.2f, %5.2f, %5u", sc, v_ipp, v_ipl, v_it);
    sprintf(sc, "%s, %5.2f, %5.2f, %5u", sc, v_epp, v_epl, v_et);
    sprintf(sc, "%s, %5.2f, %5.2f, %5.2f, %lu", sc, v_bpm, v_v, v_mv, v_alarm);
    sprintf(sc, "%s, %2d", sc, v_ie);
    sprintf(sc, "%s, %5.2f, %5.2f", sc, v_pp, v_pl);
    sprintf(sc, "%s, %5.2f", sc, v_batv); // could be added on the end
    sprintf(sc, "%s\n", sc);
    Serial1.print(sc);
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

void showVoltages(int n, bool setOffsets)
{
  double v[6] = {0};
  double vs[6] = {0};
  for(int j = 0; j < n; j++){
    for(int i = 0; i < 6; i++){
      v[i] = uno.getV(i);
      vs[i] += v[i];
    }
  }
  for(int i = 0; i < 6; i++) vs[i] /= n;
  P("Voltages: Averaged (Instantaneous) ");
  if(setOffsets) P(" (Setting Offsets!) ");
  int i = A_PX137 - A0;
  if(setOffsets) p_pOffset = vs[i];
  P("\n    Pressure on A"); P(i); P(": "); P(vs[i],4); 
  P(" ("); P(v[i],4); P(")  ");
  i = A_CAP_CPAP - A0;
  if(setOffsets) p_qOffsetCPAP = vs[i];
  P("CPAP Flow on A"); P(i); P(": "); P(vs[i],4); 
  P(" ("); P(v[i],4); P(")  ");
  i = A_CAP_PEEP - A0;
  if(setOffsets) p_qOffsetPEEP = vs[i];
  P("PEEP Flow on A"); P(i); P(": "); P(vs[i],4); 
  P(" ("); P(v[i],4); P(")  ");
  P("\n");
}

void showFlows(int n)
{
  double ps = 0, qcs = 0, qps = 0;
  for(int j = 0; j < n; j++){
    ps += getP();
    qcs += getQCPAP();
    qps += getQPEEP();
  }
  ps /= n;
  qcs /= n;
  qps /= n;
  P("\n    Patient Pressure [cm H2O]: "); P(ps,6); 
  P("\n     CPAP Flow [litres / min]: "); P(qcs,6); 
  P("\n     PEEP Flow [litres / min]: "); P(qps,6); 
  P("\n");
}
