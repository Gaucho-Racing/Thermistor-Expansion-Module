#include <Arduino.h>
#include <SPI.h>
#include <FlexCAN_T4.h>

//READ STUFF----------------------------------------------------------------------
int chipSelector(int);
float readVoltage(byte);
int getTemp(float);

void spiSetup(){
    for(int i=0; i<20; i++){
        pinMode(chipSelector(i), OUTPUT);
        digitalWrite(chipSelector(i), HIGH);
    }
    SPI.begin();
}

int temps[20][8] = {0};
void readAll(){
    SPI.beginTransaction(SPISettings(20000, MSBFIRST, SPI_MODE3));
    for(int chip = 0; chip<20; chip++){
        for(int mux = 0; mux < 8; mux++){
            temps[chip][mux] = getTemp(readVoltage(chipSelector(chip), mux));
        }
    }
    SPI.endTransaction();
}

int chipSelector(int chip){  //Sets the chip select to choose a given chip
    int pins[20]= {3,4,5,6,7,8,9,10,24,25,26,33,34,35,36,37,38,39,40,41};
    return pins[chip];
}

float readVoltage(int CSpin, uint16_t mux){
  digitalWrite(CSpin, LOW);
  SPI.transfer(0b00000001);
  uint16_t send = 0b1000000000000000 + (mux << 12);
  uint16_t data = SPI.transfer16(send);
  digitalWrite(CSpin, HIGH);
  data = data & 0b0000011111111111;
  return data * 3.3 / 1023.0;
}

int getTemp(float voltage){  //Uses lookup table to get temp
    float lut[33] = {2.44,2.42,2.40,2.38,2.35,2.32,2.27,2.23,2.17,2.11,2.05,1.99,1.92,1.86,1.80,1.74,1.68,1.63,1.59,1.55,1.51,1.48,1.45,1.43,1.40,1.38,1.37,1.35,1.34,1.33,1.32,1.31,1.30};
	
    if(voltage > lut[0]){
        return -40 - ((voltage-lut[0]) / (lut[0]-lut[1]) * 5);
    }

    if(voltage < lut[32]){
        return 120 + ((lut[32]-voltage) / (lut[31]-lut[32]) * 5);
    }

    for(int i=0; i<32; i++){
        if(voltage >= lut[i]){
            float ratio = (voltage-lut[i])/(lut[i+1]-lut[i]);
            int add = ratio*5;
            return i*5-40+add;
        }
    }
}


void printTemp(){
    for(int chip = 0; chip<20; chip++){
        for(int i=0; i<8; i++){
            Serial.print(temps[chip][i]);
            Serial.print('\t');
        }
    }
    Serial.println();
}



//WRITE STUFF----------------------------------------------------------------------
byte lowTemp(byte);
byte highTemp(byte);
byte avgTemp(byte);
void writeBMSBroadcast(byte);
void writeGeneralBroadcast(byte);
void writeAddyClaim(byte);

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1;
CAN_message_t msg;

unsigned long time = millis();
int messageNum = 0;

void canSetup(){
    can1.begin();
    can1.setBaudRate(500000);
    msg.flags.extended = 1;
}

void writeData(){
    if(millis()-time < 20)
        return;
    time = millis();
    switch (messageNum)
    {
    case 0:
        writeAddyClaim(1);//Addy of 1
        break;
    case 5:
        writeAddyClaim(2);//Addy of 2
        break;
    case 1:
    case 6:
        writeBMSBroadcast(1);//BMS of 1
        break;
    case 2:
    case 7:
        writeBMSBroadcast(2);//BMS of 2
        break;
    case 3:
    case 8:
        writeGeneralBroadcast(1);//Gen of 1
        break;
    case 4:
    case 9:
        writeGeneralBroadcast(2);//Gen of 2
        break;
    }
    messageNum++;
    if(messageNum>9)
        messageNum=0;
}

void writeBMSBroadcast(byte module){
    if(module == 1)
        msg.id = 0x1839F380;
    else
        msg.id = 0x1839F381;
    msg.buf[0] = module;
    msg.buf[1] = lowTemp(module);
    msg.buf[2] = highTemp(module);
    msg.buf[3] = avgTemp(module);;
    msg.buf[4] = 80;
    if(module == 1){
        msg.buf[5] = 0;
        msg.buf[6] = 79;
    }
    else{
        msg.buf[5] = 80;                  //THESE ARE PROB WRONG
        msg.buf[6] = 159;                 //THESE ARE PROB WRONG
    }
    
    msg.buf[7] = 0;//ihnc                 //THESE ARE PROB WRONG
    can1.write(msg);
}

void writeGeneralBroadcast(byte module){
    if(module == 1)
        msg.id = 0x1838F380;
    else
        msg.id = 0x1838F381;
    int start = 80;
    if(module == 1)
        int start = 0;

    for(int id = start; id<start+80; id++){
        msg.buf[0] = 0;
        msg.buf[1] = (byte)id;
        msg.buf[2] = temps[id/10][id%10];
    }
    
    msg.buf[3] = 80;
    msg.buf[4] = lowTemp(module);
    msg.buf[5] = highTemp(module);
    if(module == 1){
        msg.buf[6] = 0;
        msg.buf[7] = 79;
    }
    else{
        msg.buf[6] = 80;                  //THESE ARE PROB WRONG
        msg.buf[7] = 159;                 //THESE ARE PROB WRONG
    }
    can1.write(msg);
}

void writeAddyClaim(byte module){
    
}

byte lowTemp(byte module){
    byte start = 80;
    byte stop = 159;
    if(module == 1){
        start = 0;
        stop = 79;
    }
    byte lowest = temps[0][0];
    for(int i=start; i<=stop; i++){
        for(int o=0; o<8; o++){
            if(temps[i][o] < lowest)
                lowest = temps[i][o];
        }
    }
    return lowest;
}
byte highTemp(byte module){
    byte start = 80;
    byte stop = 159;
    if(module == 1){
        start = 0;
        stop = 79;
    }
    byte highest = temps[0][0];
    for(int i=start; i<=stop; i++){
        for(int o=0; o<8; o++){
            if(temps[i][o] > highest)
                highest = temps[i][o];
        }
    }
    return highest;
}
byte avgTemp(byte module){
    byte start = 80;
    byte stop = 159;
    if(module == 1){
        start = 0;
        stop = 79;
    }
    int total=0;
    for(int i=start; i<=stop; i++){
        for(int o=0; o<8; o++){
            total += temps[i][o];
        }
    }
    return (byte)(total/80);
}
