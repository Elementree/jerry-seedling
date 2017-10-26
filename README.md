
# Jerry Seedling

Arduino based sprouting automator.

#### Outputs:

  - Scheduled light control
  - Scheduled water/nutrient feeding
  - Floor Heater
  - Fan control


#### Inputs:
  - Ambient temperature
  - Relative humidity
  - RTC

#### Component Datasheets:

[1602A-1 LCD Module](https://www.openhacks.com/uploadsproductos/eone-1602a1.pdf)
[Tiny RTC](https://www.elecrow.com/wiki/index.php?title=Tiny_RTC)


### Libraries & Notes

[Dallas Temperature](https://github.com/milesburton/Arduino-Temperature-Control-Library)

[Multiple Buttons on Analog Input](http://forum.arduino.cc/index.php?topic=8558.0)

```
Arduino Connections:

D0:
D1:
D2: [LCD] D7
D3: [LCD] D6
D4: [LCD] D5
D5: [LCD] D4
D6:
D7:
D8:
D9:
D10:
D11: [LCD] EN - Enable
D12: [LCD] RS
D13: 

A0:
A1:
A2:
A3:
A4: [RTC] SDA
A5: [RTC] SCL

LCD Connections:

1  Vss -- 0V
2  Vdd -- +5V Power Supply
3  V0 -- for LCD
4  RS H/L Register Select: H:Data Input L:Instruction Input
5  R/W H/L H--Read L--Write
6  E H,H-L Enable Signal
7  DB0 H/L
8  DB1 H/L
9  DB2 H/L Data bus used in 8 bit transfer
10 DB3 H/L
11 DB4 H/L
12 DB5 H/L Data bus for both 4 and 8 bit transfer
13 DB6 H/L
14 DB7 H/L
15 BLA -- BLACKLIGHT +5V
16 BLK -- BLACKLIGHT 0V- 

```

#### Calculations

```
R LED

R = V/I
  = 5 / 0.018
  = 277 ohm
  ~ 270 ohm

```
