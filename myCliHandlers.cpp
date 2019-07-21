/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
// Minimum necessary for this file to compile

#include <externIO.h>      // IO includes and cpu...exe extern declarations
#include "myGlobals.h"
#include "myCliHandlers.h"

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
 
    void showState( BUF *bp )
    {
        RESPONSE( "  Temp:%5.1f°C (%5.1f°F), Humidity=%.0f%%", myp.tempC, myp.tempF, myp.humidity );
#ifdef DS18B20 
        RESPONSE( ", DS18s count:%d", temp.count() );
#endif
        if( myp.simulON )
            RESPONSE( ", (simulated)");
        
        RESPONSE( "\r\nTarget:%5.1f°C (%5.1f°F), Delta:%5.1f°C (%5.1f°F), Mode:%d (in EEP)\r\n", 
            myp.gp.threshold, CTOF(myp.gp.threshold), 
            myp.gp.delta,     CTOF(myp.gp.delta),
            myp.gp.tmode );
        RESPONSE( " Relay:%s, Simul:%s, Filter:%d\r\n", 
            myp.relayON?"ON":"OFF", 
            myp.simulON?"ON":"OFF",
            myp.gp.prdelay );
    }    
    void cliShowState( int n, char **arg )                         // simulate deg C 
    {
        BINIT( bp, arg );
        showState( bp );
    }
    void cliSimulC( int n, char **arg )                         // simulate deg C 
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
        showState( bp );
    }
    void cliSimulF( int n, char **arg )                         // displays list of files  
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
        showState( bp );
    }
    void cliSimulOFF( int n, char **arg )                         // simulate deg C 
    {
        BINIT( bp, arg );
        // temp.simulOff(); 
        myp.simulON = false;
        showState( bp );
    }
    void cliInit( int n, char **arg )
    {
        BINIT( bp, arg );
        cpu.led( BLINK, 2 );
#ifdef DS18B20
        temp.search( true );                                    // initialize the Dallas Thermomete
        RESPONSE("Found %d sensors\r\n", temp.count() );
#endif        
    }

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
    void cliHeatC( int n, char **arg )      // sets mode to HEAT and new target temp
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
        showState( bp );
    }
    void cliHeatF( int n, char **arg )                         // displays list of files  
    {
        BINIT( bp, arg );
        if( n>1 )
        {
            myp.gp.threshold = FTOC(atof( arg[1] ));
            if( n>2 ) 
                myp.gp.delta = FTOC(atof( arg[2] ));
            myp.gp.tmode = HEAT_MODE;
            myp.saveMyEEParms();
        }
        showState( bp );
    }
    void cliHeatOFF( int n, char **arg )                         // heater off
    {
        BINIT( bp, arg );
        myp.gp.tmode = NO_RELAY;
        myp.saveMyEEParms();
        showState( bp );
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
        showState( bp );
    }
    void cliSetUserParm( int n, char **arg )
    {
        BINIT( bp, arg );
        if( n<=2 )
        {
            RESPONSE( "? Missing <name> <value>\r\n");
            return;
        }
        if( !nmp.getParmType( arg[1] ) )
        {
            RESPONSE( "? %s parm not found\r\n", arg[1] );
        }
        else
        {
            nmp.setParmByStr( arg[1], arg[2] );
            RESPONSE( "ok. Use 'usave' after you are done\r\n" );
        }
    }
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
    CMDTABLE mypTable[]= 
    {
        {"h", "--- List of all commands ---", help },
        {"reset",   "Restart/Reboot",                                   cliReset },

        {"test45",  "Flip-flops (LED) (GPIO4) (GPIO5) until CR",        cliTestOut45 },
        {"testinp", "Inputs GPIOs: 02, 04, 05, 14 until CR",            cliTestInputs },
        {"inpPIN",  "p1 p2 ... pN. Inputs PINS until CR",               cliInputPIN },
        {"outPIN",  "pin. Squarewave output to 'pin' until CR",         cliOutputPIN },        
        
        {"state",   "Shows measurement state",                          cliShowState },
        {"init",    "Initialize DS Sensors",                            cliInit     },
        {"filter",  "N. Define filter of N-samples",                    cliFilter   },

        {"simulC",  "[value]. Simulate °C",                             cliSimulC   },
        {"simulF",  "Same in °F",                                       cliSimulF   },
        {"simulO",  "Simulation Off",                                   cliSimulOFF },

        {"heatC",  "[value] [delta]. Sets target temp in °C",           cliHeatC    },
        {"heatF",  "Same in °F",                                        cliHeatF    },
        {"heatO",  "Heat relay deactivated",                            cliHeatOFF    },
        {"coolC",  "[value] [delta]. Turn cooler on",                   cliCoolC    },

//      {"u",           "--- User Parameter Commands ---", help },
        {"usave",        "Save User EEP Parms",                         [](int, char**){ myp.saveMyEEParms();}},
        {"uinit",        "Initialize default User EEP Parms",           [](int, char**){ myp.initMyEEParms();}},
        {"ufetch",       "Fetch User EEP Parms",                        [](int, char**){ myp.fetchMyEEParms();}},
        {"ushow",        "Show User EEP Parms",                         [](int, char**){ myp.printMyEEParms();} },
        {"uset",         "name value. Set EEP Parm",                    cliSetUserParm   },

        {NULL, NULL, NULL}
    };
