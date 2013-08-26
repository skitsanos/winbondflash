#include <SPI.h>
#include <Metro.h>
#include "winbondflash.h"

winbondFlashSPI mem;

void setup()
{
  Serial.begin(9600);
  Serial.println("Init Chip...");
  if(mem.begin(_W25Q64,SPI,SS))
    Serial.println("OK");
  else
  {
    Serial.println("FAILED");
    while(1);
  }
}

void loop()
{
  byte buf1[255] = "Hello World by WarMonkey burn into W25Q64 dataflash";
  byte buf2[128];
  
  Serial.println("Prepare write");
  mem.setWriteEnable(true);
  /*
  digitalWrite(SS,LOW);
  SPI.transfer(0x06);
  digitalWrite(SS,HIGH);
  uint16_t x = mem.readSR();
  */
  //uint16_t x = mem.sync();
  //Serial.print(x>>8,HEX);
  //Serial.println(x,HEX);
  
  static uint16_t xx = 0;
  Serial.println(mem.write(xx,buf1,256),DEC);
  delay(1000);
  
  Serial.println("Read Test:");
  for(byte i=0;i<255;i++)
  {
    buf2[i] = 0;
  }
  mem.read(xx,buf2,256);
  for(byte i=0;i<255;i++)
  {
    Serial.print((char)buf2[i]);
    delay(1);
  }
  Serial.println(" ");  
  
  xx+=256;
  Serial.println(xx>>8);
  
  delay(1000);
  
  if(Serial.available() > 2)
  {
    mem.erase32kBlock(0);
    while(mem.busy());
  }
  
}
