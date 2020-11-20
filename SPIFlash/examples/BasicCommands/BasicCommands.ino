#include <SPIFlash.h>

SPIFlash myflash;


void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  //Minimize oeprations on the flash, wait for possible user disturbances
  Serial.println("Code started, wait for 5 seconds ... ");
  delay(5000);

  
  //This command takes quite some time, especially for large flash memories, give it some time;
  Serial.print("Erasing the chip... ");
  myflash.flashErase();
  Serial.println("Done");

  Serial.print("Writing to the Flash... ");
  myflash.writeToFlash('a');
  Serial.println("Done");

  Serial.print("Reading from the Flash char[0]... ");
  Serial.println(myflash.readFromFlash(0));
  Serial.println("Done");


  Serial.print("Reading data size... ");
  Serial.println(myflash.dataSize());
  Serial.println("Done");

}

void loop() {
  // put your main code here, to run repeatedly:

}
