/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
// Minimum necessary for this file to compile

#include <FS.h>
#include <externIO.h>       // IO includes and cpu...exe extern declarations
#include "myGlobals.h"
#include "SimpleSRV.h"      // We only need the getWiFiStatus()
#include "commonCLI.h"

//MGN mgn;                   // allocate Meguno Link class
char *channel = "";

// ----------------------------- CLI Command Handlers ---------------------------

    void help ( int n, char **arg ){exe.help ( n, arg );}    // what to do when "h" is entered
    void brief( int n, char **arg ){exe.brief( n, arg );} 
    
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

    void mgnGetStatus( int n, char **arg )
    {
        BINIT( bp, arg );
        
        // construct the meguno message
        bp->set("{UI:CONFIG|SET|status.Text=");
        bufWiFiStatus( bp, false );                    // included in SimpleSRV.cpp
        bp->add( "}\r\n" );
    }

// =========================== PARAMETER (EEPROM) MANAGEMENT ==============================

    void cliInitEEParms( int n, char **arg )       // initialize all EEPROM parms and save them
    {
        BINIT( bp, arg );
        eep.initHeadParms( MAGIC_CODE, sizeof( myp.gp ) );        // initialize header parameters AND save them in eeprom
        eep.saveHeadParms();

        eep.initWiFiParms();
        eep.saveWiFiParms();
                
        myp.initMyEEParms();        
        myp.saveMyEEParms();
        RESPONSE("Initialized\r\n");        
    }
    void mgnInitEEParms( int n, char **arg )       // initialize all EEPROM parms and save them
    {
        cliInitEEParms( n, arg );
        nmp.printMgnInfo( channel, "", "EE Parms" );     
        eep.printWiFiParms( channel ); 
        nmp.printMgnAllParms( channel );
    }
    void cliShowWiFiParms( int n, char **arg )
    {
        BINIT( bp, arg );          
        eep.printWiFiParms("", bp);      
    }
    void cliShowAllParms( int n, char **arg )
    {
        BINIT( bp, arg );          
        eep.printWiFiParms("", bp);      
        nmp.printAllParms("", bp );          
    }
    void cliShowUserParms( int n, char **arg )
    {            
        BINIT( bp, arg );
        int N = nmp.getParmCount();
        for( int i=1; i<N; i++ )
        {
            RESPONSE( "%s=%s%s", nmp.getParmName(i), nmp.getParmValueStr(i), (i==n-1)?"\r\n":", " );
        }
    }
    void mgnShowAllParms( int n, char **arg )
    {
        nmp.printMgnInfo( channel, "", "All EEP Parms" );     
        
        PF( "{TABLE:CONFIG|SET|WIFI PARMS| |---------------------------- }\r\n");
        eep.printMgnWiFiParms( channel ); 
        PF( "{TABLE:CONFIG|SET|USER PARMS| |---------------------------- }\r\n");
        nmp.printMgnAllParms( channel );
    }
    void cliGetUserParm( int n, char **arg )
    {
        BINIT( bp, arg );        
        if( n<=1 )
        {
            RESPONSE("? Missing parm name");
        }
        else
        {
            //int N = nmp.getParmCount();
            for( int i=1; i<n; i++ )
            {
                RESPONSE( "%s=%s%s", arg[i], nmp.getParmValueStr( arg[i] ), (i==n-1)?"\r\n":", " );
            }
        }
    }
    
    static bool _setresult; // to communicate result to next routine
    void cliSetAnyParm( int n, char **arg )
    {
        BINIT( bp, arg );   
        bool ok;
        if( n<=2 )
        {
            RESPONSE("? Missing <name> <value>\r\n" );
            ok = false;
        }
        else
        {
            ok = eep.setWiFiParm( arg[1], arg[2] );         // if found, modify & save in EEPROM
            if( !ok )        
            {
                ok = nmp.setParmByStr( arg[1], arg[2] );         // if found, change parameter
                if( ok )
                    myp.saveMyEEParms();                         // save to EEPROM
            }            
        }
        RESPONSE( ok ? "%s updated\r\n" : "%s not found\r\n", arg[1] ); 
    }
    void mgnSetAnyParm( int n, char **arg )
    {
        bool ok;
        if( n<=2 )
        {
            nmp.printMgnInfo( channel, "", "Missing args" );
            return;
        }
        else
        {
            ok = eep.setWiFiParm( arg[1], arg[2] );              // if found, modify & save in EEPROM
            if( ok ) 
                eep.printMgnWiFiParms( channel );
            else
            {
                ok = nmp.setParmByStr( arg[1], arg[2] );         // if found, change parameter
                if( ok )
                {
                    myp.saveMyEEParms();                         // save to EEPROM
                    nmp.printMgnParm( channel, arg[1] );         // update the table
                }
            }            
        }
        nmp.printMgnInfo( channel, arg[1], (char *) (ok ? "updated": "is unknown") );    // update the INFO    
    }

// ============================== CLI COMMAND TABLE =======================================
                     
    CMDTABLE cmnTable[]= 
    {
        {"h",       "[mask]. Lists of all commands",                    help },
        {"b",       "[mask]. Brief help",                               brief },
        {"p",       "shows user parameters (brief)",                    cliShowUserParms },
        {"w",       "shows WiFi parameters (brief)",                    cliShowWiFiParms },
        
        {"reset",   "Restart/Reboot",                                   cliReset },

        {"test45",  "Flip-flops (LED) (GPIO4) (GPIO5) until CR",        cliTestOut45 },
        {"testinp", "Inputs GPIOs: 02, 04, 05, 14 until CR",            cliTestInputs },
        {"inpPIN",  "p1 p2 ... pN. Inputs PINS until CR",               cliInputPIN },
        {"outPIN",  "pin. Squarewave output to 'pin' until CR",         cliOutputPIN },        

        {"format",  "Formats the filesystem",                           cliFormat }, 
        {"dir",     "List files in FS",                                 cliDirectory }, 
        
        {"initEE",          "Initialize parameters to defaults and save to EEP",cliInitEEParms },
        {"!initEE",         "",                                                 mgnInitEEParms },

        {"set",             "name value. Update parm & save in EEPROM",         cliSetAnyParm },
        {"!set",            "",                                                 mgnSetAnyParm },
        {"get",             "name1..nameN. Get parameter values",               cliGetUserParm },
        
        {"show",            "show all parameters",                              cliShowAllParms }, 
        {"!show",           "",                                                 mgnShowAllParms }, 
        
        {"!status",         "Displays wifi status",                             mgnGetStatus },
 
        {NULL, NULL, NULL}
    };
