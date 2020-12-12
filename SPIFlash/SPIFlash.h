/*
    windbond_serial_debug.cpp
    A simple program for the Arduino IDE to help familiarize you with
    using WinBond flash memory; can also be used to download the entire
    contents of a flash chip to a file via a serial port interface.

    Important bits of the code: the low-level flash functions (which
    implement the timing diagrams in the datasheet), and a simple
    serial-port command interface that allows you to talk to your
    UNO with any generic terminal application (even the command line).

    Copyright 2014, Peter J. Torelli

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>


  Revisions:
    rev 2 - 21-SEP-2014.
    User 'fiaskow' pointed out that driving the WEL instruction after
    program and erase w/o waiting for the op to finish may be corrupting
    execution. Removed this code (also not needed b/c the WEL is already
    cleared after page write or chip erase).



  Copyright 2020, Moath Salah

  Redistribution of of Peter's code with a more Arduino-user-friendly interface functions.
  GNU GPL V3

  Rev 1 - 10/Nov/2020
  

    
*/



//HW Allocataion for this library

// SS:   pin 10
// MOSI: pin 11
// MISO: pin 12
// SCK:  pin 13


//SW Allocation for this library
//
// EEPROM Addresses 10, 11, 12
//
//#define FLASH_ADDRESS_POINTER_BYTE 10 //A single address byte is required, because byte address range is 0-255
//#define FLASH_ADDRESS_POINTER_PAGE 11 //Address 11 and 12 will be used as we need an int for the page number (for a 128MBit Flash, there are 4096 pages)

//The weirdo constructo
#ifndef SPI_Flash
#define SPI_Flash



#include "Arduino.h"
#include <EEPROM.h>
#include <SPI.h>


class SPIFlash
{
	public:
		SPIFlash();
		bool writeToFlash(char chr);
		char readFromFlash(int pageAddr, int byteAddr);
		char readFromFlash(unsigned long byteAbsAddr);
		unsigned long dataSize();
		void flashErase();
	private:
		void print_page_bytes(byte *page_buffer);
		void get_jedec_id();
		void chip_erase();
		void read_page(unsigned int page_number);
		void read_all_pages();
		void write_byte(word page, byte offset, byte databyte);
		void _get_jedec_id(byte *b1, byte *b2, byte *b3);
		void _chip_erase();
		void _read_page(word page_number, byte *page_buffer);
		void _write_page(word page_number, byte *page_buffer);
		void serialEvent();
		void writeIntIntoEEPROM(int address, int number);
		void not_busy(void);
		int readIntFromEEPROM(int address);
		
		
};









#endif