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
  static unsigned long lastPrint = 0;
  uno.run();
  double w = 0.001;
  double v[6] = {0};
  static double vs[6] = {0};
  for(int i = 0; i < 6; i++){
    v[i] = uno.getV(i);
    vs[i] = (1-w)*vs[i] + w*v[i];
  }
  if(millis() - lastPrint > 50){
    lastPrint = millis();
    P("Voltages: Smoothed (Instantaneous) ");
    for(int i = 0; i < 6; i++){
      P("A"); P(i); P(": "); P(vs[i],4); 
      P(" ("); P(v[i],4); P(")  ");
    }
    P("\n");
  }
}
