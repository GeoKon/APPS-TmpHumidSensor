#include "eepGlobals.h"

// --------------- forward references (in this file) ---------------------------

#define myMAGIC 0xABCD
void setup()
{
    cpu.init();
    SPIFFS.begin();  

    myp.registerMyParms();                              // register named parameters once for all
    
    if( !eep.checkEEParms( myMAGIC, myp.bsize ) )      // fetches parameters and returns TRUE or FALSE
    {
        PF("=== Initializing parms!\r\n" );
        eep.initHeadParms( myMAGIC );                   // this sets user_size to zero, 
                                                        // ... but modified to myp.bsize by eep.setUserStruct()
        eep.initWiFiParms();                            // initialize with default WiFi
        myp.initMyParms();                              // initialize named parameters and save them in EEPROM
        ser.printParms("AFTER INITIALIZATION");
    }
    eep.incrBootCount();
    myp.fetchMyParms();                                 // retrieve parameters from EEPROM
    eep.printHeadParms("--- Current Parms");            // print current parms
    eep.printWiFiParms();                 
    ser.printParms("--- User Parms");
    
    eepTable::init( exe, eep );         // create link to eep tables
    mypTable::init( exe );               // create link to MY table
     
    exe.registerTable( mypTable::table );
    exe.registerTable( eepTable::table );
    
    interactForever();
}

void loop() 
{
    yield();
}
