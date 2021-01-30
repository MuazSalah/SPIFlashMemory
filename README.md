# SPIFlashMemory
Arduino code for SPI Flash Memory


The original tutorial and code are by Catoblepas of Instructables.com 

https://www.instructables.com/How-to-Design-with-Discrete-SPI-Flash-Memory/

(Tutorial PDF is added on the repository for reference)

I have modified the code so that I can send a simple character and readback a simple character, hiding all the ascii encoding and hexa world


## Description
SPIFlash Library + Example Codes (Standard Arduino Library format)

It provides functions which takes/returns char and takes care of bytes conversions internally.

It uses the EEPORM to store a pointer so that it will always write in the next available slot in the memory


## Functions

* ```void myflash.flashErase();```
Erases the flash memory and resets the pointer in the EEPORM to 0,0
Note: for a 128MBit flash, this process takes ~ 30 seconds

* ```bool myflash.writeToFlash(char);```
Writes a char to the flash memory starting at the next avaialble slot (byte) and returns true if write was successful

* ```char readCharFromFlash(int pageAddr, int byteAddr);```
Reads a char from a specifc page address and byte address on the flash and returns it

* ```char readCharFromFlash(int byteAbsAddr);```
Reads a char from a specifc absolute address on the flash and returns it

* ```void readFromFlash(unsigned long fromByteAddr, unsigned long toByteAddr, char *char_buffer);```
Reads a char array from an absolute byte address on the flash and stores in a user defined char* (preferablly with a size of 255 as in the example code)

* ```int dataSizeInFlash();```
Returns the size of data written on the flash memory. This basically uses the stored EEPROM address pointers.


## Compatibility
The library have been tested with Winbond W25Q128JV (using Sparkfun's Serial Flash Breakout - Assembled 128Mbit [SPX-17115]), however, it should be able to work with the other varities of SPI flash memories.


Provided as is, no warranty given
