#include <SPIFlash.h>

SPIFlash myflash;


void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  //Minimize oeprations on the flash, wait for possible user disturbances
  Serial.println("Code started, wait for 5 seconds ... ");
  delay(5000);

  
  //This command takes quite some time, especially for large flash memories, give it some time;
  //It is disabled by default to avoid erasing your data, uncomment it to test
  //Serial.print("Erasing the chip... ");
  //myflash.flashErase();
  //Serial.println("Done");

  //Serial.print("Writing to the Flash... ");
  //myflash.writeToFlash('a');
  //Serial.println("Done");

  //Serial.print("Reading from the Flash char[0]... ");
  //Serial.println(myflash.readCharFromFlash(0));
  //Serial.println("Done");


  Serial.print("Reading data size... ");
  Serial.println(myflash.dataSize());
  Serial.println("Done");
  
  
  
  Serial.print("Reading a range of char from multiple pages ... ");
  unsigned long lastCharAddr = myflash.dataSize() - 1;

  Serial.print("Last char address: ");
  Serial.println(lastCharAddr);
  
  Serial.println("Flash data: ");
  
  unsigned long toAddr = 0;
  int buffSize = 256;
  char read_buffer[buffSize];

  for (unsigned long i = 0; i <= lastCharAddr;) {
    
    if (i + buffSize < lastCharAddr) {
      toAddr = i + buffSize-1;
    } else {
      toAddr = lastCharAddr;
    }

    
    memset(read_buffer, 0, buffSize);
    myflash.readFromFlash(i, toAddr, read_buffer);
    for (int j=0; j<buffSize; j++) {
      Serial.print(read_buffer[j]);
    }
    
    i = i + buffSize;
  }
  Serial.println("");
  Serial.println("Done");
  
  
  
  
  

}

void loop() {
  // put your main code here, to run repeatedly:

}