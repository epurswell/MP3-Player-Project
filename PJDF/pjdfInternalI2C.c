
/*
    pjdfInternalI2C.c
    The implementation of the internal PJDF interface pjdfInternal.h targetted for the
    Inter-Integrated Circuit (I2C) bus

    Developed for University of Washington embedded systems programming certificate
    
    Feb-2017 written by Elizabeth Purswell after Nick Strathy and Paul Lever
*/

#include "bsp.h"
#include "pjdf.h"
#include "pjdfInternal.h"

// Control registers etc for I2C hardware
typedef struct _PjdfContextI2c
{
    I2C_TypeDef *i2cMemMap; // Memory mapped register block for a I2C interface
    
} PjdfContextI2c;

static PjdfContextI2c i2c1Context = { PJDF_I2C1 };

// here are empty functions 
static PjdfErrCode OpenI2C(DriverInternal *pDriver, INT8U flags) {return PJDF_ERR_NONE;}
static PjdfErrCode CloseI2C(DriverInternal *pDriver) {return PJDF_ERR_NONE;}
static PjdfErrCode ReadI2C(DriverInternal *pDriver, void* pBuffer, INT32U* pCount) {return PJDF_ERR_NONE;}
static PjdfErrCode WriteI2C(DriverInternal *pDriver, void* pBuffer, INT32U* pCount) {return PJDF_ERR_NONE;}
static PjdfErrCode IoctlI2C(DriverInternal *pDriver, INT8U request, void* pArgs, INT32U* pSize) {return PJDF_ERR_NONE;}

#if 0

// OpenI2C
// No special action required to open I2C device
static PjdfErrCode OpenI2C(DriverInternal *pDriver, INT8U flags)
{
    // Nothing to do
    return PJDF_ERR_NONE;
}

// CloseI2C
// No special action required to close I2C device
static PjdfErrCode CloseI2C(DriverInternal *pDriver)
{
    // Nothing to do
    return PJDF_ERR_NONE;
}

// ReadI2C
// Reading from I2C
//
// pDriver: pointer to an initialized SPI device
// pBuffer: on input the data to write to the device, on output the data output by the device
//     note: the buffer must not reside in readonly memory.
// pCount: the number of bytes to write/read.
// Returns: PJDF_ERR_NONE if there was no error, otherwise an error code.
static PjdfErrCode ReadI2C(DriverInternal *pDriver, void* i2cdat, INT32U* pCount)
{
    PjdfContextI2c *pContext = (PjdfContextI2c*) pDriver->deviceContext;
    if (pContext == NULL) while(1);
//    I2C_read();
    uint8_t i2cdat[16];
  I2C_start(I2C1, FT6206_ADDR<<1, I2C_Direction_Transmitter);
  I2C_write(I2C1, (uint8_t)0);  
  I2C_stop(I2C1);
  I2C_start(I2C1, FT6206_ADDR<<1, I2C_Direction_Receiver);
  //Wire.requestFrom((uint8_t)FT6206_ADDR, (uint8_t)32);
  
  uint8_t i;
  for (i=0; i<15; i++)
    i2cdat[i] = I2C_read_ack(I2C1);
  i2cdat[i] = I2C_read_nack(I2C1);

    return PJDF_ERR_NONE;
}


// WriteSPI
// Writes the contents of the buffer to the given device. The caller must first
// assert the appropriate slave chip select line before calling writeSPI.
//
// pDriver: pointer to an initialized SPI device
// pBuffer: the data to write to the device
// pCount: the number of bytes to write
// Returns: PJDF_ERR_NONE if there was no error, otherwise an error code.
static PjdfErrCode WriteI2C(DriverInternal *pDriver, void* pBuffer, INT32U* pCount)
{
    PjdfContextI2c *pContext = (PjdfContextI2c*) pDriver->deviceContext;
    if (pContext == NULL) while(1);
    // do some I2c writing here
    return PJDF_ERR_NONE;
}

// IoctlSPI
// Handles the request codes defined in pjdfCtrlI2C.h
static PjdfErrCode IoctlI2C(DriverInternal *pDriver, INT8U request, void* pArgs, INT32U* pSize)
{
    INT8U osErr;
    PjdfContextI2c *pContext = (PjdfContextI2c*) pDriver->deviceContext;
    if (pContext == NULL) while(1);
    switch (request)
    {
    case PJDF_CTRL_I2C_WAIT_FOR_LOCK:
        OSSemPend(pDriver->sem, 0, &osErr);
        break;
    case PJDF_CTRL_I2C_RELEASE_LOCK:
        osErr = OSSemPost(pDriver->sem);
        break;
    default:
        while(1);
        break;
    }
    return PJDF_ERR_NONE;
}

#endif // #if 0
// Initializes the given I2C driver.
PjdfErrCode InitI2C(DriverInternal *pDriver, char *pName)
{   
    if (strcmp (pName, pDriver->pName) != 0) while(1); // pName should have been initialized in driversInternal[] declaration
    
    // Initialize semaphore for serializing operations on the device 
    pDriver->sem = OSSemCreate(1); 
    if (pDriver->sem == NULL) while (1);  // not enough semaphores available
    pDriver->refCount = 0; // initial number of Open handles to the device
    
    // We may choose to handle multiple hardware instances of the SPI interface
    // each of which gets its own DriverInternal struct. Here we initialize 
    // the context of the SPI hardware instance specified by pName.
    if (strcmp(pName, PJDF_DEVICE_ID_I2C1) == 0)
    {
        pDriver->maxRefCount = 10; // Maximum refcount allowed for the device
        pDriver->deviceContext = (void*) &i2c1Context;
        I2C1_init(); // initI2C1 hardware
    }
  
    // Assign implemented functions to the interface pointers
    pDriver->Open = OpenI2C;
    pDriver->Close = CloseI2C;
    pDriver->Read = ReadI2C;
    pDriver->Write = WriteI2C;
    pDriver->Ioctl = IoctlI2C;
    
    pDriver->initialized = OS_TRUE;
    return PJDF_ERR_NONE;
}