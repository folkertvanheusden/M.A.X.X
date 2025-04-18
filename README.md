M.A.X.X is a wifi manager for the ESP32 and ESP8266. It allows more than one access point to be configured.

See test.ino for a usage example.

Don't forget to run "pio run -t uploadfs" when you use platform.io! (or else include the 'data' directory in your project)

Also make sure you have this in the platformio.ini file:

    build_flags = -std=gnu++17
    build_unflags = -std=gnu++11


Screenshot:

(the 'www.vanheusden.com' is the name of my access-point)

![(screenshot)](images/M.A.X.X-screenshot001.png)

The external dependencies are "ArduinoJson" and "ESPAsyncWebServer" (see platformio.ini).


--- Folkert van Heusden <folkert@vanheusden.com>




The program is named after Max. Max is a dog who has no knowledge about WiFi but wholeheartedly supports the development of this library!

This is a picture of Max:

![(picture of Max)](images/max.jpg)
