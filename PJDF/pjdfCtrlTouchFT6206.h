/*
    pjdfCtrlTouchFT6206.h
    FT6206 Touch screen control definitions exposed to applications

    Developed for University of Washington embedded systems programming certificate
    
    FEB-2017 Writeen by Elizabeth Purswell after (2016/2 Nick Strathy wrote/arranged it after a framework by Paul Lever)
*/

#ifndef __PJDFCTRLTOUCHFT6206_H__
#define __PJDFCTRLTOUCHFT6206_H__


// Control definitions for Touch Screen FT6206

#define PJDF_CTRL_TOUCH_FT6206_SET_READ_REGISTER  0x01   // Wait for exclusive access to Touch Screen, then lock it

#define READ_16_REGISTERS 0x00 // for doing reading of 16 registers

#endif