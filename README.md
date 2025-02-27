# ESP32_Tibber
An Arduino example for the ESP32 communicating with the Tibber API.

**Configure the program with the Config.h file first!**

Due to high energy tarifs an ESP32 is combined with a servo to control an older electric heating unit from AEG. 

1. The ESP gets todays tarifs and if available tomorrows tarifs from Tibber.
2. Calculate the mean prices.
3. If the current price is lower than the mean
    * true: turn on the heater 
    * false: turn off the heater

Thats all. There a a lot of improvements to be made, but is good enough for me. For example:
* Use light or deepsleep
* Turn off if no tarifs are availible
* Implement in ESP-Home
* Create a library

Feel free to use and experiment. 



