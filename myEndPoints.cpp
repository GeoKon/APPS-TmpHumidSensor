/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */

#include "SimpleSRV.h"      // in GKE-Lw

#include "myGlobals.h"      // in this project
#include "myEndPoints.h"    // in this project
        
// ---------------- Local variable/class allocation ---------------------

extern ESP8266WebServer server;
    
// ------------- main Callbacks (add your changes here) -----------------

void snfCallbacks( )
{
    server.on("/tstat",
    [](){
        showArgs();
        BUF json(256);
       
        json.set("{'type':%d, 'count':%d, 'temp':%.1f, 'temp2':%.1f, 'tmode':%d, 't_heat':%.1f, 'tstate':%d, 'hold':%d, 'humidity':%.0f}", 
            /* type */   myp.gp.sensor,
            /* count */  myp.tempfound,
            /* temp */   C_TO_F( myp.tempC ),
            /* temp2 */  C_TO_F( myp.tempC2 ),
            /* tmode */  myp.gp.tmode, 
            /* t_heat */ myp.gp.threshold,
            /* tstate */ myp.relayON, 
            /* hold */   myp.simulON,
            /* humidity*/ myp.humidity            
            );            

        json.quotes();
        showJson( json.c_str() );
        server.send(200, "application/json", json.c_str() );
    });
    server.on("/tstat/humidity",
    [](){
        showArgs();
        BUF json(256);
        
        json.set("{'humidity':%.0f}", myp.humidity );
        json.quotes();
        showJson( json.c_str() );
        server.send(200, "application/json", json.c_str() );
    });
    server.on("/tstat/model",
    [](){
        showArgs();
        BUF json(256);  
              
        json.set("{'model':'%s'}", WiFi.hostname().c_str() );
        json.quotes();
        showJson( json.c_str() );
        server.send(200, "application/json", json.c_str() );
    });
    server.on("/sys/network",
    [](){
        showArgs();
        BUF json(256);
        
        uint8_t m[6];
        WiFi.macAddress( m );
        IPAddress ipa; 
        ipa = WiFi.localIP();        
                
        json.set("{'ssid':%s, 'bssid':'%02x:%02x:%02x:%02x:%02x:%02x', 'channel':%d, 'rssi':%d, 'ipaddr':'%s' }", 
        /* ssid */   WiFi.SSID().c_str(),
        /* bssid */  m[0],m[1],m[2],m[3],m[4],m[5], 
        /* channel */ WiFi.channel(),
        /* rssi */   WiFi.RSSI(),
        /* ipaddr */ ipa.toString().c_str() );
        
        json.quotes();
        showJson( json.c_str() );
        server.send(200, "application/json", json.c_str() );
    });
    // this is redundant to /cli:cmd=cmd+arg+arg
    server.on("/set", HTTP_GET, 
    [](){
        showArgs();
        BUF resp(128);
        BUF final(128);
            
        if( server.args() )                              // if variable is given
        {
            char cmd[80];
            sprintf( cmd, "set %s %s", !server.argName(0), !server.arg(0) );

            resp.init();                      
            exe.dispatchBuf( cmd, resp );          // command is executed here and RESPONSE1 is saved in 'resp' buffer

            final.set("<h3 align='center'>\r\n");
            final.add("%s</h3>\r\n", !resp );
            final.add( navigate );
        }
        showJson( !final );                           // show the same on the console
        server.send(200, "text/html", !final );
    });
    server.on("/show", HTTP_GET, 
    [](){
        showArgs();
        BUF s(512);

        s.set("<h3 align='center'>\r\n");

        for( int i=0; i<nmp.getParmCount(); i++ )
            s.add("%s = %s<br/>\r\n", nmp.getParmName(i), nmp.getParmValueStr(i) );

        s.add("<br/>(Use '/set?parm=value' to modify)<br/>");
        s.add("</h3>\r\n");
        s.add( navigate );
        showJson( !s );                              
        server.send(200, "text/html", !s );
    });
}
