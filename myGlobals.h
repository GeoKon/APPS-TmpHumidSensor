/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
#pragma once

#include <ds18Class.h>      // in GKE-L2
#include "SimpleDHT.h"      // in GKE-L2
    
#include "externIO.h"       // in GKE-Lw. Includes and externs of cpu,...,eep
#include <nmpClass.h>

// ----------------- Exported by this module ---------------------------------------

    extern NMP nmp;             // allocated in myGlobals.cpp; used only by this                             
    extern DS18 temp;			// allocated in myGlobals.cpp; used this application 
    extern SimpleDHT22 dht22;	// allocated in myGlobals.cpp; used this application 
//	extern Global myp;          // allocated in myGlobals.cpp; see end of this file

// ----------------- GLOBAL HARDWARE CONSTANTS -------------------------------------

// Choose one sensor
//    #define DS18B20           // either DS18B22 or DHT22
      #define DHT22

// Choose CPU
      #define NODEMCU           // either NODEMCU or SONOFF
//    #define SONOFF            // Flash=DOUT, Size=1M (256k SPIFFS)
  
    #ifdef SONOFF 
        //Inputs
        #define BUTTON       0
        #define DS_PIN       4      // Do not use 02 (must be high during RESET)
        // Outputs
        #define MYLED       13
        #define RELAY       12
    #endif
    
    #ifdef NODEMCU
        //Inputs
        #define BUTTON       0
        #define DS_PIN       4      // this is D2, aka SDA
        // Outputs
        #define MYLED       16
        #define RELAY        5
    #endif

// --------------- Definition of Global parameters ---------------------------------

    enum wifi_state { TRYING_WIFI=0, STA_MODE=2, AP_MODE=1 };
    enum thermode_t { NO_RELAY=0, HEAT_MODE=1, COOL_MODE=2 };
    
    class Global
    {
      public:												// ======= A1. Add here all volatile parameters 
        wifi_state wifiOK;          // state variables
        
        float tempC;                // current temperature in C
        float tempF;                // current temperature in F
        float humidity;             // current humidity
        
        bool relayON;               // state of the relay
        bool simulON;               // active simulation
        float simulT;               // simulated temperature in C;                                                        
    
		void initVolatile()                                 // ======= A2. Initialize here the volatile parameters
		{
			wifiOK = TRYING_WIFI;      
            tempF = 32.0;
            tempC = humidity = simulT = 0.0;
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
            int tmode;              // 0=no_relay 1=heat, 2=cold
            int prdelay;            // propagation delay. Use 0 for no delay
            float threshold;        // always in deg C
            float delta;            // in deg C
		} gp;
		
		void initMyEEParms()                                // ======= B2. Initialize here the non-volatile parameters
		{
            gp.stream    = 0;
            gp.tmode     = (thermode_t) NO_RELAY;
            gp.prdelay   = 0;
            gp.threshold = 0.0;
            gp.delta     = 0.9;
		}		
        void registerMyEEParms()                           // ======= B3. Register parameters by name
        {
            nmp.resetRegistry();
            nmp.registerParm( "stream",     'd', &gp.stream,    "=%d (0:none 1:streaming ON)"    );
            nmp.registerParm( "tmode",      'd', &gp.tmode,     "=%d (0:none 1:heat 2:cool)"    );
            nmp.registerParm( "prdelay",    'd', &gp.prdelay,   "=%d (0:no smoothing)"          );
            nmp.registerParm( "target",     'f', &gp.threshold, "=%.1f°C (target temp)"         );
            nmp.registerParm( "delta",      'f', &gp.delta,     "=%.2f°C (hysterysis)"          );
			
			PF("%d named parameters registed\r\n", nmp.nparms );
			ASSERT( nmp.getSize() == sizeof( gp_t ) );                             // remember the size
        }
        void printMyEEParms( char *prompt="", BUF *bp=NULL ) // ======= B4. Add to buffer (or print) all volatile parms
		{
			nmp.printParms( prompt );
		}
		#include <GLOBAL.hpp>                               // Common code for all Global implementations
		
	//    void initAllParms( int myMagic  )       
	//    void fetchMyEEParms()
	//    void saveMyEEParms()
	};
    
    extern Global myp;                                      // exported class by this module
