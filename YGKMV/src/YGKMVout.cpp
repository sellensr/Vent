/**************************************************************************/
/*!
  @file YGKMVout.cpp

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
    @brief 
    @param none
    @return none
*/
/**************************************************************************/
void YGKMV::showVoltages(int n, bool setOffsets)
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
  int i = aPins[PATIENT] - A0;
  if(setOffsets) offset[PATIENT] = vs[i];
  P("\n    Pressure on A"); P(i); P(": "); P(vs[i],4); 
  P(" ("); P(v[i],4); P(")  ");
  i = aPins[CPAP] - A0;
  if(setOffsets) offset[CPAP] = vs[i];
  P("CPAP Flow on A"); P(i); P(": "); P(vs[i],4); 
  P(" ("); P(v[i],4); P(")  ");
  i = aPins[PEEP] - A0;
  if(setOffsets) offset[PEEP] = vs[i];
  P("PEEP Flow on A"); P(i); P(": "); P(vs[i],4); 
  P(" ("); P(v[i],4); P(")  ");
  P("\n");
}

void YGKMV::showFlows(int n)
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
