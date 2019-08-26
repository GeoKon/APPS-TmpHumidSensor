/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
// Minimum necessary for this file to compile

#include <FS.h>
#include <externIO.h>       // IO includes and cpu...exe extern declarations
#include "myGlobals.h"
#include "commonCLI.h"
#include "myCliHandlers.h"

// ------------------------------ Streaming & Show measurements -----------------------------

void updateDisplayStatus()
{
    PF("{UI:CONFIG|SET|stream2.Text=");
    PF( "Mode: %d, Relay: %s, Simul: ", myp.gp.tmode, myp.relayON?"ON":"OFF" );
    
    if( myp.simulON )
        PF( "ON (%.1fF)}\r\n", myp.simulT );
    else    
        PF( "OFF}\r\n" );
}
void updateDisplayReadings()
{
    PF("{UI:CONFIG|SET|stream1.Text=");
    if( myp.gp.sensor == SENSOR_DS18 )                                                    // only temp given
    {
        if( myp.tempfound == 1 )
            PF( "Temp: %.1fF (%.1fC)}\r\n", ctof(myp.tempC), myp.tempC );
        else
            PF( "T1:%.1fF T2:%.1fF (%.1fC,%.1fC)}\r\n", ctof(myp.tempC), ctof(myp.tempC2), myp.tempC, myp.tempC2 );
    }
    else if( (myp.gp.sensor == SENSOR_DHT) || (myp.gp.sensor == SENSOR_HTU)  )
        PF( "Temp: %.1fF (%.1fC), Humidity: %.0f%%}\r\n", ctof(myp.tempC), myp.tempC, myp.humidity );
    else
        PF( "No Sensor}\r\n" );
}
void mgnStream()                        // called by the main loop
{
    if( !myp.gp.stream )                // do nothing is streaming is not enabled
        return;
    updateDisplayReadings();
    updateDisplayStatus();
}
    

    // ============================== TEMP or HUMIDITY SPECIFIC CLI ===============================
    
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
            temp = ctof( temp );                            // always in F
 
        myp.gp.threshold = temp;
        if( n>2 ) 
            myp.gp.delta = atof( arg[2] );
        myp.gp.tmode = mode1;
        myp.saveMyEEParms();                                // save tmode, threshold, delta
        RESPONSE( "Mode:%d Target:%.1fF (%.1fC)\r\n", mode1, temp, ftoc( temp ) );
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
        updateDisplayStatus();
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
                temp = ctof( temp );                            // temp is in F, unless C is given
    
            myp.simulON = true;
            myp.simulT = temp;
            RESPONSE( "Simulation at %.1f°F (%.1f°C)\r\n", temp, ftoc( temp ) );
        }
    }
    void mgnSetSimul( int n, char **arg ) 
    {
        cliSetSimul( n, arg );
        updateDisplayStatus();
    }
    void mgnShowReadings( int n, char **arg )                    // display readings
    {
        updateDisplayReadings();
        updateDisplayStatus();
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
   
    void cliSetSensor( int n, char **arg )
    {
        BINIT( bp, arg );
        
        myp.gp.sensor = (n>=1) ? (sensor_t) atoi( arg[1] ) : SENSOR_NONE;
        myp.tempfound = 0;
        myp.tempindex = 0;
                    
        if( myp.gp.sensor == SENSOR_DS18 )
        {
            temp.search( true );                                    // initialize the Dallas Thermomete
            myp.tempfound = temp.count();
            RESPONSE( "DS18 x%d found\r\n", myp.tempfound ); 
        }
        else if( myp.gp.sensor == SENSOR_DHT )
        {
            RESPONSE( "DHT assumed\r\n");       
        }
        else if( myp.gp.sensor == SENSOR_HTU)
        {
            if( htu.init() )
            {    RESPONSE( "HTU21 OK\r\n" );       }
            else
            {    RESPONSE( "HTU21 Not found\r\n" ); }
        }
        else
        {
            RESPONSE( "No Sensor\r\n" );       
        }
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

// ============================== CLI COMMAND TABLE =======================================
                     
    CMDTABLE mypTable[]= 
    {
        {"filter",  "N. Define filter of N-samples",                    cliFilter      },

        {"heat",  "<tempF or C> [delta]. Sets HEATING mode & temp",     cliSetHeat    },
        {"!heat",  "",                                                  mgnSetHeat    },
        {"cool",  "<tempF or C> [delta]. Sets COOLING mode & temp",     cliSetCool    },
        {"!cool",  "",                                                  mgnSetCool    },
        {"relay", "0|1 set relay ON/OFF (forces tmode=0)",              cliSetRelay   },
        {"!relay", "",                                                  mgnSetRelay   },

        {"simul",  "<tempF or C| OFF>. Simulate temperature",           cliSetSimul   },
        {"!simul", "",                                                  mgnSetSimul   },

        {"!sensor",         "0=None 1=DS18 2=DHT 3=HTU. Select sensor type",    mgnSetSensor  },
        
        {"!stream",         "0|1 Enable streaming of measurements",               mgnSetStreaming },
//        {"!status",         "Displays wifi status",                               mgnGetStatus },
        {"!read",           "Shows measurement state",                            mgnShowReadings },
 
        {NULL, NULL, NULL}
    };
