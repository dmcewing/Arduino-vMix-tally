# Arduino vMix tally

This project contains the firmware for a tally system based on an Arduino esp8266 and the vMix TCP API.  It is a fork of [Thomas Mout's Arduino-vMix-tally](https://github.com/ThomasMout/Arduino-vMix-tally) project but adjusted to use singular LEDs instead of a LED Matrix, as I had LEDs available and they are cheaper.

## Installation

### Hardware

This modified project requires a little more hardware than the original.  The core part is a ESP8266 arduino board and the remainder are a bunch of LEDs for the indicators.  

The full list of parts I have used are:
* [DuinoTech ESP8266 Main Board](https://www.jaycar.co.nz/wifi-mini-esp8266-main-board/p/XC3802)
* 5mm LEDs (two red, one yellow, one green), I had these in my spares box from a  [100 Assorted LED pack](https://www.jaycar.co.nz/assorted-led-pack-pack-of-100/p/ZD1694)
* Pull down resistors 50 Ohm and 1k Ohm, or to suit LED
* [One Box](https://www.jaycar.co.nz/bulkhead-black-65-x-38-x-27mm/p/HB6065)
* [Short USB extension](https://www.jaycar.co.nz/micro-usb-extension-cable-100mm-pair/p/WC7756)


### Assembly

Each LED is connected to a port on the ESP8266 board, and to the Ground rail.  To simplify the circuit diagram I have just listed the ports used on the ESP8266 board.

Pin | Port   | Purpose      |  LED connected
----|--------|--------------|----------------
D2  | GPIO5  | Live (Front) | Red LED
D5  | GPIO12 | Live (Rear)  | Red LED
D6  | GPIO14 | Preview      | Amber/Yellow LED
D7  | GPIO13 | Standby      | Green LED
G   |        | Ground       |

I just wired the LEDs directly from their mounting to the board, through the headers that were supplied with the main board.

To connect the TALLY easily I used the short USB extension cable to surface the USB Micro B connector outside of the box.

## Software

### 1. Install Arduino IDE

Download the Arduino IDE from the [Arduino website](https://www.arduino.cc/en/main/software) and install it.  
After the installation is complete go to File > Preferences and add http://arduino.esp8266.com/stable/package_esp8266com_index.json to the additional Board Manager URL. Go to Tools > Board > Board Manager, search for *esp8266* and install the latest version. After the installation go to Tools > Board and select *Wemos D1 R2* as your default board.  

### 2. Uploading static files

Connect the Arduino to the computer with a USB cable.  
The static files in the Arduino-vMix-Tally/data folder must be uploaded using the Tools > ESP8266 Sketch Data Upload in the Arduino IDE.  

### 3. Uploading firmware

Upload the Arduino-vMix-Tally/Arduino-vMix-Tally.ino from this repository to the Arduino by pressing the Upload button. After the upload the tally will restart in Connecting mode (see the Muliple states section).  

## Getting Started

### States
Unlike the original that shows a letter on the matrix, this project uses independant LEDs and the combination of illumination indicates the state

State   | Live (Front) | Live (Back) | Preview | Standby | Notes
--------|--------------|-------------|---------|---------|----------
Live    |     On       |     On      |  Off    |   Off   |
Preview |     Off      |     Off     |  On     |   Off   |
Standby |     Off      |     Off     |  Off    |   On    |
Connnecting |  Off     |     Off     |  On     |   On    | The Tally is looking for the vMix instance.
Setup   |    Off       |     On      |  Off    |   On    | The Tally cannot find the WiFi and has configured itself as a default access point for configuration. See below.

### Settings

Network and tally settings can be edited on the built-in webpage. To access the webpage connect to the same WiFi network and navigate to the IP address or the devicename(*vmix_tally_#.home*, # is the tally number) in a browser.  
On this webpage the WiFi SSID, WiFi password, vMix hostname and tally number can be changed. It also shows some basic information of the device.  

### Setup Mode
In this state the tally was unable to connect to WiFi and it turned itself to access point mode. It can be accessed by connecting to the WiFi network with the SSID vMix_Tally_# (# is the tally number) and password vMix_Tally_#_access (# is the tally number). Once connected the settings can be changed by going to the webpage on IP address 192.168.4.1.

## Things to keep in mind

1. Make sure to use a power cable that does not support data when using a USB port on a camera. This can cause connecting issues in the camera.  
2. The Arduino is completely dependent on a good WiFi signal.  
3. The tally must be connected to the same network as the vMix instance.  

## Footnote

Please note all images are for illustration purpose. Actual results may vary.  
Feel free open issues on this repository for bugs or feature requests.  
If you want to create your own version of this repository then a reference is appreciated.  
Special thanks to André for the 3D casing and the solder instructions.  
