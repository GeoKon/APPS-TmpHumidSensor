/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
// Minimum necessary for this file to compile

#include <FS.h>
#include <oledClass.h>      
#include <iirClass.h>

#include "myGlobals.h"
#include "myCliHandlers.h"

OLED oled;
IIR iir1, iir2, iir3;      // IIR filters for T1, T2, Humidity

static char *channel = "";
#define RESPONSE( A, ... ) if( bp )                     \
                            bp->add(A, ##__VA_ARGS__ ); \
                           else                         \
                            PF(A, ##__VA_ARGS__);
                            
// ---------------------- 1. Initialization routines used by setup() and this module ---------------------------

    void initFilters( int samples )                                 // exported to be used in main program
    {
        iir1.setStepsToTarget( samples, 90 );
        iir2.setStepsToTarget( samples, 90 );
        iir3.setStepsToTarget( samples, 90 );
    }  
    void initSensors( sensor_t  senstype, BUF *bp )                 // also used in main setup()
    {
        char rsp[80];
        myp.tempfound = 0;                                          // initialize max temp sensors and index. Needed in case of DHT
        myp.tempindex = 0;
        if( senstype == SENSOR_DS18 )
        {
            temp.search( true );                                    // initialize the Dallas Thermomete
            myp.tempfound = temp.count();
            sf( rsp, 80, "Found %d DS18B20 sensors\r\n", myp.tempfound );
        }
        else if( senstype == SENSOR_DHT )
        {
            sf( rsp, 80, "DHT assumed\r\n");       
        }
        else if( senstype == SENSOR_HTU)
        {
            if( htu.init() )
            {    sf( rsp, 80, "HTU21 OK\r\n" );       }
            else
            {    sf( rsp, 80, "HTU21 Not found\r\n" ); }
        }
        else
        {
            sf( rsp, 80, "No Sensor\r\n" );       
        }
        if( bp )
            bp->add( rsp );
        else
            PR( rsp );        
    }

// ----------------- 2. Update routines of local CLI and MegunoLink used by loop() -----------------------------

    void updateReadings()        // Updates Meguno display 
    {
        PF("{UI:CONFIG|SET|stream1.Text=");
        if( myp.gp.sensor == SENSOR_DS18 )                                                    // only temp given
        {
            if( myp.tempfound == 1 )
                PF( "Temp: %.1fF (%.1fC)}\r\n", myp.tempF, F_TO_C(myp.tempF) );
            else
                PF( "T1:%.1fF T2:%.1fF (%.1fC,%.1fC)}\r\n", myp.tempF, myp.tempF2, F_TO_C(myp.tempF), F_TO_C(myp.tempF2) );
        }
        else if( (myp.gp.sensor == SENSOR_DHT) || (myp.gp.sensor == SENSOR_HTU)  )
        {
            PF( "Temp: %.1fF (%.1fC), Humidity: %.0f%%}\r\n", myp.tempF, F_TO_C(myp.tempF), myp.humidity );
        }
        else
            PF( "No Sensor}\r\n" );
    }    
    void updateStatus()
    {
        PF("{UI:CONFIG|SET|stream2.Text=");
        PF( "Mode: %d, Relay: %s, Simul: ", myp.gp.tmode, myp.relayON?"ON":"OFF" );
        
        if( myp.simulON )
            PF( "ON (%.1fF)}\r\n", myp.simulT );
        else    
            PF( "OFF}\r\n" );
    }
    void updateGraph()
    {
        if( (myp.gp.sensor == SENSOR_DS18) )
        {
            if( (myp.tempfound == 2) )  
            {
                PF("{TIMEPLOT:GRAPH|D|%s|T|%.1f}\r\n", "Temp1:<", myp.tempF );
                PF("{TIMEPLOT:GRAPH|D|%s|T|%.1f}\r\n", "Temp2:<", myp.tempF2 );
            }
            else
                PF("{TIMEPLOT:GRAPH|D|%s|T|%.1f}\r\n", "Temp:<", myp.tempF );
        }
        if( (myp.gp.sensor == SENSOR_DHT) || (myp.gp.sensor == SENSOR_HTU)  )
        {
            PF("{TIMEPLOT:GRAPH|D|%s|T|%.1f}\r\n", "Temperature:<", myp.tempF );
            PF("{TIMEPLOT:GRAPH|D|%s|T|%.3f}\r\n", "Humidity:>", myp.humidity );
        }
    }
    void updateSelectedDisplays( int code )    // called by the main loop
    {
        if( code & 1 )
            updateReadings();
        if( code & 2 )
            updateStatus();
        if( code & 4 )
            updateGraph();
    }

// ----------------- 3. OLED Initialization and reporting routines -------------------------------

    void initOLED()
    {
        oled.dsp( O_LED096 );        // initialize
    }
    
    void updateOLED( char *status )
    {
        static int line=2;
        oled.dsp( 0,"\vSe%d Md%1d RL%d Si%d", myp.gp.sensor, myp.gp.tmode, myp.relayON, myp.simulON );
        oled.dsp( line++, "\a%s", status );
        if( line > 7 )
            line = 7;
        delay(1000);
        PF("OLED: %s\r\n", status );
    }
    char *spinner()
    {
        static int count;
        switch( (count++) & 3 )
        {
            case 0: return "\t\a-";
            case 1: return "\t\a\\";
            case 2: return "\t\a|";
            case 3: return "\t\a/";
        }
    }
    void updateOLED( )
    {
        if( !myp.gp.oled )              // do nothing if oled is not active
            return;
            
        oled.dsp( 0, "\t\b%5.1f'F", myp.tempF );
        if( myp.gp.sensor == SENSOR_DS18 )
            oled.dsp( 2, "\t\b%5.1f'F", myp.tempF2 );
        else
            oled.dsp( 2, "\t\b%.0f%%", myp.humidity);            
    
        oled.dsp( 4, spinner() );
        oled.dsp( 5,"\aSe:%d Md%1d RL%d Si%d", myp.gp.sensor, myp.gp.tmode, myp.relayON, myp.simulON );
        IPAddress ip = WiFi.localIP();
        oled.dsp( 6,"\a%s", ip.toString().c_str());  
        oled.dsp( 7,"\aRSSI:%ddBm", WiFi.RSSI() );    
    }
    void cliOLED( int n, char **arg )
    {
        BINIT( bp, arg );
        int onoff = (n>=1) ? atoi( arg[1] ) : 0;
        
        myp.gp.oled = onoff;
        myp.saveMyEEParms();
        
        if( onoff )
        {
            initOLED();
            updateOLED("OLED ON");
        }
        else
            oled.clearDisplay();
    }
    void mgnOLED( int n, char **arg )
    {
        cliOLED( n, arg );
        nmp.printMgnInfo( channel, "oled", "updated" );
        nmp.printMgnParm( channel, "oled" );                      // update the table
    }
    
// ----------------- 4. Temperature and Humidity CLI Handlers ------------------------------

    void _setHeatCool( int n, char **arg, thermode_t mode1 )             // sets mode to HEAT or COOL and new target temp
    {
        BINIT( bp, arg );
        if( n<=1 )                  
        {
            RESPONSE("? Missing arg\r\n");
            return;
        }
        char *stemp = arg[1];
        char c = stemp[ strlen(stemp)-1 ];                  // get last character
        float temp = atof( stemp );                         // get user target
        if( (c=='C') || (c=='c') )
            temp = C_TO_F( temp );                            // always in F
 
        myp.gp.threshold = temp;
        if( n>2 ) 
            myp.gp.delta = atof( arg[2] );
        myp.gp.tmode = mode1;
        myp.saveMyEEParms();                                // save tmode, threshold, delta
        RESPONSE( "Mode:%d Target:%.1fF (%.1fC)\r\n", mode1, temp, F_TO_C( temp ) );
    }
    void cliSetHeat( int n, char **arg ) 
    {
        BINIT( bp, arg );
        if( n<=1 )                  
        {
            RESPONSE("? Missing arg\r\n");
            return;
        }
        _setHeatCool( n, arg, HEAT_MODE );
    }
    void cliSetCool( int n, char **arg ) 
    {
        BINIT( bp, arg );
        if( n<=1 )                  
        {
            RESPONSE("? Missing arg\r\n");
            return;
        }
        _setHeatCool( n, arg, COOL_MODE );
    }
    void cliSetRelay( int n, char **arg )                         // set relay, forcing mode=0 
    {
        BINIT( bp, arg );
        bool onoff = (n>=1)? atoi( arg[1] ) : 0;

        digitalWrite( RELAY, onoff );
        myp.relayON    = onoff;
        myp.gp.tmode   = RELAY_CONTROL;
        myp.saveMyEEParms();                                      // save to EEPROM
        RESPONSE("Relay %s\r\n", onoff?"ON":"OFF" );
    }
    void mgnSetRelay( int n, char **arg )                         // simulate deg C 
    {
        cliSetRelay( n, arg );
        nmp.printMgnInfo( channel, "tmode", "(ON/OFF relay) Updated" );
        nmp.printMgnParm( channel, "tmode" );                      // update the table
        updateStatus();
    }
    void mgnSetHeat( int n, char **arg )                     // sets mode to HEAT and new target temp
    {
        cliSetHeat( n, arg );
        nmp.printMgnInfo( channel, "target", "(heating) updated" );    // update table
        nmp.printMgnParm( channel, "tmode" );                
        nmp.printMgnParm( channel, "target" ); 
        nmp.printMgnParm( channel, "delta" );
    }
    void mgnSetCool( int n, char **arg )                     // sets mode to HEAT and new target temp
    {
        cliSetCool( n, arg );
        nmp.printMgnInfo( channel, "target", "(cooling) updated" );    // update table
        nmp.printMgnParm( channel, "tmode" );                
        nmp.printMgnParm( channel, "target" ); 
        nmp.printMgnParm( channel, "delta" );
    }
    void cliSetSimul( int n, char **arg )                          // simulate deg C or deg F
    {
        BINIT( bp, arg );
        if( n<=1 )                  
        {
            RESPONSE("? Missing arg\r\n");
            return;
        }
        char *stemp = arg[1];
        if( (strcmp( stemp, "OFF" )==0) ||
            (strcmp( stemp, "off" )==0) )
        {
            myp.simulON = false;
            RESPONSE( "Simulation OFF\r\n" );
        }
        else
        {
            char c = stemp[ strlen(stemp)-1 ];                  // get last character
            float temp = atof( stemp );                         // get user target
            if( (c=='C') || (c=='c') )
                temp = C_TO_F( temp );                            // temp is in F, unless C is given
    
            myp.simulON = true;
            myp.simulT = temp;
            RESPONSE( "Simulation at %.1f°F (%.1f°C)\r\n", temp, F_TO_C( temp ) );
        }
    }
    void mgnSetSimul( int n, char **arg ) 
    {
        cliSetSimul( n, arg );
        updateStatus();
    }
    void mgnShowReadings( int n, char **arg )                    // display readings
    {
        updateReadings();
        updateStatus();
    }  
    void cliFilter( int n, char **arg )
    {
        BINIT( bp, arg );
        int pd = 0;
        if( n>=1 )
            pd = atoi( arg[1] );
        
        initFilters( pd );
        myp.gp.filter = pd;
        
        RESPONSE( "Prop delay set to %d-samples\r\n", pd );
        myp.saveMyEEParms();
    }   
    void mgnFilter( int n, char **arg )
    {
        cliFilter( n, arg );
        nmp.printMgnInfo( channel, "filter", "updated" );    // update table
        nmp.printMgnParm( channel, "filter" );                
    }   
    void cliSetSensor( int n, char **arg )
    {
        BINIT( bp, arg );
                
        myp.gp.sensor = (n>=1) ? (sensor_t) atoi( arg[1] ) : SENSOR_NONE;
                    
        initSensors( myp.gp.sensor, bp );
        myp.saveMyEEParms(); 
    }
    void mgnSetSensor( int n, char **arg )
    {
        cliSetSensor( n, arg );
        nmp.printMgnInfo( channel, "sensor", "updated" );     
        nmp.printMgnParm( channel, "sensor" );
    }      
    void mgnSetStreaming( int n, char **arg )
    {
        if( n>1 )
            myp.gp.stream = atoi( arg[1] );
    }

// ----------------- 5. CLI command table ----------------------------------------------
                     
    CMDTABLE mypTable[]= 
    {
        {"filter",  "N. Set IIR filter to 90% at N-samples. 0=disable", cliFilter     },
        {"!filter", "",                                                 mgnFilter     },
        
        {"oled",    "0|1. Activate/deactivate OLED",                    cliOLED       },
        {"!oled",   "",                                                 mgnOLED       },

        {"heat",    "<tempF or C> [delta]. Sets HEATING mode & temp",   cliSetHeat    },
        {"!heat",   "",                                                 mgnSetHeat    },
        {"cool",    "<tempF or C> [delta]. Sets COOLING mode & temp",   cliSetCool    },
        {"!cool",   "",                                                 mgnSetCool    },
        {"relay",   "0|1 set relay ON/OFF (forces tmode=0)",            cliSetRelay   },
        {"!relay",  "",                                                 mgnSetRelay   },

        {"simul",   "<tempF or C| OFF>. Simulate temperature",          cliSetSimul   },
        {"!simul",  "",                                                 mgnSetSimul   },

        {"sensor",  "0=None 1=DS18 2=DHT 3=HTU. Select sensor type",    cliSetSensor  },
        {"!sensor", "",                                                 mgnSetSensor  },
        
        {"!stream", "0=stop 1=enable text 2=graph. Stream mask",        mgnSetStreaming },
        {"!read",   "Shows measurement state",                          mgnShowReadings },

 
        {NULL, NULL, NULL}
    };

 #undef RESPONSE
 
