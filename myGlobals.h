/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
#pragma once

// =================== GLOBAL HARDWARE CONSTANTS ===================================

// #define NODEMCU           // 2. Choose COU either NODEMCU or SONOFF
   #define SONOFF            // Flash=DOUT, Size=1M (256k SPIFFS)

// =================================================================================
/*
    To Program Sonoff
    Select “Generic ESP8266 Module”
    Flash size “1M (128k SPIFFS)”
    Flash Mode: DOUT
    Reset method “ck”
    Crystal “26MHz”; Flash “40MHz”, CPU “80MHz”
    Build in LED “2”
    Disconnect power  Press & Hold Flash  Connect power  Release Flash
    Download new code
    Use command line to initialize Unit Label
*/

#include <ds18Class.h>      // in GKE-L2
#include <htu21Class.h>     // in GKE-L2
#include "SimpleDHT.h"      // in GKE-L2
    
#include "externIO.h"       // in GKE-Lw. Includes and externs of cpu,...,eep
#include <nmpClass.h>

// ----------------- Exported by this module ---------------------------------------

    extern NMP nmp;             // allocated in myGlobals.cpp; used only by this                             
    extern DS18 temp;			// allocated in myGlobals.cpp; used this application 
    extern HTU21 htu;           // allocated in myGlobals.cpp; used this application 
    
    extern SimpleDHT22 dht22;	// allocated in myGlobals.cpp; used this application 

    #ifdef SONOFF 
        #define BUTTON       0      // INPUTS
        #define DS_PIN       4      // Do not use 02 (must be high during RESET)
        #define MYLED       13      // OUTPUTS
        #define RELAY       12
    #endif
    
    #ifdef NODEMCU
        #define BUTTON       0      // INPUTS
        #define DS_PIN       4      // this is D2, aka SDA
        #define MYLED       16      // OUTPUTS
        #define RELAY       12      // NodeMCU D4
    #endif

#define MAGIC_CODE 0x1447

float ctof( float X );
float ftoc( float X );
    
// --------------- Definition of Global parameters ---------------------------------

    enum wifi_state { TRYING_WIFI=0, STA_MODE=2, AP_MODE=1 };
    enum thermode_t { RELAY_CONTROL=0, HEAT_MODE=1, COOL_MODE=2 };
    enum sensor_t   { SENSOR_NONE=0, SENSOR_DS18=1, SENSOR_DHT=2, SENSOR_HTU=3 };
    
    class Global
    {
      public:												// ======= A1. Add here all volatile parameters 
        wifi_state wifiOK;          // state variables
        
        int   tempfound;            // number of sensors found
        int   tempindex;            // 0 or 1 
        float tempC;                // current temperature in C
        float tempC2;               // second temperature in C
        float humidity;             // current humidity
        
        bool relayON;               // state of the relay
        bool simulON;               // active simulation
        float simulT;               // simulated temperature in C;                                                        
    
		void initVolatile()                                 // ======= A2. Initialize here the volatile parameters
		{
			wifiOK = TRYING_WIFI;      
            tempfound = 0;
            tempC = tempC2 =  simulT = 0.0;
            humidity = -1.0;        // indicates temp measurement only
			relayON = false;
			simulON = false;
		}    
		void printVolatile( char *prompt="", BUF *bp=NULL ) // ======= A3. Add to buffer (or print) all volatile parms
		{
			;
		}
		struct gp_t                                         // ======= B1. Add here all non-volatile parameters into a structure
		{                           
            int stream;             // serial display streaming
            sensor_t sensor;        // 0=none, 1=temp, 2=temp/hum
            int flip;               // flip order of DS18 sensors
            int tmode;              // 0=no_relay 1=heat, 2=cold
            int prdelay;            // propagation delay. Use 0 for no delay
            float threshold;        // always in deg C
            float delta;            // in deg C
		} gp;
		
		void initMyEEParms()                                // ======= B2. Initialize here the non-volatile parameters
		{
            gp.stream    = 0;
            gp.sensor    = SENSOR_DS18;
            gp.flip      = 0;
            gp.tmode     = (thermode_t) RELAY_CONTROL;
            gp.prdelay   = 0;
            gp.threshold = 0.0;
            gp.delta     = 0.9;
		}		
        void registerMyEEParms()                           // ======= B3. Register parameters by name
        {
            nmp.resetRegistry();
            nmp.registerParm( "stream",     'd', &gp.stream,    "0:none 1:streaming ON"    );
            nmp.registerParm( "sensor",     'd', &gp.sensor,    "0:none 1:DS18 2=DHT 3=HTU" );
            nmp.registerParm( "flip",       'd', &gp.flip,      "flip DS18 sensors (0 or 1)" );
            nmp.registerParm( "tmode",      'd', &gp.tmode,     "0:manual 1:heat 2:cool"  );
            nmp.registerParm( "prdelay",    'd', &gp.prdelay,   "0:no smoothing"          );
            nmp.registerParm( "target",     'f', &gp.threshold, "target temp deg-F", "%.1f"  );
            nmp.registerParm( "delta",      'f', &gp.delta,     "hysterysis deg-F",  "%.2f"  );
			
			PF("%d named parameters registed\r\n", nmp.getParmCount() );
			ASSERT( nmp.getSize() == sizeof( gp_t ) );                             // remember the size
        }
        void printMyEEParms( char *prompt="", BUF *bp=NULL ) // ======= B4. Add to buffer (or print) all volatile parms
		{
			nmp.printAllParms( prompt );
		}
		#include <GLOBAL.hpp>                               // Common code for all Global implementations
		
	//    void initAllParms( int myMagic  )       
	//    void fetchMyEEParms()
	//    void saveMyEEParms()                              // saves USER parameters (only)
	};
    
    extern Global myp;                                      // exported class by this module
