/**************************************************************************/
/*!
    @brief Handle pushbutton input
    @param none
    @return none
*/
/**************************************************************************/
void loopButtons(){
  static unsigned long lastButton = 0;    // set to millis() at the end of the last button press
  static double pDelta = 1;
  static double epDelta = 1;
  
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
      if(p_epl > 12) epDelta = -abs(epDelta);
      if(p_epl < 4) epDelta = abs(epDelta);
      p_epl += epDelta;
      
      /******************* toggle enable / disable triggering ****************/
      if (p_trigEnabled) p_trigEnabled = false; // toggle trigger mode
      else p_trigEnabled = true;

      /******************* enable triggering ****************/
      p_trigEnabled = true;

      /******************* reset alarm conditions to turn off buzzer *******/
      v_alarm = VENT_NO_ERROR;  // reset alarm conditions
      v_alarmOffTime = millis();
      v_alarmOnTime = 0;
      p_alarm = false;
    }
  }  
}
