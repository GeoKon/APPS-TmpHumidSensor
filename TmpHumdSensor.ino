/* ----------------------------------------------------------------------------------
 *  Converts the SONOFF Basic switch V2 to a thermostat
 *  Presents webpage with 
 *        -- Temperature readings
 *        -- Thermostat controls
 *        -- Simulations
 *  Use WEB CLI to modify parameters.
 *  Compatible with the REST API of Radio Thermostat CT-50 and CT-80
 *   
 *  Boot sequence:
 *      1. fast blink 100/100 waiting for SERIAL connection
 *      2. slower blink 5sec waiting for WiFi connection
 * 
 * Sketch uses 346912 bytes (45%) of program storage space. Maximum is 761840 bytes.
 * Global variables use 36424 bytes (44%) of dynamic memory, leaving 45496 bytes for local variables. Maximum is 81920 bytes.
 * Uploading 351056 bytes
 *  
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 *  ---------------------------------------------------------------------------------
 */
 // SELECT CPU TYPE and SENSOR TYPE in myiGlobals.h!
 
    #include <FS.h>
    #include <bufClass.h>       // in GKE-L1
    #include <ticClass.h>       // in GKE-L1
    
    #include "eepTable.h"       // in GKE-Lw
    #include "SimpleSRV.h"      // in GKE-Lw
    #include "SimpleSTA.h"      // in GKE-Lw
    
    #include "myGlobals.h"      // in this project. This includes externIO.h
    #include "myEndPoints.h"
    #include "myCliHandlers.h"

//------------------ References and Class Allocations ------------------------

    CPU cpu;                    // declared by externIO.h
    CLI cli;
    EXE exe;
    EEP eep;
    
//    FILT filter;

    ESP8266WebServer server( 80 );
    
    BUF buffer( 1024+256 );     // buffer to hold the response from the local or remote CLI commands.

//------------------------- FORWARD REFERENCES -------------------------------
    
    char *help = "Temperature/humidity reader\r\n";   

    void blinkLEDAsync( int ms, uint32_t dly=0 );
    void startCLIAfter( int ms );
    void relayControl( float T );
    void doTempReady( float C );

// ----------------------------- Main Setup -----------------------------------

void setup() 
{
    cpu.init( 115200, MYLED+NEGATIVE_LOGIC /* LED */, BUTTON+NEGATIVE_LOGIC /* push button */ );

    pinMode(RELAY, OUTPUT); 
    digitalWrite( RELAY, LOW );                 // positive logic for SONOFF

    pinMode( 14, OUTPUT); 
    digitalWrite( 14, HIGH );
    
    ASSERT( SPIFFS.begin() );                   // start the filesystem

    myp.initAllParms( /*Magic Code*/0x1234 );   // initialize volatile & user EEPROM parameters
    
    exe.registerTable( mypTable );              // register tables to CLI
    exe.registerTable( eepTable );
        
    startCLIAfter( 10/*sec*/ );                 // this also initialized cli()          
    
#ifdef DS18B20
    temp.search( );                             // initialize the Dallas Thermomete
    PF("Found %d sensors\r\n", temp.count() );
#endif
    
//  filter.setPropDelay( myp.gp.prdelay );              // propagation delay 

    setupSTA();                                         // WiFi STA Setup 
    srvCallbacks( server, Landing_STA_Page );           // standard WEB callbacks. "staLanding" is /. HTML page
    cliCallbacks( server, buffer );                     // enable WEB CLI with buffer specified
    snfCallbacks();
    
    setTrace( T_REQUEST | T_JSON );                     // default trace    
    server.begin( 80 );                                 // start the server
    PRN("HTTP server started.");    
    
    cli.prompt();
}

#ifdef DS18B20
    TICsec tic(2);                                      // one measurement per 2-seconds
#else
    TICsec tic(5);                                      // one measurement per 5-seconds
#endif

void loop()
{
    if( WiFi.status() != WL_CONNECTED  )
        cpu.toggleEvery( 250 );
    else
        server.handleClient(); 
        
    if( cli.ready() )                                   // handle serial interactions
    {
        exe.dispatchConsole( cli.gets() ); 
        cli.prompt();
        if( myp.gp.stream )
            CRLF();
    }
#ifdef DS18B20
    if( tic.ready() )
    {
        cpu.led( ON );
        if( myp.simulON )
            doTempReady( myp.simulT );            
        else
            temp.start( 0 ); 
        cpu.led( OFF );
    }
    if( temp.ready() )
    {
        if( temp.success(true) )
            doTempReady( temp.getDegreesC() );              // save in local variables
        else
            PF("Temp Reading Error\r\n");
    }            
#endif
#ifdef DHT22
    if( tic.ready() )
    {
        float C, H;
        int err;
        cpu.led( ON );

        if( myp.simulON )               // simulated temperature
        {
            C = myp.simulT;
            H = 10.0;
            err = SimpleDHTErrSuccess;
        }
        else                            // actual reading
        {
            noInterrupts();
            err = dht22.read2( &C, &H, NULL);
            interrupts();
        }
        myp.tempC  = C; // filter.smooth( C );
        myp.tempF  = C_TO_F( myp.tempC );
        myp.humidity = H;

        if( err != SimpleDHTErrSuccess )
            PF("Error 0x%04x (delay=%d, err=0x%02x)\r\n", err, (err>>8), (err & 0xFF) );
        else
        {
            if( myp.gp.stream )
                PF("Relay:%d, Temp:%5.1f°C (%5.1f°F) Humidity:%.0f%%\r\n", 
                    myp.relayON, myp.tempC, myp.tempF, myp.humidity );
        }
        void relayControl( float T );       // forward reference
        relayControl( C );
        cpu.led( OFF );
    }
#endif
}

// -------------------------- RELAY CONTROL --------------------------------------------    

    void relayControl( float T )
    {
        switch (myp.gp.tmode )
        {
            case NO_RELAY:
                digitalWrite( RELAY, LOW );
                myp.relayON = false;
                break;
    
            case COOL_MODE:
                if( T >= myp.gp.threshold )
                    digitalWrite( RELAY, myp.relayON = true);
                if( T < myp.gp.threshold-myp.gp.delta )
                    digitalWrite( RELAY, myp.relayON = false);
                break;
    
            case HEAT_MODE:
                if( T <= myp.gp.threshold )
                    digitalWrite( RELAY, myp.relayON = true );
                if( T > myp.gp.threshold+myp.gp.delta )
                    digitalWrite( RELAY, myp.relayON = false);
                break;
        }
    }
    void doTempReady( float C )
    {
        myp.tempC    = C; // filter.smooth( C );
        myp.tempF    = C_TO_F( C );                    // save in local variables
        myp.humidity = 0.0;
        
        if( myp.gp.stream )
            PF("Relay:%d, Temp:%5.1f°C (%5.1f°F) Humidity:%.0f%%\r\n", 
                    myp.relayON, myp.tempC, myp.tempF, myp.humidity );

        relayControl( C );                            // decide what to do with the relay                
    }
