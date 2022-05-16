# Arduino Challenge

## Authors

The software and explanations are written by me, **Teodor Oprea**, in collaboration with **Nathan Wong** and **Nirav Patel**.

## Challenge Details

The challenge consists of programming and building a cart using Arduino hardware which will finish the course as fast as possible. The course contains three sections. In the first one, the cart should follow a wall that is approximately two meters long using an ultrasound sensor, which detects the distance from the wall. After, the cart has to be remotely driven through a maze using an infrared remote and sensor. Lastly, the cart should automatically follow tapes of certain colors until the finish line.

## Software Design

Right off the bat, the libraries are imported and the constants used in the program are declared using the `constexpr` keyword. Some `enum class` are also declared to facilitate reading and code understanding. Servo objects are declared and the current mode is initialized to Mode 1. Functions are written to send movement commands to the servos, calculate the distance using the ultrasonic sensor, and interpret the tape color using the color sensor. 
In the loop, the current mode is tracked in a variable and is checked each time. According to the current mode, different tasks are performed, as will be explained below

## Used Libraries (headers)

- `<stdint.h>`: Classic C library to standardize the size of integers
- `<Servo.h>`: Arduino library to control the servos
- `<IRremote.h>`: Arduino library to interpret infrared signals from a remote

## Modes
### Mode 1: Remote Control

The remote control mode is responsible for the second part of the challenge, but it is the first mode because the cart starts in this mode when powered on. From this mode, one can give commands to the cart using the infrared remote and sensor. It can control the direction of the servos, effectively making them move in the desired location. The cart will understand the commands of the 'Up', 'Down', 'Right', and "Left" buttons from the remote and move accordingly. If no movement command is made, the cart automatically stops after a short time. Also, from this mode, another mode can be chosen by pressing the buttons '2' and '3' which respectively change to Mode 2 and Mode 3. The command from each button of the remote was found experimentally. 

### Mode 2: Wall Follow

The goal of this mode is to make the cart follow the wall at a given distance. The ultrasonic sensor is used to calculate the distance from the wall: it detects the time from the sonic wave to come back and the distance is calculated by multiplying the time by the speed of sound and dividing by two (one-way distance). If the distance is larger than the target distance, the cart will move closer, and vice-versa if the distance is smaller. The ultrasonic sensor is checked often to allow the cart to correct its path as soon as possible. The cart stops when it down not detect a wall for some time. Also, the IR sensor checked each time if the Mode 1 button is pressed, to switch the cart into remote control mode if needed.

### Mode 3: Tape Follow

There are three tapes of different colors. They are superposed and are on the right, middle, and left. If the cart is over the middle tape, it should go forwards, and if not, it should correct its path. The color sensor checks repeatedly the frequencies of the red, green, blue, and clear filters. The red, green, and blue are then divided by the clear frequency, to give a ratio of RGB. These ratios are compared to experimental values of the colors of tapes to decide which color is most likely to be underneath the sensor.

## Sensors

### Infrared Sensor

The infrared sensor had three pins, one for digital signal and two for power and ground. These are the values of the remote commands for a given key, found experimentally for the remote we used:

| key   | command |
|-------|---------|
| up    | `0x18`  |
| right | `0x5A`  |
| left  | `0x08`  |
| down  | `0x52`  |
| 1     | `0x45`  |
| 2     | `0x46`  |
| 3     | `0x47`  |

### Ultrasonic Sensor

The ultrasonic sensor has four pins: the trigger pin to send a sonic wave, the echo pin to measure the duration of the wave and the power and ground pins.

### Color Sensor

The color sensor has eight pins. Two are used for power and ground, another is to power the LEDs. The 's0', 's1', 's2', and 's3' are to select the frequency power and filter type (i.e. red, green, blue, clear). The 'out' pin sends digital information of the frequency received.

