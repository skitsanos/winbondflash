/*
	Winbond spi flash memory chip operating library for Arduino
	by WarMonkey (luoshumymail@gmail.com)
	for more information, please visit bbs.kechuang.org
	latest version available on http://code.google.com/p/winbondflash
*/

#ifndef _WINBONDFLASH_H__
#define _WINBONDFLASH_H__

#include <inttypes.h>
#include <SPI.h>

//W25Q64 = 256_bytes_per_page * 16_pages_per_sector * 16_sectors_per_block * 128_blocks_per_chip
//= 256b*16*16*128 = 8Mbyte = 64MBits

#define _W25Q80  winbondFlashClass::W25Q80
#define _W25Q16  winbondFlashClass::W25Q16
#define _W25Q32  winbondFlashClass::W25Q32
#define _W25Q64  winbondFlashClass::W25Q64
#define _W25Q128 winbondFlashClass::W25Q128

class winbondFlashClass {
public:  
  enum partNumber {
    custom = -1,
    autoDetect = 0,
    W25Q80 = 1,
    W25Q16 = 2,
    W25Q32 = 4,
    W25Q64 = 8,
    W25Q128 = 16
  };

  bool begin(partNumber _partno = autoDetect);
  void end();

  long bytes();
  uint16_t pages();
  uint16_t sectors();
  uint16_t blocks();

  uint16_t read (uint32_t addr,uint8_t *buf,uint16_t n=256);

  void setWriteEnable(bool cmd = true);
  bool writeEnable();
  
  //setWriteEnable(true) before write or erase
  uint16_t write(uint32_t addr,uint8_t *buf,uint16_t n=256);

  bool erase(uint32_t addr,uint32_t n=4096);
  //auto erase: sector(s), 32k block(s), 64k block(s) select by size
  bool eraseSector(uint32_t addr_start,uint32_t n=4096);
  //erase multiple sectors ( 4096bytes ), return false if error
  bool erase32kBlock(uint32_t addr_start,uint32_t n=32768);
  //erase multiple 32k blocks ( 32768b )
  bool erase64kBlock(uint32_t addr_start,uint32_t n=65536);
  //erase multiple 64k blocks ( 65536b )
  bool eraseAll();
  //chip erase, return true if successfully started, busy()==false -> erase complete

  void eraseSuspend();
  void eraseResume();

  bool busy();
  bool sync();//wait all operations complete
  //return true when exit normally, return false = timeout

  inline void setTimeout(uint16_t ms) {m_timeout=ms;}
  //timeout == 0 -> do once, timeout < 0 -> wait forever
  inline uint16_t timeout() {return m_timeout;}
  
  uint8_t  readManufacturer();
  uint16_t readPartID();
  uint64_t readUniqueID();
  uint16_t readSR();

private:
  partNumber partno;
  uint16_t m_timeout;
  uint8_t w_cmd;
  
  bool checkPartNo(partNumber _partno);
  void setWEL();

protected:
  virtual void select() = 0;
  virtual uint8_t transfer(uint8_t x) = 0;
  virtual void deselect() = 0;
  
};

class winbondFlashSPI: public winbondFlashClass {
private:
  uint8_t nss;
  SPIClass spi;
  inline void select() {
    digitalWrite(nss,LOW);
  }

  inline uint8_t transfer(uint8_t x) {
    byte y = spi.transfer(x);
    return y;
  }

  inline void deselect() {
    digitalWrite(nss,HIGH);
  }

public:
  bool begin(partNumber _partno = autoDetect,SPIClass &_spi = SPI,uint8_t _nss = SS);
  void end();
};

#endif

