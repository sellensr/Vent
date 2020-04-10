#include <SPI.h>
#include <SdFat.h>                // https://github.com/adafruit/SdFat
#include <Adafruit_SPIFlash.h>    // https://github.com/adafruit/Adafruit_SPIFlash
// Largely copied from the SdFat_full_usage example of the Adafruit_SPIFlash library.

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

void setupFlash(){
  Serial.println("Checking the Flash for a Calibration File");

  // Initialize flash library and check its chip ID.
  if (!flash.begin()) {
    Serial.println("Error, failed to initialize flash chip!");
    while(1) yield();
  }
  Serial.print("Non-unique Flash chip JEDEC ID: 0x"); Serial.println(flash.getJEDECID(), HEX);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!fatfs.begin(&flash)) {
    Serial.println("Error, failed to mount newly formatted filesystem!");
    Serial.println("Was the flash chip formatted with the SdFat_format example?");
    while(1) yield();
  }
  Serial.println("Mounted filesystem!");

  // Check if a directory called 'vent' exists and create it if not there.
  // Note you should _not_ add a trailing slash (like '/vent/') to directory names!
  // You can use the same exists function to check for the existance of a file too.
  if (!fatfs.exists("/vent")) {
    Serial.println("Vent directory not found, creating...");
    
    // Use mkdir to create directory (note you should _not_ have a trailing slash).
    fatfs.mkdir("/vent");
    
    if ( !fatfs.exists("/vent") ) {
      Serial.println("Error, failed to create directory!");
      while(1) yield();
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
    writeCalFlash();
  } else {
    Serial.println("Calibration file found, reading in the saved values...");
    readCalFlash();
  }
  
}

int writeCalFlash(){
  delCalFlash();   // delete the old file
  // Create a calibration file in the vent directory and write data to it.
  File writeFile = fatfs.open("/vent/cal.txt", FILE_WRITE);
  if (!writeFile) {
    Serial.println("Error, failed to open cal.txt for writing!");
    return -1;
  }
  Serial.println("Opened file /vent/cal.txt for writing/appending...");
  char sc[200] = {0};
  // write a calibration constants line
  sprintf(sc,"C%7.4f,%7.4f,%7.4f,%7.2f,%7.2f,%7.2f\n",p_pOffset,p_qOffsetCPAP,p_qOffsetPEEP,p_pScale,p_qScaleCPAP,p_qScalePEEP);
  writeFile.print(sc);
  PR(sc);
  // write a servo angles line
  sprintf(sc,"S%d,%d,%d,%d,%d,%d\n",aMinCPAP,aMaxCPAP,aMinPEEP,aMaxPEEP,aCloseCPAP,aClosePEEP);
  writeFile.print(sc);
  PR(sc);
  // Close the file when finished writing.
  writeFile.close();
  Serial.println("Wrote to file /vent/cal.txt!");
  return 0;
}

int readCalFlash(){
  // Read in values from the calibration file if it exists
  File readFile = fatfs.open("/vent/cal.txt", FILE_READ);
  if (!readFile){
    P("Could not open /vent/cal.txt for reading.\n"); 
    return -1;
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

void delCalFlash(){
  // Delete the calibration file
  fatfs.remove("/vent/cal.txt");
}

void wipeCalFlash(){
  // Delete the calibration file
  fatfs.remove("/vent/cal.txt");
  // Restore the starting values
  p_pScale = 1.0;
  p_pOffset = 0.0;
  p_qScaleCPAP = 1.0;
  p_qOffsetCPAP = 0.0;
  p_qScalePEEP = 1.0;
  p_qOffsetPEEP = 0.0;
  
}

void setupExample() {
  // Initialize serial port and wait for it to open before continuing.
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }
  Serial.println("Adafruit SPI Flash FatFs Full Usage Example");

  // Initialize flash library and check its chip ID.
  if (!flash.begin()) {
    Serial.println("Error, failed to initialize flash chip!");
    while(1) yield();
  }
  Serial.print("Flash chip JEDEC ID: 0x"); Serial.println(flash.getJEDECID(), HEX);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!fatfs.begin(&flash)) {
    Serial.println("Error, failed to mount newly formatted filesystem!");
    Serial.println("Was the flash chip formatted with the SdFat_format example?");
    while(1) yield();
  }
  Serial.println("Mounted filesystem!");

  // Check if a directory called 'test' exists and create it if not there.
  // Note you should _not_ add a trailing slash (like '/test/') to directory names!
  // You can use the same exists function to check for the existance of a file too.
  if (!fatfs.exists("/test")) {
    Serial.println("Test directory not found, creating...");
    
    // Use mkdir to create directory (note you should _not_ have a trailing slash).
    fatfs.mkdir("/test");
    
    if ( !fatfs.exists("/test") ) {
      Serial.println("Error, failed to create directory!");
      while(1) yield();
    }else {
      Serial.println("Created directory!");
    }
  }

  // You can also create all the parent subdirectories automatically with mkdir.
  // For example to create the hierarchy /test/foo/bar:
  Serial.println("Creating deep folder structure...");
  if ( !fatfs.exists("/test/foo/bar") ) {
    Serial.println("Creating /test/foo/bar");
    fatfs.mkdir("/test/foo/bar");

    if ( !fatfs.exists("/test/foo/bar") ) {
      Serial.println("Error, failed to create directory!");
      while(1) yield();
    }else {
      Serial.println("Created directory!");
    }
  }

  // This will create the hierarchy /test/foo/baz, even when /test/foo already exists:
  if ( !fatfs.exists("/test/foo/baz") ) {
    Serial.println("Creating /test/foo/baz");
    fatfs.mkdir("/test/foo/baz");

    if ( !fatfs.exists("/test/foo/baz") ) {
      Serial.println("Error, failed to create directory!");
      while(1) yield();
    }else {
      Serial.println("Created directory!");
    }
  }

  // Create a file in the test directory and write data to it.
  // Note the FILE_WRITE parameter which tells the library you intend to
  // write to the file.  This will create the file if it doesn't exist,
  // otherwise it will open the file and start appending new data to the
  // end of it.
  File writeFile = fatfs.open("/test/test.txt", FILE_WRITE);
  if (!writeFile) {
    Serial.println("Error, failed to open test.txt for writing!");
    while(1) yield();
  }
  Serial.println("Opened file /test/test.txt for writing/appending...");

  // Once open for writing you can print to the file as if you're printing
  // to the serial terminal, the same functions are available.
  writeFile.println("Hello world!");
  writeFile.print("Hello number: "); writeFile.println(123, DEC);
  writeFile.print("Hello hex number: 0x"); writeFile.println(123, HEX);

  // Close the file when finished writing.
  writeFile.close();
  Serial.println("Wrote to file /test/test.txt!");

  // Now open the same file but for reading.
  File readFile = fatfs.open("/test/test.txt", FILE_READ);
  if (!readFile) {
    Serial.println("Error, failed to open test.txt for reading!");
    while(1) yield();
  }

  // Read data using the same read, find, readString, etc. functions as when using
  // the serial class.  See SD library File class for more documentation:
  //   https://www.arduino.cc/en/reference/SD
  // Read a line of data:
  String line = readFile.readStringUntil('\n');
  Serial.print("First line of test.txt: "); Serial.println(line);

  // You can get the current position, remaining data, and total size of the file:
  Serial.print("Total size of test.txt (bytes): "); Serial.println(readFile.size(), DEC);
  Serial.print("Current position in test.txt: "); Serial.println(readFile.position(), DEC);
  Serial.print("Available data to read in test.txt: "); Serial.println(readFile.available(), DEC);

  // And a few other interesting attributes of a file:
  Serial.print("File name: "); Serial.println(readFile.name());
  Serial.print("Is file a directory? "); Serial.println(readFile.isDirectory() ? "Yes" : "No");

  // You can seek around inside the file relative to the start of the file.
  // For example to skip back to the start (position 0):
  if (!readFile.seek(0)) {
    Serial.println("Error, failed to seek back to start of file!");
    while(1) yield();
  }

  // And finally to read all the data and print it out a character at a time
  // (stopping when end of file is reached):
  Serial.println("Entire contents of test.txt:");
  while (readFile.available()) {
    char c = readFile.read();
    Serial.print(c);
  }

  // Close the file when finished reading.
  readFile.close();

  // You can open a directory to list all the children (files and directories).
  // Just like the SD library the File type represents either a file or directory.
  File testDir = fatfs.open("/test");
  if (!testDir) {
    Serial.println("Error, failed to open test directory!");
    while(1) yield();
  }
  if (!testDir.isDirectory()) {
    Serial.println("Error, expected test to be a directory!");
    while(1) yield();
  }
  Serial.println("Listing children of directory /test:");
  File child = testDir.openNextFile();
  while (child) {
    char filename[64];
    child.getName(filename, sizeof(filename));
    
    // Print the file name and mention if it's a directory.
    Serial.print("- "); Serial.print(filename);
    if (child.isDirectory()) {
      Serial.print(" (directory)");
    }
    Serial.println();
    // Keep calling openNextFile to get a new file.
    // When you're done enumerating files an unopened one will
    // be returned (i.e. testing it for true/false like at the
    // top of this while loop will fail).
    child = testDir.openNextFile();
  }

  // If you want to list the files in the directory again call
  // rewindDirectory().  Then openNextFile will start from the
  // top again.
  testDir.rewindDirectory();

  // Delete a file with the remove command.  For example create a test2.txt file
  // inside /test/foo and then delete it.
  File test2File = fatfs.open("/test/foo/test2.txt", FILE_WRITE);
  test2File.close();
  Serial.println("Deleting /test/foo/test2.txt...");
  if (!fatfs.remove("/test/foo/test2.txt")) {
    Serial.println("Error, couldn't delete test.txt file!");
    while(1) yield();
  }
  Serial.println("Deleted file!");

  // Delete a directory with the rmdir command.  Be careful as
  // this will delete EVERYTHING in the directory at all levels!
  // I.e. this is like running a recursive delete, rm -rf *, in
  // unix filesystems!
  Serial.println("Deleting /test directory and everything inside it...");
  if (!testDir.rmRfStar()) {
    Serial.println("Error, couldn't delete test directory!");
    while(1) yield();
  }
  // Check that test is really deleted.
  if (fatfs.exists("/test")) {
    Serial.println("Error, test directory was not deleted!");
    while(1) yield();
  }
  Serial.println("Test directory was deleted!");

  Serial.println("Finished!");
}
