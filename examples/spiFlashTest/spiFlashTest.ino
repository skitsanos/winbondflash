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
  byte buf[256] = "Hello World by WarMonkey burn into W25Q64 dataflash";
  /*
  Serial.println("Prepare write");
  mem.setWriteEnable();
  mem.write(0,buf,256);
  */
  mem.setWriteEnable(false);
  mem.read(0,buf,256);
  for(byte i=0;i<=128;i++)
  {
    Serial.print((char)buf[i]);
    delay(1);
  }
  Serial.println(" ");  
  delay(1000);
}
