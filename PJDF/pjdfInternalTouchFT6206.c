/*
    pjdfInternalTouchFT6206.c
    The implementation of the internal PJDF interface pjdfInternal.h targetted for the
    FT6206 Touch Screen

    Developed for University of Washington embedded systems programming certificate
    
    FEB-2017 Elizabeth Purswell wrote after (Nick Strathy wrote/arranged it after a framework by Paul Lever)
*/

#include "bsp.h"
#include "pjdf.h"
#include "pjdfInternal.h"
#include "Adafruit_FT6206.h"

// Control registers etc for Touch FT6206 hardware
typedef struct _PjdfContextTft6206
{
  uint8_t registerID; // for setting what register(s) to read -- 0 indicates read 16 of them
} PjdfContextTft6206;

static PjdfContextTft6206 tft6206Context = { 0 }; // set initial register to 0

// OpenTFT6206
static PjdfErrCode OpenTFT6206(DriverInternal *pDriver, INT8U flags)
{

    return PJDF_ERR_NONE;
}

// CloseTFT6206

static PjdfErrCode CloseTFT6206(DriverInternal *pDriver)
{
    return PJDF_ERR_NONE;
}

// ReadTFT6206
// Read byte(s) from I2C
//
// pDriver: pointer to an initialized FT6206 device
// pBuffer: place to put data read by I2C
// pCount: how many bytes to read
// Returns: PJDF_ERR_NONE if there was no error, otherwise an error code.
static PjdfErrCode ReadTFT6206(DriverInternal *pDriver, void* pBuffer, INT32U* pCount) {

  PjdfContextTft6206 *pContext = (PjdfContextTft6206*) pDriver->deviceContext;
  
  if (pContext == NULL) while(1);
  
  
  I2C_start(I2C1, FT6206_ADDR<<1, I2C_Direction_Transmitter); 
  I2C_write(I2C1, (uint8_t)pContext->registerID); 
  I2C_stop(I2C1); 
  I2C_start(I2C1, FT6206_ADDR<<1, I2C_Direction_Receiver);      
  
  uint8_t i;
  uint8_t *pointer;
  uint8_t *initialPointer;
  pointer = (uint8_t *)pBuffer;
  initialPointer = (uint8_t *)pBuffer;
  
  if(pContext->registerID == READ_16_REGISTERS) {   
    
    // populate the buffer
    for (i=0; i<15; i++){
      *pointer = I2C_read_ack(I2C1); 
      pointer++;
    }
    *pointer = I2C_read_nack(I2C1);
    pBuffer = initialPointer;
   }
  else{
    *pointer = I2C_read_nack(I2C1);
    pBuffer = initialPointer;
  }
  return PJDF_ERR_NONE;
}

// WriteTFT6206
// Write to I2C
//
// pDriver: pointer to an initialized FT6206 device
// pBuffer: not used
// pdata: register to write to or data to write there
// Returns: PJDF_ERR_NONE if there was no error, otherwise an error code.
static PjdfErrCode WriteTFT6206(DriverInternal *pDriver, void* pBuffer, INT32U* pdata)
{
  uint8_t *pointer;
  pointer = (uint8_t *)pdata;
  
  PjdfContextTft6206 *pContext = (PjdfContextTft6206*) pDriver->deviceContext;
  if (pContext == NULL) while(1);
  
  I2C_start(I2C1, FT6206_ADDR<<1, I2C_Direction_Transmitter);
  I2C_write(I2C1, pContext->registerID);
  I2C_write(I2C1, *pointer);  
  I2C_stop(I2C1);
  
  return PJDF_ERR_NONE;
}

// IoctlTFT6206
// Handles the request codes defined in pjdfCtrlTouchFT6206.h
static PjdfErrCode IoctlTFT6206(DriverInternal *pDriver, INT8U request, void* pArgs, INT32U* pSize)
{
    PjdfContextTft6206 *pContext = (PjdfContextTft6206*) pDriver->deviceContext;
    if (pContext == NULL) while(1);
    
    switch (request)
    {
    case PJDF_CTRL_TOUCH_FT6206_SET_READ_REGISTER:
      uint8_t newregID;
      uint8_t *pnewregID;
      pnewregID = (uint8_t *)pArgs;
      newregID = *pnewregID;
      pContext->registerID = newregID; // sets which register to read from
      break;
    default:
        while(1);
        break;
    }
    return PJDF_ERR_NONE;
}

// Initializes the given FT6206.
PjdfErrCode InitTFT6206(DriverInternal *pDriver, char *pName)
{   
    if (strcmp (pName, pDriver->pName) != 0) while(1); // pName should have been initialized in driversInternal[] declaration
    
    // Initialize semaphore for serializing operations on the device 
    pDriver->sem = OSSemCreate(1); 
    if (pDriver->sem == NULL) while (1);  // not enough semaphores available
    pDriver->refCount = 0; // initial number of Open handles to the device
    
        I2C1_init(); // init I2C1 hardware
     if (strcmp(pName, PJDF_DEVICE_ID_TOUCH_FT6206) == 0)
    {
        pDriver->maxRefCount = 10; // Maximum refcount allowed for the device
        pDriver->deviceContext = (void*) &tft6206Context;
       I2C1_init(); // init I2C1 hardware
    }

  
    // Assign implemented functions to the interface pointers
    pDriver->Open = OpenTFT6206;
    pDriver->Close = CloseTFT6206;
    pDriver->Read = ReadTFT6206;
    pDriver->Write = WriteTFT6206;
    pDriver->Ioctl = IoctlTFT6206;
    
    pDriver->initialized = OS_TRUE;
    return PJDF_ERR_NONE;
}

