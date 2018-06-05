# GridEYE-8x8-IR-sensor-human-tracking

/*********************************************

Code library from AMG88xx GridEYE 8x8 IR is included.
https://github.com/adafruit/Adafruit_AMG88xx

This code sketch tries to read temperatures at 8x8 pixels from low-cost GridEYE IR sensor,
detect "hot" areas, and determine whether there is a human at some distance (e.g. 1m to 3m)
and track huamn movement. Similary, when a hand waves in front of the IR sensor
(e.g. 0.1m to 0.5m), it detects hand motion.

GridEYE sensors use I2C protocol to communicate with Arduino.

Any Ardunio can be used after this sensor is wired to I2C of specific Arduino model
(e.g. for Arduino Teensy 3.2, SCL is pin 19, SDA is pin 18.
https://www.pjrc.com/teensy/td_libs_Wire.html)

Written by Wenlong Meng, June 1, 2018

Feel free to reuse these codes in your projects.
Comments and advice are welcomed!
**********************************************/
