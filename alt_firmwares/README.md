# Plaits

### About
This firmware adds i2c support to Plaits. Tell it which features you want to control over i2c and then send commands to control those features. When features are set to i2c control, the module won't response to CV via the jack sockets.

**WARNING**\
You shouldn't have any problem reverting to the standard firmware if you don't get on with this one, but changes to the hardware and firmware are at your own risk.

### Hardware
To communicate with Plaits over i2c we need to use the test points on the back of the PCB. You will probably want to solder header pins to these so you can easy connect cables to them. You will need 1x4 pin and 1x2 pin 2.54mm pitch pin headers and if you're careful these can be soldered without removing the front panel.

The bottom of the 4 pins is ground, the 2 pins are SDA at the top and SCL at the bottom.

![Plaits pin locations](plaits_i2c.jpg)

### Commands
Plaits is given the i2c address 0x49

An i2c command should consist of the address for Plaits, the command number and any parameters required by the command
e.g. 0x49 0x1 0x2 0x1 (set control of trigger on) or 0x49 0x2 (send a trigger).

**Control**\
0x1 &lt;Feature&gt; &lt;0x1 (on) / 0x0 (off)&gt;\
Takes control of a feature over i2c - acts as if a cable is connected to Plaits and ignores any CV sent to it.

Features:\
0x1 - All listed\
0x2 - Trigger\
0x3 - Level\
0x4 - V/Oct

**Trigger**\
0x2\
Sends a trigger

**Level**\
0x3 &lt;MSB&gt; &lt;LSB&gt;\
Sets the level 0 - 16384

**V/Oct**\
0x4 &lt;MSB&gt; &lt;LSB&gt;\
Sets the pitch 0 - 16384 / 0v - 10v

### Issues
- You may need to add a short delay (~10ms?) between Control commands
- Some of the most recent (orange) engines don't seem to work yet

### History
*2024-04-12 - Initial Version*