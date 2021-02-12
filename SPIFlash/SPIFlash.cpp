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

  Redistribution of Peter's code with a more Arduino-user-friendly interface functions.
  GNU GPL V3

  Rev 1 - 10/Nov/2020
  


  Rev 2 - 30/01/2021
  + Improved the reading speed by using maximum SPI frequency
  + Modified the char read function name from "readFromFlash" to "readCharFromFlash"
  + Added a function to read From-To address, much faster when reading multiple pages of memory 
  + Added a helper function "_read_page_chars" which is modded from "_read_page" to ease the work on the function "readFromFlash" so that char conversion is direct using the "char()" function rather than sprintf'ing
  + Updated the example code with the new functions usage 
  
  
  
  Rev 3 - 12/02/2021
  + Bug Fix: The provided example code was printing the buffer in a single shot, leading to printing unknown char at the end
  + Minor improvement to the library by removing unnecessary extra index variable
  
*/



#include "Arduino.h"
#include <EEPROM.h>
#include "SPIFlash.h"


#include <SPI.h>
//For Uno:
// SS:   pin 10
// MOSI: pin 11
// MISO: pin 12
// SCK:  pin 13

// WinBond flash commands
#define WB_WRITE_ENABLE       0x06
#define WB_WRITE_DISABLE      0x04
#define WB_CHIP_ERASE         0xc7
#define WB_READ_STATUS_REG_1  0x05
#define WB_READ_DATA          0x03
#define WB_PAGE_PROGRAM       0x02
#define WB_JEDEC_ID           0x9f


#define PAGE_SIZE 256 //Flash memory page size
#define FLASH_MEMORY_SIZE 128000000 //Bit (128000000 = 128MBit such as Winbond W25Q128JV )
#define FLASH_MEMORY_LAST_PAGE_ADDRESS (FLASH_MEMORY_SIZE/8)/PAGE_SIZE



#define FLASH_ADDRESS_POINTER_BYTE 10 //A single address byte is required, because byte address range is 0-255
#define FLASH_ADDRESS_POINTER_PAGE 11 //Address 11 and 12 will be used as we need an int for the page number (for a 128MBit Flash, there are 4096 pages)







SPIFlash::SPIFlash()
{
	//Yay!! We have got constructed! Let us dress up (SPI Initialization for the rest of the functions)
	SPI.begin();
	SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
}






/*
   print_page_bytes() is a simple helperf function that formats 256
   bytes of data into an easier to read grid.
*/
void SPIFlash::print_page_bytes(byte *page_buffer) {
  char buf[10];
  for (int i = 0; i < 16; ++i) {
    for (int j = 0; j < 16; ++j) {
      sprintf(buf, "%02x", page_buffer[i * 16 + j]);
      //Serial.print(buf);
    }
    //Serial.println();
  }
}

/*
  ================================================================================
  User Interface Routines
  The functions below map to user commands. They wrap the low-level calls with
  print/debug statements for readability.
  ================================================================================
*/

/*
   The JEDEC ID is fairly generic, I use this function to verify the setup
   is working properly.
*/
void SPIFlash::get_jedec_id(void) {
  //Serial.println("command: get_jedec_id");
  byte b1, b2, b3;
  _get_jedec_id(&b1, &b2, &b3);
  char buf[128];
  sprintf(buf, "Manufacturer ID: %02xh\nMemory Type: %02xh\nCapacity: %02xh",
          b1, b2, b3);
  //Serial.println(buf);
  //Serial.println("Ready");
}

void SPIFlash::chip_erase(void) {
  //Serial.println("command: chip_erase (please wait for 'Ready' signal, this process might take some time)");
  _chip_erase();
  //Serial.println("Ready");
}

void SPIFlash::read_page(unsigned int page_number) {
  char buf[80];
  sprintf(buf, "command: read_page(%04xh)", page_number);
  //Serial.println(buf);
  byte page_buffer[256];
  _read_page(page_number, page_buffer);
  print_page_bytes(page_buffer);
  //Serial.println("Ready");
}

void SPIFlash::read_all_pages(void) {
  //Serial.println("command: read_all_pages");
  byte page_buffer[256];
  for (int i = 0; i < 4096; ++i) {
    _read_page(i, page_buffer);
    print_page_bytes(page_buffer);
  }
  //Serial.println("Ready");
}

void SPIFlash::write_byte(word page, byte offset, byte databyte) {
  char buf[80];
  sprintf(buf, "command: write_byte(%04xh, %04xh, %02xh)", page, offset, databyte);
  //Serial.println(buf);
  byte page_data[256];
  _read_page(page, page_data);
  page_data[offset] = databyte;
  _write_page(page, page_data);
  //Serial.println("Ready");
}

/*
  ================================================================================
  Low-Level Device Routines
  The functions below perform the lowest-level interactions with the flash device.
  They match the timing diagrams of the datahsheet. They are called by wrapper
  functions which provide a little more feedback to the user. I made them stand-
  alone functions so they can be re-used. Each function corresponds to a flash
  instruction opcode.
  ================================================================================
*/

/*
   See the timing diagram in section 9.2.35 of the
   data sheet, "Read JEDEC ID (9Fh)".
*/
void SPIFlash::_get_jedec_id(byte *b1, byte *b2, byte *b3) {
  digitalWrite(SS, HIGH);
  digitalWrite(SS, LOW);
  SPI.transfer(WB_JEDEC_ID);
  *b1 = SPI.transfer(0); // manufacturer id
  *b2 = SPI.transfer(0); // memory type
  *b3 = SPI.transfer(0); // capacity
  digitalWrite(SS, HIGH);
  not_busy();
}

/*
   See the timing diagram in section 9.2.26 of the
   data sheet, "Chip Erase (C7h / 06h)". (Note:
   either opcode works.)
*/
void SPIFlash::_chip_erase(void) {
  digitalWrite(SS, HIGH);
  digitalWrite(SS, LOW);
  SPI.transfer(WB_WRITE_ENABLE);
  digitalWrite(SS, HIGH);
  digitalWrite(SS, LOW);
  SPI.transfer(WB_CHIP_ERASE);
  digitalWrite(SS, HIGH);
  /* See notes on rev 2
    digitalWrite(SS, LOW);
    SPI.transfer(WB_WRITE_DISABLE);
    digitalWrite(SS, HIGH);
  */
  not_busy();
}

/*
   See the timing diagram in section 9.2.10 of the
   data sheet, "Read Data (03h)".
*/
void SPIFlash::_read_page(word page_number, byte *page_buffer) {
  digitalWrite(SS, HIGH);
  digitalWrite(SS, LOW);
  SPI.transfer(WB_READ_DATA);
  // Construct the 24-bit address from the 16-bit page
  // number and 0x00, since we will read 256 bytes (one
  // page).
  SPI.transfer((page_number >> 8) & 0xFF);
  SPI.transfer((page_number >> 0) & 0xFF);
  SPI.transfer(0);
  for (int i = 0; i < 256; ++i) {
    page_buffer[i] = SPI.transfer(0);
  }
  digitalWrite(SS, HIGH);
  not_busy();
}

/*
   See the timing diagram in section 9.2.10 of the
   data sheet, "Read Data (03h)".
*/
void SPIFlash::_read_page_chars(word page_number, char *page_buffer) {
  digitalWrite(SS, HIGH);
  digitalWrite(SS, LOW);
  SPI.transfer(WB_READ_DATA);
  // Construct the 24-bit address from the 16-bit page
  // number and 0x00, since we will read 256 bytes (one
  // page).
  SPI.transfer((page_number >> 8) & 0xFF);
  SPI.transfer((page_number >> 0) & 0xFF);
  SPI.transfer(0);
  for (int i = 0; i < 256; ++i) {
    page_buffer[i] = char(SPI.transfer(0));
  }
  digitalWrite(SS, HIGH);
  not_busy();
}

/*
   See the timing diagram in section 9.2.21 of the
   data sheet, "Page Program (02h)".
*/
void SPIFlash::_write_page(word page_number, byte *page_buffer) {
  digitalWrite(SS, HIGH);
  digitalWrite(SS, LOW);
  SPI.transfer(WB_WRITE_ENABLE);
  digitalWrite(SS, HIGH);
  digitalWrite(SS, LOW);
  SPI.transfer(WB_PAGE_PROGRAM);
  SPI.transfer((page_number >>  8) & 0xFF);
  SPI.transfer((page_number >>  0) & 0xFF);
  SPI.transfer(0);
  for (int i = 0; i < 256; ++i) {
    SPI.transfer(page_buffer[i]);
  }
  digitalWrite(SS, HIGH);
  /* See notes on rev 2
    digitalWrite(SS, LOW);
    SPI.transfer(WB_WRITE_DISABLE);
    digitalWrite(SS, HIGH);
  */
  not_busy();
}

/*
   not_busy() polls the status bit of the device until it
   completes the current operation. Most operations finish
   in a few hundred microseconds or less, but chip erase
   may take 500+ms. Finish all operations with this poll.

   See section 9.2.8 of the datasheet
*/
void SPIFlash::not_busy(void) {
  digitalWrite(SS, HIGH);
  digitalWrite(SS, LOW);
  SPI.transfer(WB_READ_STATUS_REG_1);
  while (SPI.transfer(0) & 1) {};
  digitalWrite(SS, HIGH);
}










//Writing the int address pointers to the EEPORM (page pointer is 0 - 4096 for a 128Mb flash)
void SPIFlash::writeIntIntoEEPROM(int address, int number)
{
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

//Reading the int address pointers from the EEPORM (page pointer is 0 - 4096 for a 128Mb flash)
int SPIFlash::readIntFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}


//Write a string to the flash, the code is utilising the existing code but instead of accepting bytes, it will take a string and work on it one char at a time
//Note: Don't try to change these to String, Strings do get messy when they are elongated
bool SPIFlash::writeToFlash(char chr) {

  //Flash memory pointers, those pointers are stored on the EEPROM such that we always write on an unallocated space byte
  int flash_page_pointer;
  int flash_byte_pointer;
  
  

  flash_byte_pointer = EEPROM.read(FLASH_ADDRESS_POINTER_BYTE);
  flash_page_pointer = readIntFromEEPROM(FLASH_ADDRESS_POINTER_PAGE);

  if (
    (flash_page_pointer <= FLASH_MEMORY_LAST_PAGE_ADDRESS)
    &&
    (flash_byte_pointer < PAGE_SIZE)
  )
  {

    //Flash Memory variables
    word pageno;
    byte offset;
    byte data;

    pageno = (word)flash_page_pointer;
    offset = (byte)flash_byte_pointer;
    data = (byte)((int)chr);

    write_byte(pageno, offset, data);



    //update the pointers
    if (flash_byte_pointer == (PAGE_SIZE-1)) {
      flash_page_pointer ++;
      flash_byte_pointer = 0;
      writeIntIntoEEPROM(FLASH_ADDRESS_POINTER_PAGE, flash_page_pointer);
    } else {
      flash_byte_pointer ++;
    }
    EEPROM.update(FLASH_ADDRESS_POINTER_BYTE, flash_byte_pointer);
    

    return true;

  }
  else {
    return false;
  }


}








//Read a char back from the flash
char SPIFlash::readCharFromFlash(int pageAddr, int byteAddr) {

  char page_buffer[PAGE_SIZE];

  _read_page_chars(pageAddr, page_buffer);

  return page_buffer[byteAddr];

}


char SPIFlash::readCharFromFlash(unsigned long byteAbsAddr) {
	
  char page_buffer[PAGE_SIZE];

  int pageAddr = byteAbsAddr / PAGE_SIZE;
  int byteAddr = (byteAbsAddr % PAGE_SIZE);

  _read_page_chars(pageAddr, page_buffer);

  return page_buffer[byteAddr];

}














//Read a char back from the flash
void SPIFlash::readFromFlash(unsigned long fromByteAddr, unsigned long toByteAddr, char *char_buffer) {

	char page_buffer[PAGE_SIZE];


	if (toByteAddr >= fromByteAddr) { //Validate the request
		int fromPageAddr = fromByteAddr / PAGE_SIZE;
		int fromByteAddr_relative = (fromByteAddr % PAGE_SIZE);
		int toPageAddr = toByteAddr / PAGE_SIZE;
		int toByteAddr_relative = (toByteAddr % PAGE_SIZE);
		
		if (fromPageAddr == toPageAddr) { //If the reading range is within the same page
			_read_page_chars(fromPageAddr, page_buffer);
			for (int i=fromByteAddr_relative; i<=toByteAddr_relative; i++) {
				char_buffer[i] = page_buffer[i];
			}
		} else {
			Serial.println("different page read");
			unsigned long j=0;
			for (int p=fromPageAddr; p<=toPageAddr; p++) {
				_read_page_chars(p, page_buffer);
				if (p==fromPageAddr) { //We are on the first page
					for (int i=fromByteAddr_relative; i<PAGE_SIZE; i++) {
						char_buffer[j] = page_buffer[i];
						j++;
					}
				} else if (p==toPageAddr) { //We are on the last page
					for (int i=0; i<=toByteAddr_relative; i++) {
						char_buffer[j] = page_buffer[i];
						j++;
					}
				} else { //We are in a middle page
					for (int i=0; i<PAGE_SIZE; i++) {
						char_buffer[j] = page_buffer[i];
						j++;
					}
				}
			}
		}
	}
}
















unsigned long SPIFlash::dataSize() {
  return EEPROM.read(FLASH_ADDRESS_POINTER_BYTE) + (readIntFromEEPROM(FLASH_ADDRESS_POINTER_PAGE) * 1L * PAGE_SIZE); //the * 1L is required for unsigned long calculation
}





void SPIFlash::flashErase(){
  //Erase the chip
  chip_erase();

  //Return the address pointers to 0s
  EEPROM.update(FLASH_ADDRESS_POINTER_BYTE, 0);
  writeIntIntoEEPROM(FLASH_ADDRESS_POINTER_PAGE, 0);
}
