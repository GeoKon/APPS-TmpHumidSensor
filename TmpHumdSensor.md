## Features

- Measures  temperature and humidity.

- Convenient Local GUI interface

- Connects via WiFi to local LAN

- Accessible via Web Interface using

  - HTTP pages
  - REST interface

-  Can be targeted to a variety of ESP8266 or ESP8265 platforms such as 

  - NodeMCU (4MB/1MB FS)
  - Sonoff (1MB/128kB FS) 

- Supports multiple sensor types:

  - one or two DS18B20 using a single wire interface (Vcc GND SDA)
  - the DHT22 temperature and humidity sensor using a propriatory single wire interface (Vcc GND SDA)
  - the HUT21 temperature and humidity sensor using a standard I2C interface

- Local OLED display

  - Displays Temperature(s) and/or Humidity
  - Can be enabled or disabled via serial CLI or via WEB interface
  
- Sensors can be selected are runtime via

  - Serial command line interface (the `sensor` command) 
  - MegunoLink GUI interface (using three pushbuttons)
  - REST interface
  - WEB CLI interface

- Thermostat features

  - In heat mode, a relay is activated if the current temperature is below a specified target temperature 
  - In cold mode, the relay is activated if the current temperature is above the target temperature
  - In manual mode, the relay can be controlled by 
    - the Serial CLI, MegunoLink GUI, WEB or REST interface
    - the local push-button which acts as a toggle switch for the relay

  

## Code Reference

This section is an overview of the code. Describes the various modules and flows

#### setup()

The main setup initializes all global parameters such as WiFi parms, and User parms. A standardized template for global parameters is followed in myGlobals.h. The `myp.initAllParms()` method does the heavy lifting, i.e. checks the integrity of EEPROM, and if good, fetches all working parameters. It also initializes the volatile (aka state) variables. If not good, installs into the EEPROM the default version of parameters.

Next installs the CLI table using the `exe.registerTable()` method which allows execution of commands entered either via the console (Serial interface) or via the Web interface

Then, the OLED is initialized, regardless if it is installed or not.

The `startCLIAfter()`function is used to optionally start the command line interface without activating the WiFi and the main loop. This allows checking most of the sensor functionality, improving the local GUI interface, forcing initialization of the EEPROM, examining and modifying default parameters, etc. 

The WiFi interface is started using the `setupSTA()` function which uses the EEPROM parameters to infer to the SSID and password. It optionally fixes the IP address or defaults to the DHCP discovery. In addition, it activates the mDNS discovery using a label easily discoverable in the local LAN.

Immediately after, the Web server is activated starting with the registration of the end points. The `srvCallbacks()` activate the most often used end points most of which not requiring any `.htm` files in the filesystem. The `cliCallbacks()` function is used to connect the console CLI via REST interface or an HTML page.

Finally, the selected sensors are initialized by invoking the `initSensors()` function.  Just before the sensor initialization, three IIR filters are activated using the `initFilters()` function.  In case of DS18, the number of devices is discovered on the single wire bus using the  `temp.search()` method. In case of the HTU, the initialization method `htu.init()` is used.

#### loop()

The main loop uses the *ready-act* design pattern in combination with the `TICsec` class. This class provides regular tics to start sampling of the various sensors. More explicitly, the following *ready* methods are used: 

1. `buttonPressReady()` to indicate that the push-button was pressed
2. `cli.ready()` to indicate that something has been entered in the console
3. `ticDS18.ready()` to indicate that the DS18 sensor is to be sampled
4. `ticDHT.ready()` to indicate that the DHT sensor is to be polled
5. the same timer `ticDS18.ready()` is also used to indicate that the HUT sensor is to be sampled

Each of the above *ready* functions, lead to an appropriate *action*. More specifically,

For (1), when the push-button is pressed the relay is flipped to manual control and it is toggles. The heat or cool mode are cancelled. The local CLI status display is updated.

For (2), when a CLI command has been entered (as indicated by the RETURN terminator), the appropriate function is executed. Note that each function is expected to buffer the responses and the actual local printing is performed as a part of the *actions*.

For (3), (4) and (5), after the *ready()* is triggered at periodic tics, measurements are made and saved in the Global state structure consisting of members such as:

- `myp.tempF` for the first temperature
- `myp.tempF2` for the second temperature (if 2nd DS18 sensor is present)
- `myp.humidity` for the humidity

Each *action* associated with (3), (4) and (5), reads the sensor(s) and just fills the above Global variables.  All other use of sensor data are handled totally asynchronously either from the local CLI, or Web or REST interfaces by accessing the above Global variables.

The *action* blocks also handle simulations; that is, if simulation is activated instead of using the real sensor readings, the simulated temperature is used instead.

All *actions*, call he `setTempHumidity()` to perform the following functions:

- applies the IIR smoothing filter to all measurements.
- sets the Global variables properly.
- checks if heat or cool mode is enabled and if the temperature is above or below the target threshold, it activates or deactivates the relay.
- updates the local CLI and/or MegunoLink GUI
- updates the OLED

The `setRelayHeatCool()` function which is responsible to activate the relay. This relay is activated if the temperature is below the target temperature (in case of heating) or above the target (in case of cooling). 

Updates of the OLED are performed calling `updateOLED().` 

Updates to the local CLI are performed using the `updateSelectedDisplays()`

The DS18 sensor is sampled in two steps; first the measurement is initiated using the `temp.start()` method. This method starts the conversion process either for one or two sensors in sequential fashion. When the measurement is complete, the `temp.ready()` becomes true; next the `temp.getDegreesC()` is called to obtain the actual measurement.

The DUT nd HUT sensors operate in a simpler fashion: take measurements, then call the `setTempHumidity()` to complete savings of the filtered measurements and updates of OLED and CLI.