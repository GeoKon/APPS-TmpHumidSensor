/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
// Minimum necessary for this file to compile

#include <FS.h>
#include <externIO.h>       // IO includes and cpu...exe extern declarations
#include "mgnClass.h"
#include "myGlobals.h"
#include "SimpleSRV.h"      // We only need the getWiFiStatus()
#include "myCliHandlers.h"

MGN mgn;                   // allocate Meguno Link class

// ------------------------------ Streaming & Show measurements -----------------------------

    void bufferState( BUF *b )               // buffers state into buffer
    {
        b->set("{UI:CONFIG|SET|stream1.Text=");
        if( myp.gp.sensor == SENSOR_DS18 )                                                    // only temp given
        {
            if( myp.tempfound == 1 )
                b->add( "Temp: %.1fC (%.1fF)}\r\n", myp.tempC, C_TO_F(myp.tempC) );
            else
                b->add( "T1:%.1fC T2:%.1fC (%.1fF,%.1fF)}\r\n", myp.tempC, myp.tempC2, C_TO_F(myp.tempC), C_TO_F(myp.tempC2) );
        }
        else if( (myp.gp.sensor == SENSOR_DHT) || (myp.gp.sensor == SENSOR_HTU)  )
            b->add( "Temp: %.1fC (%.1fF), Humidity: %.0f%%}\r\n", myp.tempC, C_TO_F(myp.tempC), myp.humidity );
        else
            b->add( "No Sensor}\r\n" );
            
        b->add("{UI:CONFIG|SET|stream2.Text=");
        b->add( "Relay: %s, Simul: %s}\r\n", myp.relayON?"ON":"OFF", myp.simulON?"ON":"OFF" ); 

// OLD CODE        
//        RESPONSE( "  Temp:%5.1f°C (%5.1f°F), Humidity=%.0f%%", myp.tempC, myp.tempF, myp.humidity );
//# DS18B20 
//        RESPONSE( ", DS18s count:%d", temp.count() );
//#endif
//        if( myp.simulON )
//            RESPONSE( ", (simulated)");
//        
//        RESPONSE( "\r\nTarget:%5.1f°C (%5.1f°F), Delta:%5.1f°C (%5.1f°F), Mode:%d (in EEP)\r\n", 
//            myp.gp.threshold, CTOF(myp.gp.threshold), 
//            myp.gp.delta,     CTOF(myp.gp.delta),
//            myp.gp.tmode );
//        RESPONSE( " Relay:%s, Simul:%s, Filter:%d\r\n", 
//            myp.relayON?"ON":"OFF", 
//            myp.simulON?"ON":"OFF",
//            myp.gp.prdelay );
    }
    
    BUF streamb(256);                       // buffer used for streaming. Static/extern to avoid alloc/free all the time
    void mgnStream()                        // called by the main loop
    {
        if( !myp.gp.stream )                // do nothing is streaming is not enabled
            return;
        bufferState( &streamb );
        streamb.print();                    // and print it
    }
 
    
// ----------------------------- CLI Command Handlers ---------------------------

#define BINIT( A, ARG )      BUF *A = (BUF *)ARG[0]
#define RESPONSE( A, ... ) if( bp )                     \
                            bp->add(A, ##__VA_ARGS__ ); \
                           else                         \
                            PF(A, ##__VA_ARGS__);

//#define RESPONSE( A, ... )   bp->add(A, ##__VA_ARGS__)

#define FTOC(A) ((A)-32.0)*5.0/9.0
#define CTOF(A) (A)*9.0/5.0 + 32.0
                                   
    void help( int n, char **arg ){exe.help( n, arg );}    // what to do when "h" is entered

//    #define MYPTABLE_COMMON
//    #include "myCommon.hpp"

// ============================== TEMP or HUMIDITY SPECIFIC CLI ===============================
 

    void cliFilter( int n, char **arg )
    {
        BINIT( bp, arg );
        int pd = 0;
        if( n>=1 )
            pd = atoi( arg[1] );
        
        //filter.setPropDelay( myp.gp.prdelay = pd );
        
        RESPONSE( "Prop delay set to %d-samples\r\n", pd );
        myp.saveMyEEParms();
    }
    void cliHeatC( int n, char **arg )                      // sets mode to HEAT and new target temp
    {
        BINIT( bp, arg );
        if( n>1 )
        {
            myp.gp.threshold = atof( arg[1] );
            if( n>2 ) 
                myp.gp.delta = atof( arg[2] );
            myp.gp.tmode = HEAT_MODE;
            myp.saveMyEEParms();
        }
        bufferState( bp );
    }
    void cliHeatF( int n, char **arg )                      // sets mode to HEAT and new target temp
    {
        BINIT( bp, arg );
        if( n>1 )
        {
            myp.gp.threshold = F_TO_C(atof( arg[1] ));
            if( n>2 ) 
                myp.gp.delta = F_TO_C(atof( arg[2] ));
            myp.gp.tmode = HEAT_MODE;
            myp.saveMyEEParms();
        }
        bufferState( bp );
    }
    void cliManual( int n, char **arg )                     // Relay in Manual Mode
    {
        BINIT( bp, arg );
        myp.gp.tmode = RELAY_CONTROL;
        myp.saveMyEEParms();
        bufferState( bp );
    }
    void cliCoolC( int n, char **arg )
    {
        BINIT( bp, arg );
        if( n>2 )
            myp.gp.delta = atof( arg[2] );
        if( n>1 )
            myp.gp.threshold = atof( arg[1] );
        myp.gp.tmode = COOL_MODE;
        myp.saveMyEEParms();
        bufferState( bp );
    }
 
    // ============================== LOW LEVEL DIAGNOSTIC COMMANDS ===============================
 
    void cliReset( int n, char **arg )
    {
        PRN("Resetting...");
        digitalWrite( 14, HIGH );
        delay( 1000 );
        digitalWrite( 14, LOW );
        delay( 1000 );
        digitalWrite( 14, HIGH );
        PRN("Reset Failed");
    }
    bool kbabort()
    {
        if( Serial.available() )
            if( Serial.read() == 0x0D )
                return true;
        return false;
    }
    void cliTestOut45( int n, char **arg )                         
    {
        PRN("GPIO4, D1, CLK: ON(250ms), OFF(1000ms)");
        PRN("GPIO5, D2, SDA: ON(1000ms), OFF(250ms)");
        
        pinMode(4, OUTPUT);
        pinMode(5, OUTPUT);
        
        while( !kbabort() )
        {
            cpu.led( ON );
            digitalWrite( 4, true );
            digitalWrite( 5, false);
            delay( 250 );
            cpu.led( OFF );
            digitalWrite( 4, false);
            digitalWrite( 5, true );
            delay( 1000);
            PR(".");
        }
        CRLF();
    }
    void cliTestInputs( int n, char **arg )                        // displays list of files  
    {
        pinMode(14,INPUT);
        pinMode(2, INPUT);
        pinMode(4, INPUT);
        pinMode(5, INPUT);
        char *hi = "*H*";
        char *lo = "-L-";
        
        while( !kbabort() )
        {
           PF("GPIO2=%s GPIO4=%s GPIO5=%s GPIO14=%s\r\n",
           digitalRead( 2 ) ?hi:lo,
           digitalRead( 4 ) ?hi:lo,
           digitalRead( 5 ) ?hi:lo,
           digitalRead( 14 )?hi:lo );
           delay( 400 );
       }
    }
    void cliInputPIN( int n, char **arg )                        
    {
        if( n<=1 )
        {
            PRN("? Missing PIN");
            return;
        }
        char *hi = "*H*";
        char *lo = "-L-";

        int pin = atoi( arg[1] );
        pinMode( pin, INPUT);
        
        bool tnew, told;
        tnew = digitalRead( pin );
        told = !tnew;
        while( !kbabort() )
        {
           tnew = digitalRead( pin );
           if( tnew != told )
                PF("GPIO%d=%s\r\n", pin, tnew ? hi : lo );
           cpu.led( (onoff_t) tnew );
           told = tnew;
           delay( 100 );
       }
    }
    void cliOutputPIN( int n, char **arg )                        
    {
        static bool toggle = false;
        if( n<=1 )
        {
            PRN("?Missing PIN");
            return;
        }
        int pin = atoi( arg[1] );
        pinMode( pin, OUTPUT );
        
        while( !kbabort() )
        {
           toggle = !toggle;
           cpu.led( (onoff_t) toggle );
           digitalWrite( pin, toggle );
           delay( 500 );
       }
    }
    void cliFormat( int n, char **arg )                        
    {
        SPIFFS.format();
        PF("OK\r\n");
    }
    void cliDirectory( int n, char **arg )                        
    {
        Dir dir = SPIFFS.openDir("/");
        while (dir.next()) 
        {
            Serial.print(dir.fileName());
            File f = dir.openFile("r");
            Serial.println(f.size());
        }
    }
// ============================== MEGUNO LINK INTERFACE for EEPROM ===============================
    
    void cliShowEEP( BUF *mbf, char *status, char *help="" ) // all EEP changes call this at the end
    {
        if( mbf==NULL )
        {
            PF("Missing buffer for Meguno\r\n");
            return;
        }
        mgn.init( mbf, "CONFIG" );
        mgn.tableClear();
        mgn.tableSet( (const char *)status, " ", help );
        mgn.tableSet( "reboots:",   eep.head.reboots, "times" );
        mgn.tableSet( "ssid:",      "=", eep.wifi.ssid );
        mgn.tableSet( "pwd:",       "=", eep.wifi.pwd );
        mgn.tableSet( "port:",      eep.wifi.port, " " );
        mgn.tableSet( "staticIP:",  "=", eep.wifi.stIP );
        
        mgn.tableSet( "USER PARMS", " ", " " );
        mgn.tableSet( "sensor:", myp.gp.sensor, "0=none 1=DS18 2=DHT 3=HTU" );
        mgn.tableSet( "stream:", myp.gp.stream, "0:disabled 1:enabled" );
        mgn.tableSet( "tmode:", myp.gp.tmode, "0:none 1:heat 2:cool" );
        mgn.tableSet( "target:", myp.gp.threshold, "deg-C" );
        mgn.tableSet( "-", (float) C_TO_F(myp.gp.threshold), "deg-F" );
        mgn.tableSet( "prdelay:", myp.gp.prdelay, "number of samples to 90%" );
        mgn.tableSet( "delta:",   myp.gp.delta, "deg-C" );
    }
//    void fetchEEParms( int n, char **arg )
//    {
//        BINIT( bp, arg );
//        myp.fetchMyEEParms();
//        cliShowEEP( bp, "Fetched" );
//    }
//    void saveEEParms( int n, char **arg )
//    {
//        BINIT( bp, arg );
//        myp.saveMyEEParms();
//        cliShowEEP( bp, "Saved" );
//    }
    
    void mgnSetSensor( int n, char **arg )
    {
        BINIT( bp, arg );
        
        myp.gp.sensor = (n>=1) ? (sensor_t) atoi( arg[1] ) : SENSOR_NONE;
        myp.tempfound = 0;
        myp.tempindex = 0;
                    
        if( myp.gp.sensor == SENSOR_DS18 )
        {
            temp.search( true );                                    // initialize the Dallas Thermomete
            myp.tempfound = temp.count();
            char s[32];
            sprintf( s, "%d found", myp.tempfound );
            cliShowEEP( bp, "DS18B20", s );       
        }
        else if( myp.gp.sensor == SENSOR_DHT )
        {
            cliShowEEP( bp, "DHT22" );       
        }
        else if( myp.gp.sensor == SENSOR_HTU)
        {
            if( htu.init() )
                cliShowEEP( bp, "HTU21", "ok" );       
            else
                cliShowEEP( bp, "HTU21", "Not found" );
        }
        else
        {
            cliShowEEP( bp, "No Sensor" );       
        }
        myp.saveMyEEParms(); 
    }
    
    void mgnInitEEParms( int n, char **arg )       // initialize all EEPROM parms and save them
    {
        BINIT( bp, arg );
        eep.initHeadParms( MAGIC_CODE, sizeof( myp.gp ) );        // initialize header parameters AND save them in eeprom
        eep.saveHeadParms();

        eep.initWiFiParms();
        eep.saveWiFiParms();
                
        myp.initMyEEParms();        
        myp.saveMyEEParms();
        cliShowEEP( bp, "Initialized");
    }
    void mgnShowEEParms( int n, char **arg )
    {
        BINIT( bp, arg );
        cliShowEEP(bp, "EEP PARMS");
    }
    void mgnSetUserParm( int n, char **arg )
    {
        BINIT( bp, arg );
        char *p;
        if( n<=2 )
        {
            cliShowEEP( bp, "Missing <name> <value>");
            return;
        }
        char *name = arg[1];
        if( strcmp(name,"reboots")==0 )
        {
           eep.head.reboots = atoi( arg[2] );
           eep.saveHeadParms(); 
           cliShowEEP( bp, "Updated reboots");
           return;
        }
        else if( strcmp(name,"ssid")==0 )
            eep.initWiFiParms( arg[2], NULL, NULL, 0 );   

        else if( strcmp(name,"pwd")==0 )
            eep.initWiFiParms( NULL, arg[2], NULL, 0 );   

        else if( strcmp(name,"staticIP")==0 )
        {
            if( arg[2][0]=='*' )
                eep.initWiFiParms( NULL, NULL, "", 0 );   // use '*' for No Static
            else
                eep.initWiFiParms( NULL, NULL, arg[2], 0 );   // use '*' for No Static
        }        
        else if( strcmp(name,"port")==0 )
            eep.initWiFiParms( NULL, NULL, NULL, atoi( arg[2] ) );   

        else if( nmp.getParmType( name ) )
            nmp.setParmByStr( arg[1], arg[2] );
        else
        {
            cliShowEEP( bp, "Parm not found");
            return;
        }
        eep.saveWiFiParms();
        cliShowEEP( bp, "Updated Parms");
    }    
    void mgnSetStreaming( int n, char **arg )
    {
        if( n>1 )
            myp.gp.stream = atoi( arg[1] );
    }
    void mgnGetStatus( int n, char **arg )
    {
        BINIT( bp, arg );
        
        // construct the meguno message
        bp->set("{UI:CONFIG|SET|status.Text=");
        bufWiFiStatus( bp, false );                    // included in SimpleSRV.cpp
        bp->add( "}\r\n" );
    }
    void mgnSimulC( int n, char **arg )                         // simulate deg C 
    {
        BINIT( bp, arg );
        float value;
        if( n>1 )
        {
            myp.simulON = true;
            myp.simulT = atof( arg[1] );
            // temp.simulC( value );
            RESPONSE( "Simulation at %.1f°C\r\n", myp.simulT );
        }
        bufferState( bp );
    }
    void mgnSimulF( int n, char **arg )                         // displays list of files  
    {
        BINIT( bp, arg );
        float value;
        if( n>1 )
        {
            myp.simulON = true;
            value = atof( arg[1] );
            myp.simulT = FTOC( value );
            RESPONSE( "Simulation at %.1f°F\r\n", value );
        }
        bufferState( bp );
    }
    void mgnSimulOFF( int n, char **arg )                         // simulate deg C 
    {
        BINIT( bp, arg );
        // temp.simulOff(); 
        myp.simulON = false;
        bufferState( bp );
    }
    void mgnShowState( int n, char **arg )                         // simulate deg C 
    {
        BINIT( bp, arg );
        bufferState( bp );
    }

    void mgnSetRelay( int n, char **arg )                         // simulate deg C 
    {
        BINIT( bp, arg );
        if( n>=1 )
        {
            bool onoff = atoi( arg[1] ) > 0;
            digitalWrite( RELAY, onoff );
            myp.relayON = onoff;
        }
        bufferState( bp );
    }

// ============================== CLI COMMAND TABLE =======================================
                     
    CMDTABLE mypTable[]= 
    {
        {"h", "--- List of all commands ---", help },
        {"reset",   "Restart/Reboot",                                   cliReset },

        {"test45",  "Flip-flops (LED) (GPIO4) (GPIO5) until CR",        cliTestOut45 },
        {"testinp", "Inputs GPIOs: 02, 04, 05, 14 until CR",            cliTestInputs },
        {"inpPIN",  "p1 p2 ... pN. Inputs PINS until CR",               cliInputPIN },
        {"outPIN",  "pin. Squarewave output to 'pin' until CR",         cliOutputPIN },        

        {"format",  "Formats the filesystem",                           cliFormat }, 
        {"dir",     "List files in FS",                                 cliDirectory }, 

        
        {"filter",  "N. Define filter of N-samples",                    cliFilter      },

        {"heatC",  "[value] [delta]. Sets heat mode and target temp in °C",           cliHeatC    },
        {"heatF",  "Same in °F",                                                      cliHeatF    },
        {"coolC",  "[value] [delta]. Sets cool mode and target temp in °C",           cliCoolC    },
        {"tmode0", "Heat/Cool mode deactivated. Manual control of relay",             cliManual   },
        

//      {"u",           "--- User Parameter Commands ---", help },
//      {"usave",        "Save User EEP Parms",                         [](int, char**){ myp.saveMyEEParms();}},
//      {"uinit",        "Initialize default User EEP Parms",           [](int, char**){ myp.initMyEEParms();}},
//      {"ufetch",       "Fetch User EEP Parms",                        [](int, char**){ myp.fetchMyEEParms();}},
//      {"ushow",        "Show User EEP Parms",                         [](int, char**){ myp.printMyEEParms();} },
//      {"uset",         "name value. Set EEP Parm",                    cliSetUserParm   },

//      {"!fetchEEParms",   "Fetch parameters",                                   fetchEEParms },
//      {"!saveEEParms",    "Save parameters",                                    saveEEParms },

        {"!sensor",         "0=None 1=DS18 2=DHT 3=HTU. Select sensor type",      mgnSetSensor  },
        {"!initEEParms",    "Initialize parameters to defaults and save to EEP",  mgnInitEEParms },
        {"!showEEParms",    "Show all EEPROM parms",                              mgnShowEEParms },
        {"!uset",           "name value. Set EEP Parm",                           mgnSetUserParm },
        {"!stream",         "0|1 Enable streaming of measurements",               mgnSetStreaming },
        {"!status",         "Displays status to UI_status",                       mgnGetStatus },
        {"!simulF",         "[value]. Simulate °F",                               mgnSimulF   },
        {"!simulC",         "[value]. Simulate °C",                               mgnSimulC   },
        {"!simulO",         "Simulation Off",                                     mgnSimulOFF },
        {"!state",          "Shows measurement state",                            mgnShowState },
        {"!relay",          "0|1 set relay ON/OFF (assumes tmode=0)",             mgnSetRelay },

        {NULL, NULL, NULL}
    };
