/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */

#include "nmpClass.h"
#include "myGlobals.h"

    Global  myp;					// Allocation of the Global parameters
    NMP     nmp;					// Allocation of Named Parms needed by this module    

    OneWire ds( DS_PIN ); 
    DS18    temp( &ds );            // Allocation of DS18B20 class with OneWire

    HTU21   htu;                    // Allocation of DHU21 temp-humidity sensor
    
    SimpleDHT22 dht22( DS_PIN );    // Allocation of Humidity Sensors
