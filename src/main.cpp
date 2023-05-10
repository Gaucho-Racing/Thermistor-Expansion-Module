#include <Thermisteor Functions.h>

unsigned long printdelay = 0;
void setup() {
  // put your setup code here, to run once:

  canSetup();
  spiSetup();
}

void loop() {
  // put your main code here, to run repeatedly:
  readAll();
  writeData();
  if(millis() - printdelay > 250){
    printdelay = millis();
    printTemp();
  }
}