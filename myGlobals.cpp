/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */

#include "nmpClass.h"
#include "myGlobals.h"
#include <ds18Class.h>      // in GKE-L2

    Global myp;					// Allocation of the Global parameters
    NMP nmp;					// Allocation of Named Parms needed by this module    

#ifdef DS18B20					// Allocation of Temp Sensors
    OneWire ds( DS_PIN ); 
    DS18 temp( &ds );           // associate DS18B20 class with OneWire
#endif

#ifdef DHT22					// Allocation of Humidity Sensors
    SimpleDHT22 dht22( DS_PIN );
#endif
