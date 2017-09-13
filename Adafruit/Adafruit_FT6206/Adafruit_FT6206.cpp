/*************************************************** 
  This is a library for the Adafruit Capacitive Touch Screens

  ----> http://www.adafruit.com/products/1947
 
  Check out the links above for our tutorials and wiring diagrams
  This chipset uses I2C to communicate

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
// FEB-2017 Updated by Elizabeth Purswell to use pjdf driver calls for Touch
//  EJP -- take out all the "Serial Print" lines so I don't get a headache
//  EJP -- take out the Wire. call -- I'm not using them

#include <Adafruit_FT6206.h>
#include "bspI2c.h"  //EJP note -- I shouldn't need this one when this is done
#include "pjdf.h" // EJP

#if defined(__SAM3X8E__)
    #define Wire Wire1
#endif

void delay(uint32_t);

/**************************************************************************/
/*! 
    @brief  Instantiates a new FT6206 class
*/
/**************************************************************************/

Adafruit_FT6206::Adafruit_FT6206() {
  hTouch = 0; // EJP need a handle
}

//***********************************
//   Get handle of Touch Driver EJP (Just copy what it does in ILI9341.cpp file)
//*********************************/
void Adafruit_FT6206::setPjdfHandle(HANDLE hTouch) {
  this->hTouch = hTouch;
} 

/**************************************************************************/
/*! 
    @brief  Setups the HW
*/
/**************************************************************************/
boolean Adafruit_FT6206::begin(uint8_t threshhold) {
  
  uint8_t readVendID;
  uint8_t readChipID;  

  uint8_t thresholdRegister;
  thresholdRegister = FT6206_REG_THRESHHOLD;
  
  Ioctl(hTouch, PJDF_CTRL_TOUCH_FT6206_SET_READ_REGISTER, (void *)&thresholdRegister,0);
  Write(hTouch, 0, (INT32U *)&threshhold); // write threshold into Threshold register
  
  readVendID = readRegister8(FT6206_REG_VENDID);
  readChipID = readRegister8(FT6206_REG_CHIPID);
  
  uint8_t vendIDregister;
  vendIDregister = FT6206_REG_VENDID;
  Ioctl(hTouch, PJDF_CTRL_TOUCH_FT6206_SET_READ_REGISTER, (void *)&vendIDregister,0);
  Read(hTouch, &readVendID, 0); // read the vendID from register
  
  uint8_t chipIDregister;
  chipIDregister = FT6206_REG_CHIPID;
   Ioctl(hTouch, PJDF_CTRL_TOUCH_FT6206_SET_READ_REGISTER, (void *)&chipIDregister,0);
  Read(hTouch, &readChipID, 0); // read the ChipID from register
  
   if ((readVendID != 17) || (readChipID != 6)) 
    return false;
  return true;
  
}

// EJP took out the DON'T DO THIS function

boolean Adafruit_FT6206::touched(void) {
  
  uint8_t n;
  
  uint8_t numTouchesRegister;
  numTouchesRegister = FT6206_REG_NUMTOUCHES;
  
  Ioctl(hTouch, PJDF_CTRL_TOUCH_FT6206_SET_READ_REGISTER, (void *)&numTouchesRegister, 0);
  Read(hTouch, &n, 0);
  
  if ((n == 1) || (n == 2)) return true;
  return false;
}

/*****************************/

void Adafruit_FT6206::readData(uint16_t *x, uint16_t *y) {
  
  uint8_t i2cdat[16];
  
  uint8_t read16Register;
  read16Register = READ_16_REGISTERS;
  Ioctl(hTouch, PJDF_CTRL_TOUCH_FT6206_SET_READ_REGISTER,(void *) &read16Register, 0);
  Read(hTouch, i2cdat, 0);
  
  touches = i2cdat[0x02];
  
  if (touches > 2) {
    touches = 0;
    *x = *y = 0;
  }
  if (touches == 0) {
    *x = *y = 0;
    return;
  }
  
  for (uint8_t i=0; i<2; i++) {
    touchX[i] = i2cdat[0x03 + i*6] & 0x0F;
    touchX[i] <<= 8;
    touchX[i] |= i2cdat[0x04 + i*6]; 
    touchY[i] = i2cdat[0x05 + i*6] & 0x0F;
    touchY[i] <<= 8;
    touchY[i] |= i2cdat[0x06 + i*6];
    touchID[i] = i2cdat[0x05 + i*6] >> 4;
  }
  
  *x = touchX[0]; *y = touchY[0];
}

TS_Point Adafruit_FT6206::getPoint(void) {
  uint16_t x, y;
  readData(&x, &y);
  return TS_Point(x, y, 1);
}


uint8_t Adafruit_FT6206::readRegister8(uint8_t reg) {
  uint8_t x ;

  Ioctl(hTouch, PJDF_CTRL_TOUCH_FT6206_SET_READ_REGISTER,(void *) &reg, 0);
  Read(hTouch, &x, 0);



  return x;
}

void Adafruit_FT6206::writeRegister8(uint8_t reg, uint8_t val) {
   
  Ioctl(hTouch, PJDF_CTRL_TOUCH_FT6206_SET_READ_REGISTER,(void *) &reg, 0);
  Write(hTouch, &val, 0);

}

/****************/

TS_Point::TS_Point(void) {
  x = y = 0;
}

TS_Point::TS_Point(int16_t x0, int16_t y0, int16_t z0) {
  x = x0;
  y = y0;
  z = z0;
}

bool TS_Point::operator==(TS_Point p1) {
  return  ((p1.x == x) && (p1.y == y) && (p1.z == z));
}

bool TS_Point::operator!=(TS_Point p1) {
  return  ((p1.x != x) || (p1.y != y) || (p1.z != z));
}
