/*
    pjdfCtrlI2c.h
    I2C control definitions exposed to applications

    Developed for University of Washington embedded systems programming certificate
    
      Feb-2017 Elizabeth Purswell wrote after Nick Strathy and Paul Lever
*/

#ifndef __PJDFCTRLI2C_H__
#define __PJDFCTRLI2C_H__


// Control definitions for I2C1

#define PJDF_CTRL_I2C_WAIT_FOR_LOCK  0x01   // Wait for exclusive access to I2C, then lock it
#define PJDF_CTRL_I2C_RELEASE_LOCK   0x02   // Release exclusive I2C lock

#endif