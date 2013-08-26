/*
	Winbond spi flash memory chip operating library for Arduino
	by WarMonkey (luoshumymail@gmail.com)
	for more information, please visit bbs.kechuang.org
	latest version available on http://code.google.com/p/winbondflash
*/

#include <Arduino.h>
#include <SPI.h>
#include <Metro.h>
#include <errno.h>
#include "winbondflash.h"

//COMMANDS
#define W_EN 	0x06	//write enable
#define W_DE	0x04	//write disable
#define R_SR1	0x05	//read status reg 1
#define R_SR2	0x35	//read status reg 2
#define W_SR	0x01	//write status reg
#define PAGE_PGM	0x02	//page program
#define QPAGE_PGM	0x32	//quad input page program
#define BLK_E_64K	0xD8	//block erase 64KB
#define BLK_E_32K	0x52	//block erase 32KB
#define SECTOR_E	0x20	//sector erase 4KB
#define CHIP_ERASE	0xc7	//chip erase
#define CHIP_ERASE2	0x60	//=CHIP_ERASE
#define E_SUSPEND	0x75	//erase suspend
#define E_RESUME	0x7a	//erase resume
#define PDWN		0xb9	//power down
#define HIGH_PERF_M	0xa3	//high performance mode
#define CONT_R_RST	0xff	//continuous read mode reset
#define RELEASE		0xab	//release power down or HPM/Dev ID (deprecated)
#define R_MANUF_ID	0x90	//read Manufacturer and Dev ID (deprecated)
#define R_UNIQUE_ID	0x4b	//read unique ID (suggested)
#define R_JEDEC_ID	0x9f	//read JEDEC ID = Manuf+ID (suggested)
#define READ		0x03
#define FAST_READ	0x0b

#define SR1_BUSY_MASK	0x01
#define SR1_WEN_MASK	0x02

#define WINBOND_MANUF	0xef

#define DEFAULT_TIMEOUT 200

typedef struct {
    winbondFlashClass::partNumber pn;
    uint16_t id;
    uint32_t bytes;
    uint16_t pages;
    uint16_t sectors;
    uint16_t blocks;
}pnListType;
  
static const pnListType pnList[] PROGMEM = {
    { winbondFlashClass::W25Q80, 0x4014,1048576, 4096, 256, 16  },
    { winbondFlashClass::W25Q16, 0x4015,2097152, 8192, 512, 32  },
    { winbondFlashClass::W25Q32, 0x4016,4194304, 16384,1024,64  },
    { winbondFlashClass::W25Q64, 0x4017,8388608, 32768,2048,128 },
    { winbondFlashClass::W25Q128,0x4018,16777216,65536,4096,256 }
};
  
void winbondFlashClass::setWriteEnable(bool cmd)
{
  w_cmd = cmd ? W_EN : W_DE;
}

bool winbondFlashClass::writeEnable()
{
  if(w_cmd == W_EN)
    return true;
  w_cmd = W_DE;
  return false;
}

uint16_t winbondFlashClass::readSR()
{
  uint8_t r1,r2;
  select();
  transfer(R_SR1);
  r1 = transfer(0xff);
  deselect();
  deselect();//some delay
  select();
  transfer(R_SR2);
  r2 = transfer(0xff);
  deselect();
  return (((uint16_t)r2)<<8)|r1;
}

uint8_t winbondFlashClass::readManufacturer()
{
  uint8_t c;
  select();
  transfer(R_JEDEC_ID);
  c = transfer(0x00);
  transfer(0x00);
  transfer(0x00);
  deselect();
  return c;
}

uint64_t winbondFlashClass::readUniqueID()
{
  uint64_t uid;
  uint8_t *arr;
  arr = (uint8_t*)&uid;
  select();
  transfer(R_UNIQUE_ID);
  transfer(0x00);
  transfer(0x00);
  transfer(0x00);
  transfer(0x00);
  //for little endian machine only
  for(int i=7;i>=0;i--)
  {
    arr[i] = transfer(0x00);
  }
  deselect();
  return uid;
}

uint16_t winbondFlashClass::readPartID()
{
  uint8_t a,b;
  select();
  transfer(R_JEDEC_ID);
  transfer(0x00);
  a = transfer(0x00);
  b = transfer(0x00);
  deselect();
  return (a<<8)|b;
}

bool winbondFlashClass::checkPartNo(partNumber _partno)
{
  uint8_t manuf;
  uint16_t id;
  
  select();
  transfer(R_JEDEC_ID);
  manuf = transfer(0x00);
  id = transfer(0x00) << 8;
  id |= transfer(0x00);
  deselect();

  Serial.print("MANUF=0x");
  Serial.print(manuf,HEX);
  Serial.print(",ID=0x");
  Serial.print(id,HEX);
  Serial.println();
  
  if(manuf != WINBOND_MANUF)
    return false;
  Serial.println("MANUF OK");

  if(_partno == custom)
    return true;
  Serial.println("Not a custom chip type");

  if(_partno == autoDetect)
  {
    Serial.print("Autodetect...");
    for(int i=0;i<sizeof(pnList)/sizeof(pnList[0]);i++)
    {
      if(id == pgm_read_word(&(pnList[i].id)))
      {
        _partno = (partNumber)pgm_read_byte(&(pnList[i].pn));
        Serial.println("OK");
        return true;
      }
    }
    if(_partno == autoDetect)
    {
      Serial.println("Failed");
      return false;
    }
  }

  //test chip id and partNo
  for(int i=0;i<sizeof(pnList)/sizeof(pnList[0]);i++)
  {
    if(_partno == (partNumber)pgm_read_byte(&(pnList[i].pn)))
    {
      if(id == pgm_read_word(&(pnList[i].id)))//id equal
        return true;
      else
        return false;
    }
  }
  Serial.println("partNumber not found");
  return false;//partNo not found
}

bool winbondFlashClass::busy()
{
  uint8_t r1;
  select();
  transfer(R_SR1);
  r1 = transfer(0xff);
  deselect();
  if(r1 & SR1_BUSY_MASK)
    return true;
  return false;
}

bool winbondFlashClass::sync(void)
{
  Metro tt(m_timeout);
  uint8_t c;
  select();
  transfer(R_SR1);
  do {
    c = transfer(0x00);
    //Serial.println(c,HEX);
    if((c & SR1_BUSY_MASK) == 0) //not busy
    {
      deselect();
      return true;
    }
//    if(timeout == 0) break;
  }while(tt.check() == 0|| (m_timeout < 0));
  deselect();
  return false;
}

long winbondFlashClass::bytes()
{
  for(int i=0;i<sizeof(pnList)/sizeof(pnList[0]);i++)
  {
    if(partno == (partNumber)pgm_read_byte(&(pnList[i].pn)))
    {
      return pgm_read_dword(&(pnList[i].bytes));
    }
  }
  return 0;
}

uint16_t winbondFlashClass::pages()
{
  for(int i=0;i<sizeof(pnList)/sizeof(pnList[0]);i++)
  {
    if(partno == (partNumber)pgm_read_byte(&(pnList[i].pn)))
    {
      return pgm_read_word(&(pnList[i].pages));
    }
  }
  return 0;
}

uint16_t winbondFlashClass::sectors()
{
  for(int i=0;i<sizeof(pnList)/sizeof(pnList[0]);i++)
  {
    if(partno == (partNumber)pgm_read_byte(&(pnList[i].pn)))
    {
      return pgm_read_word(&(pnList[i].sectors));
    }
  }
  return 0;
}

uint16_t winbondFlashClass::blocks()
{
  for(int i=0;i<sizeof(pnList)/sizeof(pnList[0]);i++)
  {
    if(partno == (partNumber)pgm_read_byte(&(pnList[i].pn)))
    {
      return pgm_read_word(&(pnList[i].blocks));
    }
  }
  return 0;
}

bool winbondFlashClass::begin(partNumber _partno)
{
  m_timeout = DEFAULT_TIMEOUT;
  w_cmd = W_DE;
  
  select();
  transfer(RELEASE);
  deselect();
  delayMicroseconds(5);//>3us
  Serial.println("Chip Released");
  
  if(!checkPartNo(_partno))
    return false;
}

void winbondFlashClass::end()
{
  select();
  transfer(PDWN);
  deselect();
  delayMicroseconds(5);//>3us
}

void winbondFlashClass::setWEL()
{
  select();
  transfer(w_cmd);
  deselect();
}

uint16_t winbondFlashClass::read (uint32_t addr,uint8_t *buf,uint16_t n)
{
  if(!sync())
    return 0;
    
  select();
  transfer(READ);
  transfer(addr>>16);
  transfer(addr>>8);
  transfer(addr);
  for(uint16_t i=0;i<n;i++)
  {
    buf[i] = transfer(0x00);
  }
  deselect();
  
  return n;
}

uint16_t winbondFlashClass::write(uint32_t addr,uint8_t *buf,uint16_t n)
{
  uint16_t i = 0;
  
  if(addr & 0x000000ff)
  {
    uint8_t k;
    k = 0xff - (uint8_t)addr + 1;
    //remaining bytes in current page
    if(!sync())
    {
      Serial.println("Busy: timeout");
      return 0;
    }
    setWEL();
    //write min(n,k) bytes
    uint8_t a = n > k ? k : n;
    select();
    transfer(PAGE_PGM);
    transfer((uint8_t)(addr>>16));
    transfer((uint8_t)(addr>>8));
    transfer((uint8_t)addr);
    for(i=0;i<a;i++)
    {
      transfer(buf[i]);
    }
    deselect();
    
    if(n<=k) //all data written
      return i;
      
    n -= k;
    addr = addr&0x00ffff00 + 256UL;
  }
  
  uint16_t end_page = (addr + (uint32_t)n) >> 8;
  for(uint16_t start_page=addr>>8;start_page<end_page;start_page++)
  {
    if(!sync())
    {
      //Serial.println("Busy: timeout");
      return i;
    }
    //Serial.println("Sync OK");
    setWEL();
    select();
    transfer(PAGE_PGM);
    transfer(start_page>>8);
    transfer(start_page);
    transfer(0x00);
    uint8_t *tbuf = buf+i;
    uint8_t j = 0;
    do {
      transfer(tbuf[j]);
      j++;
    }while(j != 0);
    i+=256;
    deselect();
  }
  
  if(i < n)
  {
    if(!sync())
      return i;
    setWEL();
    select();
    transfer(PAGE_PGM);
    transfer(end_page>>8);
    transfer(end_page);
    transfer(0x00);
    uint8_t *tbuf = buf+i;
    for(uint8_t j=0;j<=n-i;j++)
      transfer(tbuf[j]);
    deselect();
  }
  
  return n;
}

bool winbondFlashClass::erase(uint32_t addr,uint32_t n)
{
  return false;
}

bool winbondFlashClass::eraseSector(uint32_t addr_start,uint32_t n)
{
  if(n==0)
    return false;
  if(n%4096) //test if n is 12bit-aligned
    return false;
  if(addr_start & 0x00000fff) //test if addr_start is 12bit-aligned
    return false;
    
  setWEL();
  select();
  transfer(SECTOR_E);
  transfer(addr_start>>16);
  transfer(addr_start>>8);
  transfer(addr_start);
  deselect();
  
  return true;  
}

bool winbondFlashClass::erase32kBlock(uint32_t addr_start,uint32_t n)
{
  if(n==0)
    return false;
  if(n%32768) //test if n is 15bit-aligned
    return false;
  if(addr_start & 0x00007fff) //test if addr_start is 15bit-aligned
    return false;
    
  setWEL();
  select();
  transfer(BLK_E_32K);
  transfer(addr_start>>16);
  transfer(addr_start>>8);
  transfer(addr_start);
  deselect();
  
  return true;
}

bool winbondFlashClass::erase64kBlock(uint32_t addr_start,uint32_t n)
{
  if(n==0)
    return false;
  if(n%65536) //test if n is 16bit-aligned
    return false;
  if(addr_start & 0x0000ffff) //test if addr_start is 16bit-aligned
    return false;
    
  setWEL();
  select();
  transfer(BLK_E_64K);
  transfer(addr_start>>16);
  transfer(addr_start>>8);
  transfer(addr_start);
  deselect();
  
  return true;
}

bool winbondFlashClass::eraseAll()
{
  if(!sync())
    return false;
  setWEL();
  select();
  transfer(CHIP_ERASE);
  deselect();
  return true;
}

void winbondFlashClass::eraseSuspend()
{
  select();
  transfer(E_SUSPEND);
  deselect();
}

void winbondFlashClass::eraseResume()
{
  select();
  transfer(E_RESUME);
  deselect();
}

bool winbondFlashSPI::begin(partNumber _partno,SPIClass &_spi,uint8_t _nss)
{
  spi = _spi;
  nss = _nss;

  pinMode(MISO,INPUT_PULLUP);
  spi.begin();
  spi.setBitOrder(MSBFIRST);
  spi.setClockDivider(SPI_CLOCK_DIV2);
  spi.setDataMode(SPI_MODE0);
  deselect();
  Serial.println("SPI OK");

  return winbondFlashClass::begin(_partno);
}

void winbondFlashSPI::end()
{
  winbondFlashClass::end();
  spi.end();
}



