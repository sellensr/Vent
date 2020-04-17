/**************************************************************************/
/*!
  @file YGKMVflash.cpp

  @section intro Introduction

// Largely copied from the SdFat_full_usage example of the Adafruit_SPIFlash library.

  @subsection author Author

  Written by Rick Sellens.

  @subsection license License

  MIT license
*/
/**************************************************************************/
#include "YGKMV.h"
    // Configuration of the flash chip pins and flash fatfs object.
    // You don't normally need to change these if using a Feather/Metro
    // M0 express board.
    
    #if defined(__SAMD51__) || defined(NRF52840_XXAA)
      Adafruit_FlashTransport_QSPI flashTransport(PIN_QSPI_SCK, PIN_QSPI_CS, PIN_QSPI_IO0, PIN_QSPI_IO1, PIN_QSPI_IO2, PIN_QSPI_IO3);
    #else
      #if (SPI_INTERFACES_COUNT == 1 || defined(ADAFRUIT_CIRCUITPLAYGROUND_M0))
        Adafruit_FlashTransport_SPI flashTransport(SS, &SPI);
      #else
        Adafruit_FlashTransport_SPI flashTransport(SS1, &SPI1);
      #endif
    #endif
    
    Adafruit_SPIFlash flash(&flashTransport);
    
    // file system object from SdFat
    FatFileSystem fatfs;

/**************************************************************************/
/*!
    @brief Sets up the flash file system and checks for a calibration file.
            Creates the /vent directory if it doesn't already exist.
            Creates the calibration file cal.txt from current values if one
            doesn't already exist. Does not create a patient file automatically
    @param none
    @return negative error code, 0 for nothing interesting, or a positive
            value indicating what files were found besides the calibration file. 
*/
/**************************************************************************/
int YGKMV::setupFlash(){
//  Serial.println("Checking the Flash for a Calibration File");
  int ret = 0;
  // Initialize flash library and check its chip ID.
  if (!flash.begin()) {
    Serial.println("Error, failed to initialize flash chip!");
    return -1;
  }
//  Serial.print("Non-unique Flash chip JEDEC ID: 0x"); Serial.println(flash.getJEDECID(), HEX);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!fatfs.begin(&flash)) {
    Serial.println("Error, failed to mount newly formatted filesystem!");
    Serial.println("Was the flash chip formatted with the SdFat_format example?");
    return -2;
  }
//  Serial.println("Mounted filesystem!");

  // Check if a directory called 'vent' exists and create it if not there.
  // Note you should _not_ add a trailing slash (like '/vent/') to directory names!
  // You can use the same exists function to check for the existance of a file too.
  if (!fatfs.exists("/vent")) {
    Serial.println("Vent directory not found, creating...");
    
    // Use mkdir to create directory (note you should _not_ have a trailing slash).
    fatfs.mkdir("/vent");
    
    if ( !fatfs.exists("/vent") ) {
      Serial.println("Error, failed to create directory!");
      return -3;
    }else {
      Serial.println("Created directory!");
    }
  }
  
  if (!fatfs.exists("/vent/cal.txt")) {
    return -4;  // ventilator is not calibrated --  don't write a cal file
//    Serial.println("Calibration file not found, creating one using default values...");
//    String cmd = "C";
//    doConsoleCommand(cmd);
//    cmd = "S";
//    doConsoleCommand(cmd);
//    ret += writeCalFlash();
  } else {
    Serial.println("Calibration file found, reading in the saved values...");
    ret += readCalFlash();
  }
  if(!ret){ // still OK, so check the patient file
    if (fatfs.exists("/vent/patient.txt")) {
      Serial.println("Patient file found, reading in the saved values...");
      int rp = readPatFlash();
      if(rp) return rp;
      else ret += 1;
    } else Serial.println("Patient file not found...");  
  }
  return ret;  
}

/**************************************************************************/
/*!
    @brief Write the calibration file cal.txt from current values.
    @param none
    @return negative error code, 0 for success. 
*/
/**************************************************************************/
int YGKMV::writeCalFlash(){
  delCalFlash();   // delete the old file
  // Create a calibration file in the vent directory and write data to it.
  File writeFile = fatfs.open("/vent/cal.txt", FILE_WRITE);
  if (!writeFile) {
    Serial.println("Error, failed to open cal.txt for writing!");
    return -8;
  }
  Serial.println("Opened file /vent/cal.txt for writing/appending...");
  char sc[MAX_COMMAND_LENGTH] = {0};
  // write a calibration constants line
  sprintf(sc,"C%7.4f,%7.4f,%7.4f,%7.2f,%7.2f,%7.2f\n",
    offset[PATIENT], offset[CPAP], offset[PEEP],
    scale[PATIENT],  scale[CPAP],  scale[PEEP]);
  writeFile.print(sc);
  PR(sc);
  // write a servo angles line
  sprintf(sc,"S%d,%d,%d,%d,%d,%d\n", aMinCPAP, aMaxCPAP, aMinPEEP, aMaxPEEP, aCloseCPAP, aClosePEEP);
  writeFile.print(sc);
  PR(sc);
  // write a model / serial numbers line
  sprintf(sc,"M%d,%d\n", p_modelNumber, p_serialNumber);
  writeFile.print(sc);
  PR(sc);
  // Close the file when finished writing.
  writeFile.close();
  v_calFile = true;
  Serial.println("Wrote to file /vent/cal.txt!");
  return 0;
}

/**************************************************************************/
/*!
    @brief Read the calibration file cal.txt and overwrite current values.
    @param none
    @return negative error code, 0 for success. 
*/
/**************************************************************************/
int YGKMV::readCalFlash(){
  // Read in values from the calibration file if it exists
  File readFile = fatfs.open("/vent/cal.txt", FILE_READ);
  if (!readFile){
    P("Could not open /vent/cal.txt for reading.\n"); 
    return -16;
  }
  P("Reading and executing lines from calibration file.\n");
  String line = "";
  line = readFile.readStringUntil('\n');
  while(line != ""){
    P("From File: "); PL(line);
    doConsoleCommand(line);
    line = readFile.readStringUntil('\n');
  }
  return 0;
}

/**************************************************************************/
/*!
    @brief Delete the calibration file cal.txt from flash.
    @param none
    @return none 
*/
/**************************************************************************/
void YGKMV::delCalFlash(){
  // Delete the calibration file
  fatfs.remove("/vent/cal.txt");
}

/**************************************************************************/
/*!
    @brief Delete the calibration file cal.txt from flash and overwrite
    existing calibration values.
    @param none
    @return none 
*/
/**************************************************************************/
void YGKMV::wipeCalFlash(){
  // Delete the calibration file
  fatfs.remove("/vent/cal.txt");
  // Restore the starting values
  scale[PATIENT] = 1.0;
  offset[PATIENT] = 0.0;
  scale[CPAP] = 1.0;
  offset[CPAP] = 0.0;
  scale[PEEP] = 1.0;
  offset[PEEP] = 0.0;
  
}

/**************************************************************************/
/*!
    @brief Write the patient file patient.txt from current values.
    @param none
    @return negative error code, 0 for success. 
*/
/**************************************************************************/
int YGKMV::writePatFlash(){
  delPatFlash();   // delete the old file
  File writeFile = fatfs.open("/vent/patient.txt", FILE_WRITE);
  if (!writeFile) {
    Serial.println("Error, failed to open patient.txt for writing!");
    return -8;
  }
  Serial.println("Opened file /vent/patient.txt for writing/appending...");
  char sc[MAX_COMMAND_LENGTH] = {0};
  // write an inspiration pressure line
  sprintf(sc,"I%7.2f,%7.2f,%7.2f\n", p_iph, p_ipl, p_iphTol);
  writeFile.print(sc);
  PR(sc);
  // write an expiration pressure line
  sprintf(sc,"E%7.2f,%7.2f,%7.2f\n", p_eph, p_epl, p_eplTol);
  writeFile.print(sc);
  PR(sc);
  // write an inspiration time line
  sprintf(sc,"i%d,%d,%d\n", p_it, p_ith, p_itl);
  writeFile.print(sc);
  PR(sc);
  // write an expiration time line
  sprintf(sc,"e%d,%d,%d\n", p_et, p_eth, p_etl);
  writeFile.print(sc);
  PR(sc);
  // write a Triggering setting line
  int i = -1;
  if(p_trigEnabled) i = 1; 
  sprintf(sc,"T%d\n", i);
  writeFile.print(sc);
  PR(sc);
  v_lastPatChange = 0;
  // Close the file when finished writing.
  writeFile.close();
  Serial.println("Wrote to file /vent/patient.txt!");
  return 0;
}

/**************************************************************************/
/*!
    @brief Read the calibration file cal.txt and overwrite current values.
    @param none
    @return negative error code, 0 for success. 
*/
/**************************************************************************/
int YGKMV::readPatFlash(){
  File readFile = fatfs.open("/vent/patient.txt", FILE_READ);
  if (!readFile){
    P("Could not open /vent/patient.txt for reading.\n"); 
    return -16;
  }
  P("Reading and executing lines from patient file.\n");
  String line = "";
  line = readFile.readStringUntil('\n');
  while(line != ""){
    P("From File: "); PL(line);
    doConsoleCommand(line);
    line = readFile.readStringUntil('\n');
  }
  v_lastPatChange = 0;  // don't need to update values just read from flash
  return 0;
}

/**************************************************************************/
/*!
    @brief Delete the calibration file cal.txt from flash.
    @param none
    @return none 
*/
/**************************************************************************/
void YGKMV::delPatFlash(){
  fatfs.remove("/vent/patient.txt");
}

/**************************************************************************/
/*!
    @brief Delete the patient file patient.txt from flash and overwrite
    existing calibration values.
    @param none
    @return none 
*/
/**************************************************************************/
void YGKMV::wipePatFlash(){
  fatfs.remove("/vent/patient.txt");
  // Restore the starting values
  p_iph = IP_MAX;        ///< the inspiration pressure upper bound.
  p_ipl = 0.0;           ///< the inspiration pressure lower bound -- PEEP setting, no action
  p_iphTol = 0.5;        ///< difference from p_eph required to trigger start of expiration if P > p_iph - p_iphTol or alarm if beyond
  p_eph = EP_MAX;        ///< the expiration pressure upper bound, no action
  p_epl = 0.0;           ///< the expiration pressure lower bound -- PEEP setting.
  p_eplTol = 2.0;        ///< difference from p_epl required to trigger start of new breath if P < p_epl - p_eplTol or alarm if beyond
  p_it = PB_DEF * INF_DEF;  ///< inspiration time setting, high/low limits
  p_ith = IT_MAX;
  p_itl = IT_MIN;
  p_et = PB_DEF - p_it;     ///< expiration time setting, high/low limits
  p_eth = ET_MAX;
  p_etl = ET_MIN;
  p_trigEnabled = false;   ///< enable triggering on pressure limits
}
