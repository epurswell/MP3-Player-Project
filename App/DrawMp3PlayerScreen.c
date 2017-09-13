// this file has all the LCD screen little functions

#include <stdarg.h>

#include "os_cpu.h"
#include "bsp.h"
#include "print.h"
#include "mp3Util.h"
#include "pjdf.h"
#include "SD.h"
#include "DrawMp3PlayerScreen.h"

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>

extern Adafruit_ILI9341 lcdCtrl;

#define BUFSIZE 256

void DecodeTimeUpdate(INT8U decodeTime,INT8U progress)
{
  char buf[BUFSIZE];
  
      lcdCtrl.setCursor(40, 80);
      lcdCtrl.setTextColor(ILI9341_WHITE, ILI9341_BLUE); 
      lcdCtrl.setTextSize(2);
      
      switch(progress){
      case 0: // start of file 
        PrintToLcdWithBuf(buf, BUFSIZE, "Press START       ",decodeTime);
        break;
      case 1: // file is playing, print decode time to screen
        PrintToLcdWithBuf(buf, BUFSIZE, "Decode Time: %d   ",decodeTime);
        break;
      case 2: // file is finished playing, print message
        PrintToLcdWithBuf(buf, BUFSIZE, "File finished...");
        break;
      case 3: // file is stopped by stop button press
        PrintToLcdWithBuf(buf, BUFSIZE, "File Stopped...");
        break;
      default:
        PrintToLcdWithBuf(buf, BUFSIZE, "Non recognized command");
      }
}

// Renders a character at the current cursor position on the LCD
static void PrintCharToLcd(char c)
{
    lcdCtrl.write(c);
}

/************************************************************************************

   Print a formated string with the given buffer to LCD.
   Each task should use its own buffer to prevent data corruption.

************************************************************************************/
void PrintToLcdWithBuf(char *buf, int size, char *format, ...)
{
    va_list args;
    va_start(args, format);
    PrintToDeviceWithBuf(PrintCharToLcd, buf, size, format, args);
    va_end(args);
}
