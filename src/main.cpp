#include <ThermisteorFunctions.h>

uint32_t printdelay = 250;
void setup() {
  // put your setup code here, to run once:
  canSetup();
  spiSetup();
}

uint32_t lastPrintTime = 0;

void loop() {
  // put your main code here, to run repeatedly:
  readAll();
  writeData();
  if(millis() >= lastPrintTime + printdelay){
    lastPrintTime = millis();
    printTemp();
  }
}
