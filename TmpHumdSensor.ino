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

 // >>>>>>>>>>>>> SELECT CPU TYPE in myiGlobals.h <<<<<<<<<<<<<<<<<<<
 
    #include <FS.h>
    #include <bufClass.h>       // in GKE-L1
    #include <ticClass.h>       // in GKE-L1
    
    #include "SimpleSRV.h"      // in GKE-Lw
    #include "SimpleSTA.h"      // in GKE-Lw
    
    #include "myGlobals.h"      // in this project. This includes externIO.h
    #include "myEndPoints.h"
    #include "commonCLI.h"
    #include "myCliHandlers.h"

//------------------ References and Class Allocations ------------------------

    CPU cpu;                    // declared by externIO.h
    CLI cli;
    EXE exe;
    EEP eep;
    
//  FILT filter;

    ESP8266WebServer server( 80 );
    
    BUF buffer( 1024+256 );     // buffer to hold the response from the local or remote CLI commands.

//------------------------- FORWARD REFERENCES -------------------------------
    
//   char *help = "Temperature/humidity reader\r\n";   

//    void blinkLEDAsync( int ms, uint32_t dly=0 );
//    bool startCLIAfter( int ms );
    void relayControl( float T );
    void doTempReady( float C );
    void checkButton();

// ----------------------------- Main Setup -----------------------------------

void setup() 
{
    cpu.init( 115200, MYLED+NEGATIVE_LOGIC /* LED */, BUTTON+NEGATIVE_LOGIC /* push button */ );
    
    pinMode(RELAY, OUTPUT); 
    digitalWrite( RELAY, LOW );                 // positive logic for SONOFF

//    pinMode( 14, OUTPUT);                       // why this??
//    digitalWrite( 14, HIGH );
    
    ASSERT( SPIFFS.begin() );                   // start the filesystem

    myp.initAllParms( MAGIC_CODE );             // initialize volatile & user EEPROM parameters
    exe.registerTable( cmnTable );              // register common CLI tables
    exe.registerTable( mypTable );              // register tables to CLI
        
    startCLIAfter( 10/*sec*/, &buffer );        // this also initializes cli(). See SimpleSTA.cpp         
    
    myp.tempfound = 0;                          // initialize max temp sensors and index. Needed in case of DHT
    myp.tempindex = 0;
    if( myp.gp.sensor == SENSOR_DS18 )
    {
        temp.search( true );                    // initialize the Dallas Thermomete
        myp.tempfound = temp.count();
        PF("Found %d DS18B20 sensors\r\n", myp.tempfound );
    }
    if( myp.gp.sensor == SENSOR_HTU )
        if( htu.init() )
            PF("Found HTU21D sensor\r\n" );
    
//  filter.setPropDelay( myp.gp.prdelay );              // propagation delay 

    setupSTA( 30 );                                     // WiFi STA Setup 
    srvCallbacks( server, Landing_STA_Page );           // standard WEB callbacks. "staLanding" is /. HTML page
    cliCallbacks( server, buffer );                     // enable WEB CLI with buffer specified
    snfCallbacks();
    
    setTrace( T_REQUEST | T_JSON );                     // default trace    
    server.begin( eep.wifi.port );                      // start the server
    PRN("HTTP server started.");    
    
    cli.prompt();
}

TICsec ticDS18( 2 );                                       // used for DS18
TICsec ticDHT ( 5 );                                       // used for DHT

void loop()
{
//    if( WiFi.status() != WL_CONNECTED  )
//        cpu.toggleEvery( 250 );
//    else
        server.handleClient(); 
    
    checkButton();                                      // toggle relay of button is pressed
        
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
                doTempReady( myp.simulT );            
            else
                temp.start( myp.tempindex );             // either 0 or 1
                
            cpu.led( OFF );
        }
        if( temp.ready() )
        {
            if( temp.success(true) )
            {
                float C = temp.getDegreesC();
                
                if( myp.tempindex ^ myp.gp.flip )       // reverse the order if flip is set
                    myp.tempC2    = C;                  // read C and C2 based on the index
                else
                    myp.tempC     = C;

                myp.tempindex = temp.nextID();         // if multiple sensors, get the next one
                
                myp.humidity = -1.0;                   // humidity of <0.0 indicates temp measurement only    
                relayControl( ctof(C) );               // decide what to do with the relay                        
                mgnStream();                           // report measurements if streaming is enabled
            }
            else
                PF("Temp Reading Error\r\n");
        }            
    }
    if( myp.gp.sensor == SENSOR_DHT )                           // humidity
    {
        if( ticDHT.ready() )
        {
            float C, H;
            int err;
            cpu.led( ON );
    
            if( myp.simulON )                                   // simulated temperature
            {
                C = ftoc( myp.simulT );
                H = 10.0;
                err = SimpleDHTErrSuccess;
            }
            else                                                // actual reading
            {
                noInterrupts();
                err = dht22.read2( &C, &H, NULL);
                interrupts();
            }
            myp.tempC       = C; // filter.smooth( C );
            myp.tempC2      = 0.0;
            myp.humidity    = H;
    
            if( err == SimpleDHTErrSuccess )
            {
                relayControl( ctof(C) );                              // decide what to do with the relay        
                mgnStream();   
            }
            else
                PF("Error 0x%04x (delay=%d, err=0x%02x)\r\n", err, (err>>8), (err & 0xFF) );
            
            cpu.led( OFF );
        }
    }
    if( myp.gp.sensor == SENSOR_HTU )                           // temp-humidity using DHU21D
    {
        if( ticDS18.ready() )
        {
            float C, H;
            int err;
            cpu.led( ON );
    
            if( myp.simulON )                                   // simulated temperature
            {
                C = ftoc( myp.simulT );
                H = 10.0;
            }
            else                                                // actual reading
            {
                C = htu.readTemperature();
                H = htu.readHumidity();
            }
            myp.tempC       = C; // filter.smooth( C );
            myp.tempC2      = 0.0;
            myp.humidity    = H;

            relayControl( ctof(C) );                            // decide what to do with the relay        
            mgnStream(); 
            cpu.led( OFF );
        }
    }
}

// -------------------------- RELAY CONTROL --------------------------------------------    

    void doTempReady( float F )                       // simulate tempC and tempC2
    {   
        myp.tempC    = ftoc(F);                       // filter.smooth( C );
        myp.tempC2   = ftoc(F);                       
        myp.humidity = 0.0;
        mgnStream();                                  // stream if streaming is enabled
        
        relayControl( F );                            // decide what to do with the relay                
    }
    void checkButton()
    {
        static bool toggle = false;
        static uint32_t T0;
        if( cpu.button() )                            // button pressed
        {
            if( (millis()-T0)<1000 )                   // no more than once per second
                return;
            T0 = millis();
            
            toggle = !toggle;            
            digitalWrite( RELAY, myp.relayON = toggle );
            PF("Button set relay %s\r\n", toggle?"ON":"OFF" );
        }
    }
    void relayControl( float F )
    {
        switch (myp.gp.tmode )
        {
            case RELAY_CONTROL:                       // let user change the relay
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
    
