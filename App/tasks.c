/************************************************************************************

Copyright (c) 2001-2016  University of Washington Extension.

Module Name:

tasks.c

Module Description:

The tasks that are executed by the test application.

2016/2 Nick Strathy adapted it for NUCLEO-F401RE 
2017-Feb Elizabeth Purswell filled in the TODOs

************************************************************************************/
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

Adafruit_ILI9341 lcdCtrl = Adafruit_ILI9341(); // The LCD controller

Adafruit_FT6206 touchCtrl = Adafruit_FT6206(); // The touch controller

Adafruit_GFX_Button startButtonCtrl = Adafruit_GFX_Button(); // start button on screen
Adafruit_GFX_Button stopButtonCtrl = Adafruit_GFX_Button(); // stop button on screen

OS_EVENT *mboxA; // mailbox that sends from LCD to Mp3 task
OS_EVENT *mboxB; // mailbox that sends from Mp3 task to LCD
OS_EVENT *mboxC; // mailbox from LCD to Mp3 task (NOT USED)

// Event flags for synchronizing mailbox messages
OS_FLAG_GRP *rxFlags = 0;


#define PENRADIUS 3

long MapTouchToScreen(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


#include "train_crossing.h"
#include "bach.h"

#define BUFSIZE 256

/************************************************************************************

Allocate the stacks for each task.
The maximum number of tasks the application can have is defined by 
OS_MAX_TASKS in os_cfg.h

************************************************************************************/

static OS_STK   LcdTouchTaskStk[APP_CFG_TASK_START_STK_SIZE];
static OS_STK   Mp3StreamTaskStk[APP_CFG_TASK_START_STK_SIZE];
static OS_STK  SDCardTaskStk[APP_CFG_TASK_START_STK_SIZE];


// Task prototypes
void LcdTouchTask(void* pdata);
void Mp3StreamTask(void* pdata);
void SDCardTask(void* pdata);



// Useful functions
void PrintToLcdWithBuf(char *buf, int size, char *format, ...);

// Globals
BOOLEAN nextSong = OS_FALSE;

/************************************************************************************

This task is the initial task running, started by main(). It starts
the system tick timer and creates all the other tasks. Then it deletes itself.

************************************************************************************/
void StartupTask(void* pdata)
{
  char buf[BUFSIZE];
  INT8U err;
  
  PjdfErrCode pjdfErr;
  INT32U length;
  static HANDLE hSD = 0;
  static HANDLE hSPI = 0;
  
  PrintWithBuf(buf, BUFSIZE, "StartupTask: Begin\n");
  PrintWithBuf(buf, BUFSIZE, "StartupTask: Starting timer tick\n");
  
  // Start the system tick
  OS_CPU_SysTickInit(OS_TICKS_PER_SEC);
  
  // Initialize SD card
  PrintWithBuf(buf, PRINTBUFMAX, "Opening handle to SD driver: %s\n", PJDF_DEVICE_ID_SD_ADAFRUIT);
  hSD = Open(PJDF_DEVICE_ID_SD_ADAFRUIT, 0);
  if (!PJDF_IS_VALID_HANDLE(hSD)) while(1);
  
  
  PrintWithBuf(buf, PRINTBUFMAX, "Opening SD SPI driver: %s\n", SD_SPI_DEVICE_ID);
  // We talk to the SD controller over a SPI interface therefore
  // open an instance of that SPI driver and pass the handle to 
  // the SD driver.
  hSPI = Open(SD_SPI_DEVICE_ID, 0);
  if (!PJDF_IS_VALID_HANDLE(hSPI)) while(1);
  
  length = sizeof(HANDLE);
  pjdfErr = Ioctl(hSD, PJDF_CTRL_SD_SET_SPI_HANDLE, &hSPI, &length);
  if(PJDF_IS_ERROR(pjdfErr)) while(1);
  
  
  if (!SD.begin(hSD)) {
    PrintWithBuf(buf, BUFSIZE, "Attempt to initialize SD card failed.\n");
  }
  
  // Create the test tasks
  PrintWithBuf(buf, BUFSIZE, "StartupTask: Creating the application tasks\n");
  
  mboxA = OSMboxCreate(NULL); // create empty mailbox to talk from LCD task to Mp3 task
  mboxB = OSMboxCreate(NULL); // create empty mailbox to talk from Mp3 task to LCD task
  mboxC = OSMboxCreate(NULL); // create empty mailbox to talk from LCD to Mp3 task
  
  rxFlags = OSFlagCreate(0x0, &err); // flags, starting with all bits clear
  
  // The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h
  OSTaskCreate(Mp3StreamTask, (void*)0, &Mp3StreamTaskStk[APP_CFG_TASK_START_STK_SIZE-1], APP_TASK_TEST1_PRIO); // this is the highest priority task
  OSTaskCreate(LcdTouchTask, (void*)0, &LcdTouchTaskStk[APP_CFG_TASK_START_STK_SIZE-1], APP_TASK_TEST2_PRIO);
  OSTaskCreate(SDCardTask, (void*)0, &SDCardTaskStk[APP_CFG_TASK_START_STK_SIZE-1], APP_TASK_TEST3_PRIO); // this is the lowest priority task
  
  // Delete ourselves, letting the work be done in the new tasks.
  PrintWithBuf(buf, BUFSIZE, "StartupTask: deleting self\n");
  OSTaskDel(OS_PRIO_SELF);
}


// numbers from https://learn.adafruit.com/adafruit-2-8-tft-touch-shield-v2
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

// here are defines for start/stop buttons size/location
#define START_BUTTON_X 75
#define START_BUTTON_Y 200
#define START_BUTTON_HEIGHT 75
#define START_BUTTON_WIDTH 75
#define STOP_BUTTON_X 175
#define STOP_BUTTON_Y 200
#define STOP_BUTTON_HEIGHT 75
#define STOP_BUTTON_WIDTH 75

#define BOXSIZE 15 // box height for filename box


// this will set up the initial screen of touch screen
// fill screen with background color
// writing message
// write stop/start buttons
//
static void DrawLcdContents()
{
  char buf[BUFSIZE];
  lcdCtrl.fillScreen(ILI9341_BLUE);
  
  // Print a message on the LCD
  lcdCtrl.setCursor(40, 35);
  lcdCtrl.setTextColor(ILI9341_WHITE, ILI9341_BLUE);  
  lcdCtrl.setTextSize(2);
  PrintToLcdWithBuf(buf, BUFSIZE, "MP3 Player");
  
  // start button (green)
  startButtonCtrl.initButton(&lcdCtrl, START_BUTTON_X, START_BUTTON_Y, \
    START_BUTTON_HEIGHT, START_BUTTON_WIDTH,0,ILI9341_BLACK,ILI9341_GREEN,"START",2);
  startButtonCtrl.drawButton(1);
  
  // stop button (red)
  stopButtonCtrl.initButton(&lcdCtrl, STOP_BUTTON_X, STOP_BUTTON_Y, \
    STOP_BUTTON_HEIGHT, STOP_BUTTON_WIDTH,0,ILI9341_BLACK,ILI9341_RED,"STOP",2);
  stopButtonCtrl.drawButton(1);   
}

/************************************************************************************

Runs LCD/Touch code

************************************************************************************/
void LcdTouchTask(void* pdata)
{
  PjdfErrCode pjdfErr;
  INT8U err;
  INT32U length;
  
  char buf[BUFSIZE];
  PrintWithBuf(buf, BUFSIZE, "LcdTouchTask: starting\n");
  
  PrintWithBuf(buf, BUFSIZE, "Opening LCD driver: %s\n", PJDF_DEVICE_ID_LCD_ILI9341);
  // Open handle to the LCD driver
  HANDLE hLcd = Open(PJDF_DEVICE_ID_LCD_ILI9341, 0);
  if (!PJDF_IS_VALID_HANDLE(hLcd)) while(1);
  
  PrintWithBuf(buf, BUFSIZE, "Opening LCD SPI driver: %s\n", LCD_SPI_DEVICE_ID);
  // We talk to the LCD controller over a SPI interface therefore
  // open an instance of that SPI driver and pass the handle to 
  // the LCD driver.
  HANDLE hSPI = Open(LCD_SPI_DEVICE_ID, 0);
  if (!PJDF_IS_VALID_HANDLE(hSPI)) while(1);
  
  length = sizeof(HANDLE);
  pjdfErr = Ioctl(hLcd, PJDF_CTRL_LCD_SET_SPI_HANDLE, &hSPI, &length);
  if(PJDF_IS_ERROR(pjdfErr)) while(1);
  
  PrintWithBuf(buf, BUFSIZE, "Initializing LCD controller\n");
  lcdCtrl.setPjdfHandle(hLcd);
  lcdCtrl.begin();
  
  DrawLcdContents(); // draw the initial screen 
  OSTimeDly(100); // allow SD task to get first filename on screen
  
  PrintWithBuf(buf, BUFSIZE, "Opening FT6206 driver: %s\n", PJDF_DEVICE_ID_TOUCH_FT6206);
  // Open handle to the touch driver
  HANDLE hTouch = Open(PJDF_DEVICE_ID_TOUCH_FT6206, 0);
  if (!PJDF_IS_VALID_HANDLE(hTouch)) while(1);
  touchCtrl.setPjdfHandle(hTouch);
  
  PrintWithBuf(buf, BUFSIZE, "Initializing FT6206 touchscreen controller\n");
  
  if (! touchCtrl.begin(40)) {  // pass in 'sensitivity' coefficient
    PrintWithBuf(buf, BUFSIZE, "Couldn't start FT6206 touchscreen controller\n");
    while (1);
  }
  
  //   int currentcolor = ILI9341_RED;
  int currentcolor = ILI9341_CYAN;  // color of dots 
  
  while(1)  // LCD Task loop
  {
    // update decode time on screen
    
    // wait for flag bit 1 to get set -- this means that mboxB will have an update file progress time
    OSFlagPend(rxFlags,0x2, OS_FLAG_WAIT_SET_ALL, 0, &err);
    
    // now get decode time out of mboxB
    int *pdecodeTime;
    pdecodeTime = (int *)OSMboxPend(mboxB,0,&err);
    
    if(*pdecodeTime==-1)
      DecodeTimeUpdate(0,0); // "Press START" prompt
    else
      DecodeTimeUpdate(*pdecodeTime,1);    // draw that new decode time on LCD
    
    
    // now poll for a touch on the touch screen
    boolean touched = false;
    
    OSTimeDly(30);  // need a little delay so that touchpad can notice a touch
    touched = touchCtrl.touched();
    
    TS_Point rawPoint;
    
    touchCtrl.readData((uint16_t *)&rawPoint.x,(uint16_t *)&rawPoint.y);
    
    // transform touch orientation to screen orientation.
    TS_Point p = TS_Point();
    p.x = MapTouchToScreen(rawPoint.x, 0, ILI9341_TFTWIDTH, ILI9341_TFTWIDTH, 0);
    p.y = MapTouchToScreen(rawPoint.y, 0, ILI9341_TFTHEIGHT, ILI9341_TFTHEIGHT, 0);
    
    //       lcdCtrl.fillCircle(p.x, p.y, PENRADIUS, currentcolor); // draw a little dot where I pressed
    // Note: whilst debugging, I kept this for a while just to see that a touch was being received
    
    // need to see if (x,y) is in one of the buttons
    boolean isinStartbutton = false;
    boolean startbuttonPush = false;
    boolean isinStopbutton = false;
    boolean stopbuttonPush = false;
    
    isinStartbutton = startButtonCtrl.contains(p.x,p.y);
    startButtonCtrl.press(isinStartbutton);
    startbuttonPush = startButtonCtrl.isPressed();
    isinStopbutton = stopButtonCtrl.contains(p.x,p.y);
    stopButtonCtrl.press(isinStopbutton);
    stopbuttonPush = stopButtonCtrl.isPressed();
    
    if(startbuttonPush){
      PrintWithBuf(buf,BUFSIZE,"Start button is pushed\n");
      OSFlagPost(rxFlags,0x1,OS_FLAG_SET, &err); // set flag bit 0 to begin playing
    }
    else if(stopbuttonPush){
      PrintWithBuf(buf,BUFSIZE,"Stop button is pushed\n");
      OSFlagPost(rxFlags,0x1,OS_FLAG_CLR, &err); // clear flag bit 0 to stop playing
    }
    else
    {
      PrintWithBuf(buf,BUFSIZE,"No button is pushed\n"); // don't mess with MP3 streaming 
    }
    
    // clear decode time flag now that decode time on screen is updated
    OSFlagPost(rxFlags,0x2,OS_FLAG_CLR, &err);
  }
}

/************************************************************************************

Runs SD Card Task

************************************************************************************/
void SDCardTask(void* pdata) 
{
  PjdfErrCode pjdfErr;
  INT32U length;
  INT8U err;
  char buf[BUFSIZE];
  
  File dir = SD.open("/");
  while (1) // SDCard Task Loop
  {
    File entry = dir.openNextFile();
    if (!entry)
    {
      //break;
      dir.seek(0); // reset directory file to read again;
    }
    if (entry.isDirectory())  // skip directories
    {
      entry.close();
      continue;
    }
    
    // Print filename on the LCD
    
    lcdCtrl.fillRect(0, 60, ILI9341_TFTWIDTH, BOXSIZE, ILI9341_BLUE);
    
    lcdCtrl.setCursor(40, 60);
    lcdCtrl.setTextColor(ILI9341_WHITE);  
    lcdCtrl.setTextSize(2);
    PrintToLcdWithBuf(buf, BUFSIZE, entry.name());
    
    char *msg = entry.name();
    
    if(!strlen(msg)==0){ // if filename length is non-zero, need to put it in mailbox
      
      OSMboxPost(mboxA,(void *)msg);  // post message (filename) to mailbox for Mp3 task to pick up
      
      OSFlagPost(rxFlags, 0x4, OS_FLAG_SET, &err); // I've posted filename to mailbox, now set flag bit 2
      // to say that message has been posted
      OSFlagPend(rxFlags, 0x4, OS_FLAG_WAIT_CLR_ALL, 0, &err); // wait for flag to be cleared (done with file, get next one)
    }
    entry.close();
  }
  dir.seek(0); // reset directory file to read again;
}

/************************************************************************************

Runs MP3 Streaming Task

************************************************************************************/
void Mp3StreamTask(void* pdata)
{
  PjdfErrCode pjdfErr;
  INT32U length;
  
  OSTimeDly(1000); // Allow other task to initialize LCD before we use it.
  
  char buf[BUFSIZE];
  PrintWithBuf(buf, BUFSIZE, "Mp3StreamTask: starting\n");
  
  PrintWithBuf(buf, BUFSIZE, "Opening MP3 driver: %s\n", PJDF_DEVICE_ID_MP3_VS1053);
  // Open handle to the MP3 decoder driver
  HANDLE hMp3 = Open(PJDF_DEVICE_ID_MP3_VS1053, 0);
  if (!PJDF_IS_VALID_HANDLE(hMp3)) while(1);
  
  PrintWithBuf(buf, BUFSIZE, "Opening MP3 SPI driver: %s\n", MP3_SPI_DEVICE_ID);
  // We talk to the MP3 decoder over a SPI interface therefore
  // open an instance of that SPI driver and pass the handle to 
  // the MP3 driver.
  HANDLE hSPI = Open(MP3_SPI_DEVICE_ID, 0);
  if (!PJDF_IS_VALID_HANDLE(hSPI)) while(1);
  
  length = sizeof(HANDLE);
  pjdfErr = Ioctl(hMp3, PJDF_CTRL_MP3_SET_SPI_HANDLE, &hSPI, &length);
  if(PJDF_IS_ERROR(pjdfErr)) while(1);
  
  // Send initialization data to the MP3 decoder and run a test
  PrintWithBuf(buf, BUFSIZE, "Starting MP3 device test\n");
  Mp3Init(hMp3);
  
  const char *DummyMsg = "XX";
  char *msgReceived = (char*) DummyMsg;
  
  const char *DoneMsg = "DONE";
  char *sendMsg = (char*) DoneMsg;
  int waitforstarttime = -1;
  INT8U err;
  
  OSMboxPost(mboxB,&waitforstarttime); // post -1 as decode time
  OSFlagPost(rxFlags, 0x1, OS_FLAG_CLR, &err); // flag to stop
  
  while (1) // MP3Task Loop
  {
    
    OSFlagPost(rxFlags,0x2,OS_FLAG_CLR,&err); // set flag bit 1 for new file decode time
    
    OSMboxPost(mboxB,(void *)&waitforstarttime);
    OSFlagPost(rxFlags,0x2,OS_FLAG_SET,&err); // there should be a -1 in mboxB now
    
    
    // wait for a START to be pressed 
    // note: I think this is not the most elegant way to do this, but it works
    err = OSTaskChangePrio(5,10);
    
    OS_FLAGS flagit;
    flagit = OSFlagAccept(rxFlags,0x1,OS_FLAG_WAIT_SET_ALL, &err);
    if(flagit==0){
      PrintWithBuf(buf,BUFSIZE,"Start flag not yet set\n");
      DecodeTimeUpdate(0,0);
    }
    else {
      PrintWithBuf(buf,BUFSIZE,"Start flag is now set\n");
      OSTaskChangePrio(10,5);
      DecodeTimeUpdate(0,1); // initialize decode time
      OSFlagPost(rxFlags,0x2,OS_FLAG_CLR,&err); 
      msgReceived = (char *)OSMboxPend(mboxA, 0, &err); // get filename from SD task
      
      if(strlen(msgReceived)==0)
        PrintWithBuf(buf, BUFSIZE, "Empty file\n"); // don't do any playing
      else {
        PrintWithBuf(buf, BUFSIZE, "File name: %s\n",msgReceived);
        
        Mp3StreamSDFile(hMp3, msgReceived); // play the file that mailbox sent 
      }
      
      
      OSFlagPost(rxFlags, 0x5, OS_FLAG_CLR, &err);//clear the file and playing flags now that I'm done playing file
#define FILE_FINISHED_PLAYING 0x2
      DecodeTimeUpdate(0,FILE_FINISHED_PLAYING); // print "file finished" message on LCD
      OSTimeDly(500); // pause a little bit so I can see the message
      
      
      PrintWithBuf(buf, BUFSIZE, "Done streaming sound file: %s\n", msgReceived);
      
    }
    
    
  }
}






