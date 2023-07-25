# ESP32-Telegram-Send-Document

This actually a code for a system to measure 4x load cell using HX711 and then the notification is sent to the telegram. 
The program running on two core, since the wifi connection and also the process of sending the file and notification to the telegram sometimes prevent the loadcell to measure the weight. Therefore I separate the tasks.

First of all you need to replace the these two file in the universal arduino telegram bot master library
usually file in C:\Users\<your usernam>\Documents\Arduino\libraries\Universal-Arduino-Telegram-Bot-master\src

1.  UniversalTelegramBot.cpp
2.  UniversalTelegramBot.h

after that you can just try and adapt the arduino code as you want.
goodluck
:)
