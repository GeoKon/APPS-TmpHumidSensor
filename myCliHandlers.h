/*
###         myCliHanders.cpp Organization

Contains the following blocks of code:

1. Initialization routines used by setup() and this module, such as
   - `initFilters()`
   - `initSensors()`
2. Update routines of local CLI and MegunoLink used by loop(), such as
   - `updateReadings()`
   - `updateStatus()`
   - `updateGraph()`
   - `updateSelectedDisplays()`
3. OLED Initialization and reporting routines, such as
   - `initOLED()`
   - `updateOLED()`
4. Temperature and Humidity CLI Handlers
   - `cliXXX()`  are called by the CLI dispatcher and display results to the console.
   - `mgnXXX()` augment the previous functionality by displaying results to the MegunoLink GUI.

5. CLI command table
   - Lists of all CLI commands. 
   - Can be involved via the console CLI or via he WEB server

###         Exported functions
```
*/
#include "cliClass.h"

extern CMDTABLE mypTable[];                             // table of all CLI commands
extern IIR iir1, iir2, iir3;                            // Smoothing filters (used by the main loop()

void initFilters( int samples );                        // intializes IIR filters
void initSensors( sensor_t  senstype, BUF *bp=NULL );   // initialized sensors according to myp.gp.sensor

void updateSelectedDisplays( int code );                // used by main loop() to update CLI and Meguno displays

void initOLED();                                        // OLED functions
void updateOLED( );
void updateOLED( char *status );
/*
```
*/
