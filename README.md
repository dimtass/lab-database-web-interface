Lab inventory web interface
----

This repo is my personal web interface that I use to keep an inventory of my
components, boards, SBC, e.t.c. for my home lab. The web interface is just a
simple html/php/javascript code that executes queries in an SQLite3 database.
The web interface also supports to send aREST commands to an ESP8266 that drives
a WS2812b RGB LED strip. You can also choose another LED strip type in the
ESP8266 firmware. More details regarding the firmware are later in this README
and also in my blog port.

*Note:* There is also a blog post here with more details on
how to build the project:
[https://www.stupid-projects.com/component-database-with-led-indicators/](https://www.stupid-projects.com/component-database-with-led-indicators/)


## Build firmware
To build the firmware you need [platform.io](https://platformio.org/).
I'm running an Ubuntu 18.04.3LTS and I'm using Visual Studio Code with
the platform.io extension. In this case you just need to open the
project and then open a terminal by clicking `View -> Terminal` and then
run those two commands:

```sh
pio lib update
pio lib install
```

This will install all the dependency libraries. Now you can build the firmware

## Connect ESP8266
Now you need to connect everything together, which is quite simple.
Use a +5V power supply that can provide the needed power and current
for the LED strip. Now, connect the +5V and GND of the WS2812B strip
to the PSU and then also do the following connections:

ESP8266 | WS2812B
-|-
D2 | Din
GND | GND

Also connect the D1 pin of the NodeMCU to GND via a 4K7 resistor.
This pin is used for reseting the configuration to the default values
and when it's connected to the GND then it's the normal operation and
when is connected to +3V3 then if you reset the ESP8266 then the default
configuration will be loaded. Always remember to connect the resistor
again to the GND after reseting to defaults.

Finally connect a USB cable to ESP8266 and your computer or a USB power
supply in normal use.

## aREST commands
The supported aREST commands are the following ones

Command | Description
-|-
led_index | The index of the LED to turn ON
led_on_color | The integer value of the CRGB color for ON
led_off_color | The integer value of the CRGB color for OFF
led_on_timeout | The integer value of the LED timeout in seconds
led_ambient | The integer value of the CRGB color for the ambient mode
enable_ambient | Enables/disables the ambient mode
wifi_ssid | The WiFi SSIDwifi_passwordThe WiFi password

The color CRGB values can be calculated by converting the
HEX value to integer for each supported color in this file:
`.pio/libdeps/nodemcuv2/FastLED_ID126/pixeltypes.h`

Assuming that the ESP8266 IP address is `192.168.0.42`, then to
send an aREST command, open a web browser and enter this url to
lit the first LED in the strip

```
http://192.168.0.42/led_index?params=0
```

Or if you want to set the led_on_color to CRGB::SkyBlue (which according
to pixeltypes.h is 0x87CEEB in HEX or 8900331 in integer), then
send this command:

```
http://192.168.0.42/led_on_color?params=8900331
```

Similarly, you can execute any spported command.

#### ambient mode
When ambient mode is enabled then all the LEDs in the strip
are lit permanately with the color defined in the the `led_ambient`
aREST variable. Assuming that the ESP8266 IP address is
`192.168.0.42`, then to enable the ambient mode, open a web
browser and enter this url:
```
http://192.168.0.42/enable_ambient?params=1
```

And to disable it:
```
http://192.168.0.42/enable_ambient?params=0
```

## Install SQLite browser
[DB Browser for SQLite](https://sqlitebrowser.org/) is an open source
browser with a GUI that you can use to edit manually your database.
You can find installers in the previous link, but in case you're using
Ubuntu like me, then you can install it with apt

```sh
sudo apt install sqlitebrowser
```

## Testing the interface with Docker
To test the web interface or develop new features you don't have
to install a web server from the scratch. Instead you can use the
included Dockerfile to build an image that includes `lighttpd`,
`php-7.4` and `sqlite3`. To build the image, then from the root
repo directory run this command:

```sh
docker build --build-arg WWW_DATA_UID=$(id -u ${USER}) -t lighttpd-server docker-lighttpd-php7.4-sqlite3/
```

> *Note:* Your host's user uid need to be the same with the `www-data`
user in the image. By default the `www-data` uid is set to `1000`.
If your host user has a different uid, then you need to set the
proper uid in the build command by setting the `WWW_DATA_UID` arg.
The above script will automatically extract your user's uid, but in
case you want a specific one then just override the `WWW_DATA_UID`.


After you build the image then you can use `docker-compose` with the
provided `scripts/docker-compose.yml` file to bring up a web server and start
experimenting. To that you need first to install `docker-compose` by
following this guide [here](https://docs.docker.com/compose/install/).
Then you can run this command:

```sh
docker-compose up
```

The above yml file will mount the `www/lab` folder to the `/var/www/html/lab`
and it will replace the default `/etc/lighttpd/lighttpd.conf` file with
the one in the repo's `www/lighttpd.conf`.

This step is not necessary, but if you need to get acccess in to the
console of the running container then run this command:
```sh
docker-compose exec webserver /bin/sh
```

Finally, in order to test the webserver you need the container's IP
address. You can find that with two different ways. One way is to find
the ip using docker tool.

```sh
docker ps -a
```

This command will output all the running containers. You need to extract
the `CONTAINER ID` of the web-server (e.g.f539671b84fd) and then use this
to inspect the container with this command:

```sh
docker inspect f539671b84fd
```

This will print several information but what you need is the
`NetworkSettings -> Networks -> IPAddress` value.

The other way to find the ip is to connect to the container's console and
run `ifconfig`.

```sh
docker-compose exec webserver /bin/sh
ifconfig
```

## Licence
For all the parts of code that's mine the lience is MIT.
Be aware that other libraries used in the code may have
different licence.