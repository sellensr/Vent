Vent_4 to be FROZEN 2020-04-14 as shipped with prototype. Moving on to Vent_5

  Future Features and Mods:
    enable shorter values for STOP_MAX in cooperation with display unit
    save and recover settings to resume as was after power cycle, independent of display unit
    add plateau phase to allow 0 to 1 second delay between inspiration phase and expiration phase, probably as a fraction of inspiration time.
    report pause time remaing before auto restart

  Vent requires some libraries you may not have installed. These two can be installed
  via Tools/Manage Libraries... inside the IDE. Search for sdFat and be sure to install
  the Adafruit fork of the original. 
  #include <SdFat.h>                // https://github.com/adafruit/SdFat
  #include <Adafruit_SPIFlash.h>    // https://github.com/adafruit/Adafruit_SPIFlash
  This one you will need to download from github and copy into your libraries folder.
  #include "RWS_UNO.h"    // https://github.com/sellensr/RWS_UNO

