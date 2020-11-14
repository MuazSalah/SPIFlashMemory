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



#include "Arduino.h"
#include <EEPROM.h>
#include "SPIFlash.h"


#include <SPI.h>
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


#define FLASH_ADDRESS_POINTER_BYTE 10 //A single address byte is required, because byte address range is 0-255
#define FLASH_ADDRESS_POINTER_PAGE 11 //Address 11 and 12 will be used as we need an int for the page number (for a 128MBit Flash, there are 4096 pages)





SPIFlash::SPIFlash()
{
	//Yay!! We have got constructed! Let us dress up (SPI Initialization for the rest of the functions)
	SPI.begin();
	SPI.setDataMode(0);
	SPI.setBitOrder(MSBFIRST);
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
void SPIFlash::writeToFlash(String data_string) {

  //A char array for converting the string to a set of bytes
  char char_array[data_string.length() + 1];

  //Stor the string in the char array
  data_string.toCharArray(char_array, data_string.length() + 1);

  //Flash Memory variables
  word pageno;
  byte offset;
  byte data;

  //Flash memory pointers, those pointers are stored on the EEPROM such that we always write on an unallocated space byte
  int flash_page_pointer;
  int flash_byte_pointer;

  flash_byte_pointer = EEPROM.read(FLASH_ADDRESS_POINTER_BYTE);
  flash_page_pointer = readIntFromEEPROM(FLASH_ADDRESS_POINTER_PAGE);



  for (int i = 0; i < data_string.length(); i++)
  {

    pageno = (word)flash_page_pointer;
    offset = (byte)flash_byte_pointer;
    data = (byte)((int)char_array[i]);

    write_byte(pageno, offset, data);



    //update the pointers
    if (flash_byte_pointer == 255) {
      flash_page_pointer ++;
      flash_byte_pointer = 0;
    } else {
      flash_byte_pointer ++;
    }

  }




  EEPROM.update(FLASH_ADDRESS_POINTER_BYTE, flash_byte_pointer);
  writeIntIntoEEPROM(FLASH_ADDRESS_POINTER_PAGE, flash_page_pointer);


}



//Read the string back from the flash
String SPIFlash::readFromFlash() {

  String tmp_str;

  int total_stored;
  int flash_page_pointer;
  int flash_byte_pointer;

  flash_byte_pointer = EEPROM.read(FLASH_ADDRESS_POINTER_BYTE);
  flash_page_pointer = readIntFromEEPROM(FLASH_ADDRESS_POINTER_PAGE);


  total_stored = flash_byte_pointer + (flash_page_pointer * 255);

  /*Serial.print("Size of data stored: ");
  Serial.println(total_stored);

  Serial.println("Stored data: ");*/

  byte data_buffer;
  char buf[10];
  int temp;
  char str_buffer[10];
  byte page_buffer[256];

  for (int i = 0; i <= flash_page_pointer; i++) {
    for (int j = 0; j < flash_byte_pointer; j++) {
      _read_page(i, page_buffer);
      sprintf(buf, "%03d", page_buffer[i * 16 + j]);
      temp = atoi(buf);
      tmp_str = tmp_str + (char)temp;
      
    }
  }

  /*Serial.println(tmp_str);

  Serial.println("All data read");*/

  return tmp_str;

}





void SPIFlash::flashErase(){
  //Erase the chip
  chip_erase();

  //Return the address pointers to 0s
  EEPROM.update(FLASH_ADDRESS_POINTER_BYTE, 0);
  writeIntIntoEEPROM(FLASH_ADDRESS_POINTER_PAGE, 0);
}
