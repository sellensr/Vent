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
    @return none
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
    Serial.println("Calibration file not found, creating one using default values...");
    String cmd = "C";
    doConsoleCommand(cmd);
    cmd = "S";
    doConsoleCommand(cmd);
    ret += writeCalFlash();
  } else {
    Serial.println("Calibration file found, reading in the saved values...");
    ret += readCalFlash();
  }
  return ret;  
}

int YGKMV::writeCalFlash(){
  delCalFlash();   // delete the old file
  // Create a calibration file in the vent directory and write data to it.
  File writeFile = fatfs.open("/vent/cal.txt", FILE_WRITE);
  if (!writeFile) {
    Serial.println("Error, failed to open cal.txt for writing!");
    return -4;
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
  Serial.println("Wrote to file /vent/cal.txt!");
  return 0;
}

int YGKMV::readCalFlash(){
  // Read in values from the calibration file if it exists
  File readFile = fatfs.open("/vent/cal.txt", FILE_READ);
  if (!readFile){
    P("Could not open /vent/cal.txt for reading.\n"); 
    return -8;
  }
  P("Reading lines from calibration file.\n");
  String line = "";
  line = readFile.readStringUntil('\n');
  while(line != ""){
    P("From File: "); PL(line);
    doConsoleCommand(line);
    line = readFile.readStringUntil('\n');
  }
  return 0;
}

void YGKMV::delCalFlash(){
  // Delete the calibration file
  fatfs.remove("/vent/cal.txt");
}

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