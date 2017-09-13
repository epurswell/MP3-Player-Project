    // Code snippet to cycle forever through the root directory of the SD card and play every file found.
    
    File dir = SD.open("/");
    while (1)
    {
        while (1)
        {
            File entry = dir.openNextFile();
            if (!entry)
            {
                break;
            }
            if (entry.isDirectory())  // skip directories
            {
                entry.close();
                continue;
            }
                
            // Print filename on the LCD
            OSTimeDly(100);     // One reason for the delay here is 
                                // to allow the screen to be repainted by lower pri task.
                                // A semaphore might be a better way of controlling lcd access.
            lcdCtrl.fillRect(0, 60, ILI9341_TFTWIDTH, BOXSIZE, ILI9341_BLACK);
            
            lcdCtrl.setCursor(40, 60);
            lcdCtrl.setTextColor(ILI9341_WHITE);  
            lcdCtrl.setTextSize(2);
            PrintToLcdWithBuf(buf, BUFSIZE, entry.name());
            
            Mp3StreamSDFile(hMp3, entry.name()); 
            
            entry.close();
        }
        dir.seek(0); // reset directory file to read again;
    }
