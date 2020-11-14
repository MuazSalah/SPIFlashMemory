# SPIFlashMemory
Arduino code for SPI Flash Memory


The original tutorial and code are by Catoblepas of Instructables.com 

https://www.instructables.com/How-to-Design-with-Discrete-SPI-Flash-Memory/

(Tutorial PDF is added on the repository for reference)

I have modified the code so that I can send a simple string and readback a simple string, hiding all the ascii encoding and hexa world


## Description
* SPIFlash Library + Example Codes (Standard Arduino Library format)

* It provides functions which takes/returns Strings and takes care of bytes conversions internally.

* It uses the EEPORM to store a pointer so that it will always write in the next available slot in the memory


## Functions

* ```myflash.flashErase();```
Erases the flash memory and resets the pointer in the EEPORM to 0,0

* ```myflash.writeToFlash(String);```
Writes a string to the flash memory starting at the next avaialble slot (byte);

* ```myflash.readFromFlash();```
Reads all the content of the flash and return it in the form of a string


Provided as is, no warranty given
