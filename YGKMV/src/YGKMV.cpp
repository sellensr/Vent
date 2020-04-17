/**************************************************************************/
/*!
  @file YGKMV.cpp

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
    @brief Construction and Initialization of YGKMV System for communication
            with a display unit and / or other console on a serial connection.
    @param model  Hardware model to be initialized. Model 3 is the prototype
                  shipped on 2020-04-15.
    @param disp   Serial connection for the display unit or NULL if the display
                  unit will use the USB Serial port.
    @param spd    Baud rate for Serial connections.
*/
/**************************************************************************/    
YGKMV::YGKMV(int model, HardwareSerial *disp, unsigned long spd) {
  unsigned long baud = spd;             // set the serial port speed
  if (spd == 0) baud = 115200;          // default to 115200
  baud = max(300UL,baud);               // limits from IDE 1.8.9 monitor
  baud = min(2000000UL,baud);
  displaySpeed = consoleSpeed = baud;   // same speed setting for both
  display = disp;
  // change hardware setting to match models other than 3
  
  }
  
YGKMV::~YGKMV() {}

/**************************************************************************/
/*!
    @brief Initialization of YGKMV System for communication with a display 
            unit and / or other console on a serial connection.
    @return ventilator state
*/
/**************************************************************************/
int YGKMV::begin(){
  uno.begin(consoleSpeed);
  if(display){ 
    display->begin(displaySpeed);
    while(!*display && millis() < 5000);
  }
  servoCPAP.attach(dPins[CPAP]);   ///< actuates CPAP valve only 11
  servoPEEP.attach(dPins[PEEP]);   ///< actuates PEEP valve only 10
  servoDual.attach(dPins[DUAL]);   ///< actuates both valve bodies alternately 9
  PR("\n\nYGK Modular Ventilator\n\nFirmware Library Version: "); PL(YGKMV_VERSION); 
  int fl = setupFlash();   // reads all the calibration data, hardware model and serial numbers
  if(fl > 0) v_patientSet = true;  ///< a patient data file was found
  if(fl < 0){ ///< no calibration file was found
    v_calFile = false;  
  } else v_calFile = true;
  PR("Hardware Model: "); PR(p_modelNumber); PR("    Serial Number: "); PR(p_serialNumber);
  PR("\n\n");
  setupP();
  setupQ();
  servoDual.write(aMid);
  servoPEEP.write(aMaxPEEP); 
  servoCPAP.write(aMaxCPAP);  

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BLUE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(YELLOW_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
  pinMode(aPins[ALARM], OUTPUT);
  digitalWrite(aPins[ALARM], HIGH);
  delay(5); ///< just long enough to make a little squeak so you know it is waking up
  digitalWrite(aPins[ALARM], LOW);
  delay(100); 
  if(!p_plotterMode){
    PR("Serial lines of CSV data at 115200 baud\n");
    PR("  millis(),  prog,  CPAP,  PEEP,  Dual,  v_o2,   v_p,   v_q, v_ipp, v_ipl,  v_it, v_epp, v_epl,  v_et, v_bpm,   v_v,   v_mv, v_alarm, v_ie, plus other stuff\n");
  }
  if(display){
    display->print("Serial lines of CSV data at 115200 baud\n");
    display->print("  millis(),  prog,  CPAP,  PEEP,  Dual,  v_o2,   v_p,   v_q, v_ipp, v_ipl,  v_it, v_epp, v_epl,  v_et, v_bpm,   v_v,   v_mv, v_alarm, v_ie, plus other stuff\n");
  }
  return status();
}

/**************************************************************************/
/*!
    @brief Check
    @param 
    @return status value
*/
/**************************************************************************/
int YGKMV::status(){
  return 0;
  }

/**************************************************************************/
/*!
    @brief Go to Run mode by setting the appropriate state parameter flags.
    @param none
    @return none
*/
/**************************************************************************/
void YGKMV::setRun(){
    p_closeCPAP = false;
    p_openAll = false;
    p_stopped = false;
    p_config = false;
    if(v_firstRun == 0) v_firstRun = millis();
}


/**************************************************************************/
/*!
    @brief 
    @param none
    @return none
*/
/**************************************************************************/
void YGKMV::setupP(){  // do any setup required for pressure measurement
  
}
 
/**************************************************************************/
/*!
    @brief 
    @param none
    @return none
*/
/**************************************************************************/
double YGKMV::getP(){  // return the current value for patient pressure in cm H20
  double v = uno.getV(aPins[PATIENT]); // instantaneous voltage
  double w = v_tauW; // weighting factor for exponential smoothing
  v_px137v = v_px137v * (1-w) + v * w; // smoothed voltage
  double p = (v_px137v - offset[PATIENT]) * scale[PATIENT];
  return p;   
}

/**************************************************************************/
/*!
    @brief 
    @param none
    @return none
*/
/**************************************************************************/
void YGKMV::setupQ(){  // do any setup required for flow measurement
  for(int i = 0; i < 100; i++){
    v_CPAPv = uno.getV(aPins[CPAP]);  
    v_PEEPv = uno.getV(aPins[PEEP]);  
  }
}

/**************************************************************************/
/*!
    @brief 
    @param none
    @return none
*/
/**************************************************************************/
double YGKMV::getQ(){  // return the current value for patient flow in litres / minute
  double w = v_tauW; // weighting factor for exponential smoothing
  double v = uno.getV(aPins[CPAP]); // instantaneous voltage
  v_CPAPv = v_CPAPv * (1-w) + v * w; // smoothed voltage
  v = uno.getV(aPins[PEEP]); // instantaneous voltage
  v_PEEPv = v_PEEPv * (1-w) + v * w; // smoothed voltage
  v_qCPAP = (v_CPAPv - offset[CPAP]) * scale[CPAP];  // flow on the CPAP side
  v_qPEEP = (v_PEEPv - offset[PEEP]) * scale[PEEP];  // minus return flow on the PEEP side
  return v_qCPAP - v_qPEEP;
}

/**************************************************************************/
/*!
    @brief 
    @param none
    @return none
*/
/**************************************************************************/
double YGKMV::getQCPAP(){  // return the instantaneous value for CPAP side flow in litres / minute
  return (uno.getV(aPins[CPAP]) - offset[CPAP]) * scale[CPAP];  // flow on the CPAP side
}

/**************************************************************************/
/*!
    @brief 
    @param none
    @return none
*/
/**************************************************************************/
double YGKMV::getQPEEP(){  // return the instantaneous value for PEEP side flow in litres / minute
  return (uno.getV(aPins[PEEP]) - offset[PEEP]) * scale[PEEP];  // flow on the PEEP side
}
