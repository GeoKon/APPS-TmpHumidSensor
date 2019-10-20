/* ----------------------------------------------------------------------------------
 *  Converts the SONOFF Basic switch V2 to a thermostat. Also compatible with NodeMCU
 *  Presents webpage with 
 *        -- Temperature readings
 *        -- Thermostat controls
 *        -- Simulations
 * 
 *  Use Meguno interface via serial port.
 *  Use WEB to display measurements remotely.
 *  Compatible with the REST API of Radio Thermostat CT-50 and CT-80
 *   
 *  Boot sequence:
 *      1. fast blink 100/100 waiting for SERIAL connection
 *      2. slower blink 5sec waiting for WiFi connection
 * 
 * NODEMCU: 374200 bytes (35%) of program storage space. Maximum is 1044464 bytes
 * Global variables use 37548 bytes (45%) of dynamic memory, leaving 44372 bytes 
 * for local variables. Maximum is 81920 bytes. Uploading 378352 bytes
 * 
 * Sketch uses 346912 bytes (45%) of program storage space. Maximum is 761840 bytes.
 * Global variables use 36424 bytes (44%) of dynamic memory, leaving 45496 bytes for 
 * local variables. Maximum is 81920 bytes. Uploading 351056 bytes
 *  
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 *  ---------------------------------------------------------------------------------
 */

 // >>>>>>>>>>>>> SELECT CPU TYPE in myGlobals.h <<<<<<<<<<<<<<<<<<<
 
    #include <FS.h>
    #include <bufClass.h>       // in GKE-L1
    #include <ticClass.h>       // in GKE-L1
    #include <iirClass.h>
    
    #include "SimpleSRV.h"      // in GKE-Lw
    #include "SimpleSTA.h"      // in GKE-Lw
    #include "CommonCLI.h"      // in GKE-Lw
                                
    #include "myGlobals.h"      // in this project. This includes externIO.h
    #include "myEndPoints.h"
    #include "myCliHandlers.h"

//------------------ References and Class Allocations ------------------------

    CPU cpu;                    // declared by externIO.h
    CLI cli;
    EXE exe;
    EEP eep;

    ESP8266WebServer server( 80 );
    
    BUF buffer( 1024+256 );     // buffer to hold the response from the local or remote CLI commands.

//------------------------- FORWARD REFERENCES -------------------------------
    
    bool buttonPressReady();
    void setTempHumidity( int type, float X1, float X2 );
    void setRelayHeatCool( float F );
    
// ----------------------------- Main Setup -----------------------------------

void setup() 
{
    int je = setjmp( myp.env );                     // restart point
    cpu.init( 115200, MYLED+NEGATIVE_LOGIC /* LED */, BUTTON+NEGATIVE_LOGIC /* push button */ );
    
    pinMode(RELAY, OUTPUT); 
    digitalWrite( RELAY, LOW );                 // positive logic for SONOFF

//    pinMode( 14, OUTPUT);                       // why this??
//    digitalWrite( 14, HIGH );
    
    ASSERT( SPIFFS.begin() );                   // start the filesystem

    myp.initAllParms();                         // initialize volatile & user EEPROM parameters
    
    linkParms2cmnTable( &myp );                // cast to (Global *) is not required
    exe.registerTable( cmnTable );              // register common CLI tables
    exe.registerTable( mypTable );              // register tables to CLI

    initOLED();                                 // initialize OLED. 
    updateOLED( "Waiting for CLI");  
    startCLIAfter( 10/*sec*/, &buffer );        // this also initializes cli(). See SimpleSTA.cpp    

    updateOLED( "Connecting STA");              // Leave this as is. OneWire conflicts with I2C
    setupWiFi ( );                             // WiFi STA Setup 

    updateOLED("Starting Server");    
    srvCallbacks( server, Landing_STA_Page );   // standard WEB callbacks. "staLanding" is /. HTML page
    cliCallbacks( server, buffer );             // enable WEB CLI with buffer specified
    snfCallbacks();    
    setTrace( T_REQUEST | T_JSON );             // default WEB trace    
    server.begin( eep.wifi.port );              // start the server

    updateOLED("Init Sensors");    
    
    initFilters( myp.gp.filter );               // initialize smoothing filters (see CLI Handlers)
    initSensors( myp.gp.sensor );               // initialize sensors
    
    cli.prompt();
}

TICsec ticDS18( 2 );                                    // used for DS18
TICsec ticDHT ( 5 );                                       // used for DHT

void loop()
{
//    if( WiFi.status() != WL_CONNECTED  )
//        cpu.toggleEvery( 250 );
//    else
        server.handleClient(); 
    
     if( cpu.buttonPressed() )                       // check if button is pressed (and released afterwards)
     {
        myp.relayON    = !myp.relayON;            
        digitalWrite( RELAY, myp.relayON );
        if( myp.gp.tmode != RELAY_CONTROL )
        {
            myp.gp.tmode   = RELAY_CONTROL;             // force mode to be relay_control. Not heat or cool
            myp.saveMyEEParms();
        }
        updateSelectedDisplays( 2 );                     // update status line
       // PF("Button set relay %s\r\n", myp.relayON?"ON":"OFF" ); 
    }
    if( cli.ready() )                                   // handle serial interactions
    {
        //exe.dispatchConsole( cli.gets() );            // if Meguno is not used
        
        exe.dispatchBuf( cli.gets(), buffer );          // required by Meguno
        buffer.print();
        
        cli.prompt();
        if( myp.gp.stream )
            CRLF();
    }
    if( myp.gp.sensor == SENSOR_DS18 )                  // thermometer
    {
        if( ticDS18.ready() )
        {
            cpu.led( ON );            
                        
            if( myp.simulON )                           
                setTempHumidity( SENSOR_DS18, myp.simulT, myp.simulT  );
            else
                temp.start( myp.tempindex );                // myp.tempindex is either 0 or 1
                
            cpu.led( OFF );
        }
        if( temp.ready() )                                  // this will become true every time a sensor is ready
        {
            if( temp.success(true) )
            {
                float F = C_TO_F( temp.getDegreesC() );
                static float F1, F2;
                
                if( myp.tempindex ^ myp.gp.flip )           // reverse the order if flip is set
                    F2 = F;                                 // F1 is the previous reading  
                else
                    F1 = F;

                if( myp.tempfound == 1 )                    // one sensor
                    setTempHumidity( SENSOR_DS18, F1, F1 );
                else                                        // two sensors
                    if( myp.tempindex == 1 )
                        setTempHumidity( SENSOR_DS18, F1, F2 );

                myp.tempindex = temp.nextID();              // if multiple sensors, get the next one
            }
            else
                PF("Temp Reading Error\r\n");
        }            
    }
    if( myp.gp.sensor == SENSOR_DHT )                       // humidity
    {
        if( ticDHT.ready() )
        {
            float F, H;
            int err;
            cpu.led( ON );
    
            if( myp.simulON )                               // simulated temperature
            {
                F = myp.simulT;
                H = 0.0;
                err = SimpleDHTErrSuccess;
            }
            else                                            // actual reading
            {
                float C;
                noInterrupts();
                err = dht22.read2( &C, &H, NULL);
                interrupts();
                F = C_TO_F( C );
            }
            if( err == SimpleDHTErrSuccess )
                setTempHumidity( SENSOR_DHT, F, H ); 
            else
                PF("Error 0x%04x (delay=%d, err=0x%02x)\r\n", err, (err>>8), (err & 0xFF) );
            
            cpu.led( OFF );
        }
    }
    if( myp.gp.sensor == SENSOR_HTU )                       // temp-humidity using DHU21D
    {
        if( ticDS18.ready() )
        {
            float F, H;
            cpu.led( ON );
    
            if( myp.simulON )                               // simulated temperature
            {
                F = myp.simulT;
                H = 0.0;
            }
            else                                            // actual reading
            {
                F = C_TO_F( htu.readTemperature() );
                H = htu.readHumidity();
            }
            setTempHumidity( SENSOR_HTU, F, H );
            cpu.led( OFF );
        }
    }
}

// -------------------------- RELAY CONTROL --------------------------------------------    
    
    void setTempHumidity( int type, float F, float X )      // X can be either humidity or 2nd temp
    {
        if( type == SENSOR_DS18 )
        {
            myp.tempF       = iir1.filter( F );
            myp.tempF2      = iir2.filter( X );
            myp.humidity    = 0.0;          
        }
        else
        {
            myp.tempF       = iir1.filter( F );
            myp.tempF2      = myp.tempF;
            myp.humidity    = iir3.filter( X );            
        }
        setRelayHeatCool( F );                              // decide what to do with the relay        
        updateOLED();                      
        updateSelectedDisplays( myp.gp.stream ); 
    }
    
    void setRelayHeatCool( float F )
    {
        switch (myp.gp.tmode )
        {
            case RELAY_CONTROL:                             // let user change the relay
//              digitalWrite( RELAY, LOW );
//              myp.relayON = false;
                break;
            
            case COOL_MODE:
                if( F >= myp.gp.threshold )
                    digitalWrite( RELAY, myp.relayON = true);
                if( F < myp.gp.threshold-myp.gp.delta )
                    digitalWrite( RELAY, myp.relayON = false);
                break;
    
            case HEAT_MODE:
                if( F <= myp.gp.threshold )
                    digitalWrite( RELAY, myp.relayON = true );
                if( F > myp.gp.threshold+myp.gp.delta )
                    digitalWrite( RELAY, myp.relayON = false);
                break;
        }
    }
    
