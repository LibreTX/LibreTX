/*
 * Authors (alphabetical order)
 * - Andre Bernet <bernet.andre@gmail.com>
 * - Andreas Weitl
 * - Bertrand Songis <bsongis@gmail.com>
 * - Bryan J. Rentoul (Gruvin) <gruvin@gmail.com>
 * - Cameron Weeks <th9xer@gmail.com>
 * - Erez Raviv
 * - Gabriel Birkus
 * - Jean-Pierre Parisy
 * - Karl Szmutny
 * - Michael Blandford
 * - Michal Hlavinka
 * - Pat Mackenzie
 * - Philip Moss
 * - Rob Thomson
 * - Romolo Manfredini <romolo.manfredini@gmail.com>
 * - Thomas Husterer
 *
 * opentx is based on code named
 * gruvin9x by Bryan J. Rentoul: http://code.google.com/p/gruvin9x/,
 * er9x by Erez Raviv: http://code.google.com/p/er9x/,
 * and the original (and ongoing) project by
 * Thomas Husterer, th9x: http://code.google.com/p/th9x/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "../../opentx.h"

void menuGeneralSdManagerInfo(uint8_t event)
{
  SIMPLE_SUBMENU(STR_SD_INFO_TITLE, 1);

  lcd_putsLeft(2*FH, STR_SD_TYPE);
  lcd_puts(10*FW, 2*FH, SD_IS_HC() ? STR_SDHC_CARD : STR_SD_CARD);

  lcd_putsLeft(3*FH, STR_SD_SIZE);
  lcd_outdezAtt(10*FW, 3*FH, SD_GET_SIZE_MB(), LEFT);
  lcd_putc(lcdLastPos, 3*FH, 'M');

  lcd_putsLeft(4*FH, STR_SD_SECTORS);
  lcd_outdezAtt(10*FW, 4*FH, SD_GET_BLOCKNR()/1000, LEFT);
  lcd_putc(lcdLastPos, 4*FH, 'k');

  lcd_putsLeft(5*FH, STR_SD_SPEED);
  lcd_outdezAtt(10*FW, 5*FH, SD_GET_SPEED()/1000, LEFT);
  lcd_puts(lcdLastPos, 5*FH, "kb/s");
}

inline bool isFilenameGreater(bool isfile, const char * fn, const char * line)
{
  return (isfile && !line[SD_SCREEN_FILE_LENGTH+1]) || (isfile==(bool)line[SD_SCREEN_FILE_LENGTH+1] && strcasecmp(fn, line) > 0);
}

inline bool isFilenameLower(bool isfile, const char * fn, const char * line)
{
  return (!isfile && line[SD_SCREEN_FILE_LENGTH+1]) || (isfile==(bool)line[SD_SCREEN_FILE_LENGTH+1] && strcasecmp(fn, line) < 0);
}

void flashBootloader(const char * filename)
{
  FIL file;
  f_open(&file, filename, FA_READ);
  uint8_t buffer[1024];
  UINT count;

  lcd_clear();
  lcd_putsLeft(4*FH, STR_WRITING);
  lcd_rect(3, 6*FH+4, 204, 7);
  lcdRefresh();

  static uint8_t unlocked = 0;
  if (!unlocked) {
    unlocked = 1;
    unlockFlash();
  }

  for (int i=0; i<BOOTLOADER_SIZE; i+=1024) {
    watchdogSetTimeout(100/*1s*/);
    if (f_read(&file, buffer, 1024, &count) != FR_OK || count != 1024) {
      POPUP_WARNING(STR_SDCARD_ERROR);
      break;
    }
    if (i==0 && !isBootloaderStart((uint32_t *)buffer)) {
      POPUP_WARNING(STR_INCOMPATIBLE);
      break;
    }
    for (int j=0; j<1024; j+=FLASH_PAGESIZE) {
      writeFlash(CONVERT_UINT_PTR(FIRMWARE_ADDRESS+i+j), (uint32_t *)(buffer+j));
      lcd_hline(5, 6*FH+6, (200*i)/BOOTLOADER_SIZE, FORCE);
      lcd_hline(5, 6*FH+7, (200*i)/BOOTLOADER_SIZE, FORCE);
      lcd_hline(5, 6*FH+8, (200*i)/BOOTLOADER_SIZE, FORCE);
      lcdRefresh();
      SIMU_SLEEP(30/*ms*/);
    }
  }

  if (unlocked) {
    lockFlash();
    unlocked = 0;
  }

  f_close(&file);
}

void onSdManagerMenu(const char *result)
{
  TCHAR lfn[_MAX_LFN+1];

  // TODO possible buffer overflows here!

  uint8_t index = m_posVert-1-s_pgOfs;
  char *line = reusableBuffer.sdmanager.lines[index];

  if (result == STR_SD_INFO) {
    pushMenu(menuGeneralSdManagerInfo);
  }
  else if (result == STR_SD_FORMAT) {
    POPUP_CONFIRMATION(STR_CONFIRM_FORMAT);
  }
  else if (result == STR_COPY_FILE) {
    clipboard.type = CLIPBOARD_TYPE_SD_FILE;
    f_getcwd(clipboard.data.sd.directory, CLIPBOARD_PATH_LEN);
    strncpy(clipboard.data.sd.filename, line, CLIPBOARD_PATH_LEN-1);
  }
  else if (result == STR_PASTE) {
    f_getcwd(lfn, _MAX_LFN);
    POPUP_WARNING(fileCopy(clipboard.data.sd.filename, clipboard.data.sd.directory, lfn));
    reusableBuffer.sdmanager.offset = -1;
  }
  else if (result == STR_DELETE_FILE) {
    f_getcwd(lfn, _MAX_LFN);
    strcat(lfn, PSTR("/"));
    strcat(lfn, line);
    f_unlink(lfn);
    strncpy(statusLineMsg, line, 13);
    strcpy_P(statusLineMsg+min((uint8_t)strlen(statusLineMsg), (uint8_t)13), STR_REMOVED);
    showStatusLine();
    reusableBuffer.sdmanager.offset = -1;
    if (m_posVert == reusableBuffer.sdmanager.count) 
      m_posVert--;
  }
  /* TODO else if (result == STR_LOAD_FILE) {
    f_getcwd(lfn, _MAX_LFN);
    strcat(lfn, "/");
    strcat(lfn, line);
    POPUP_WARNING(eeLoadModelSD(lfn));
  } */
  else if (result == STR_PLAY_FILE) {
    f_getcwd(lfn, _MAX_LFN);
    strcat(lfn, "/");
    strcat(lfn, line);
    audioQueue.stopAll();
    audioQueue.playFile(lfn, 0, ID_PLAY_FROM_SD_MANAGER);
  }
  else if (result == STR_ASSIGN_BITMAP) {
    strAppendFilename(g_model.header.bitmap, line, sizeof(g_model.header.bitmap));
    LOAD_MODEL_BITMAP();
    memcpy(modelHeaders[g_eeGeneral.currModel].bitmap, g_model.header.bitmap, sizeof(g_model.header.bitmap));
    eeDirty(EE_MODEL);
  }
  else if (result == STR_VIEW_TEXT) {
    f_getcwd(lfn, _MAX_LFN);
    strcat(lfn, "/");
    strcat(lfn, line);
    pushMenuTextView(lfn);
  }
  else if (result == STR_FLASH_BOOTLOADER) {
    f_getcwd(lfn, _MAX_LFN);
    strcat(lfn, "/");
    strcat(lfn, line);
    flashBootloader(lfn);
  }
#if defined(LUA)
  else if (result == STR_EXECUTE_FILE) {
    f_getcwd(lfn, _MAX_LFN);
    strcat(lfn, "/");
    strcat(lfn, line);
    luaExec(lfn);
  }
#endif
}

void menuGeneralSdManager(uint8_t _event)
{
  FILINFO fno;
  DIR dir;
  char *fn;   /* This function is assuming non-Unicode cfg. */
  TCHAR lfn[_MAX_LFN + 1];
  fno.lfname = lfn;
  fno.lfsize = sizeof(lfn);

  if (s_warning_result) {
    s_warning_result = 0;
    displayPopup(STR_FORMATTING);
    closeLogs();
    audioQueue.stopSD();
    if (f_mkfs(0, 1, 0) == FR_OK) {
      f_chdir("/");
      reusableBuffer.sdmanager.offset = -1;
    }
    else {
      POPUP_WARNING(STR_SDCARD_ERROR);
    }
  }

  uint8_t event = ((READ_ONLY() && EVT_KEY_MASK(_event) == KEY_ENTER) ? 0 : _event);
  SIMPLE_MENU(SD_IS_HC() ? STR_SDHC_CARD : STR_SD_CARD, menuTabGeneral, e_Sd, 1+reusableBuffer.sdmanager.count);

  if (s_editMode > 0)
    s_editMode = 0;

  switch(_event) {
    case EVT_ENTRY:
      f_chdir(ROOT_PATH);
      reusableBuffer.sdmanager.offset = 65535;
      break;

    case EVT_KEY_LONG(KEY_MENU):
      if (!READ_ONLY()) {
        killEvents(_event);
        // MENU_ADD_ITEM(STR_SD_INFO);  TODO: Implement
        MENU_ADD_ITEM(STR_SD_FORMAT);
        menuHandler = onSdManagerMenu;
      }
      break;

    case EVT_KEY_BREAK(KEY_ENTER):
    {
      if (m_posVert > 0) {
        vertpos_t index = m_posVert-1-s_pgOfs;
        if (!reusableBuffer.sdmanager.lines[index][SD_SCREEN_FILE_LENGTH+1]) {
          f_chdir(reusableBuffer.sdmanager.lines[index]);
          s_pgOfs = 0;
          m_posVert = 1;
          reusableBuffer.sdmanager.offset = 65535;
          killEvents(_event);
          break;
        }
      }
      if (!IS_ROTARY_BREAK(_event) || m_posVert==0)
        break;
      // no break;
    }

    case EVT_KEY_LONG(KEY_ENTER):
      killEvents(_event);
      {
        uint8_t index = m_posVert-1-s_pgOfs;
        // TODO duplicated code for finding extension
        char *line = reusableBuffer.sdmanager.lines[index];
        int len = strlen(line) - 4;
        char *ext = line+len;
        /* TODO if (!strcasecmp(ext, MODELS_EXT)) {
          s_menu[s_menu_count++] = STR_LOAD_FILE;
        }
        else */ if (!strcasecmp(ext, SOUNDS_EXT)) {
          MENU_ADD_ITEM(STR_PLAY_FILE);
        }
        else if (!strcasecmp(ext, BITMAPS_EXT) && !READ_ONLY() && len <= (int)sizeof(g_model.header.bitmap)) {
          MENU_ADD_ITEM(STR_ASSIGN_BITMAP);
        }
        else if (!strcasecmp(ext, TEXT_EXT)) {
          MENU_ADD_ITEM(STR_VIEW_TEXT);
        }
        else if (!strcasecmp(ext, FIRMWARE_EXT) && !READ_ONLY()) {
          MENU_ADD_ITEM(STR_FLASH_BOOTLOADER);
        }
#if defined(LUA)
        else if (!strcasecmp(ext, SCRIPTS_EXT)) {
          MENU_ADD_ITEM(STR_EXECUTE_FILE);
        }
#endif
        if (!READ_ONLY()) {
          if (line[SD_SCREEN_FILE_LENGTH+1]) // it's a file
            MENU_ADD_ITEM(STR_COPY_FILE);
          if (clipboard.type == CLIPBOARD_TYPE_SD_FILE)
            MENU_ADD_ITEM(STR_PASTE);
          // MENU_ADD_ITEM(STR_RENAME_FILE);  TODO: Implement
          MENU_ADD_ITEM(STR_DELETE_FILE);
        }
      }
      menuHandler = onSdManagerMenu;
      break;
  }

  if (reusableBuffer.sdmanager.offset != s_pgOfs) {
    if (s_pgOfs == 0) {
      reusableBuffer.sdmanager.offset = 0;
      memset(reusableBuffer.sdmanager.lines, 0, sizeof(reusableBuffer.sdmanager.lines));
    }
    else if (s_pgOfs == reusableBuffer.sdmanager.count-7) {
      reusableBuffer.sdmanager.offset = s_pgOfs;
      memset(reusableBuffer.sdmanager.lines, 0, sizeof(reusableBuffer.sdmanager.lines));
    }
    else if (s_pgOfs > reusableBuffer.sdmanager.offset) {
      memmove(reusableBuffer.sdmanager.lines[0], reusableBuffer.sdmanager.lines[1], 6*sizeof(reusableBuffer.sdmanager.lines[0]));
      memset(reusableBuffer.sdmanager.lines[6], 0xff, SD_SCREEN_FILE_LENGTH);
      reusableBuffer.sdmanager.lines[6][SD_SCREEN_FILE_LENGTH+1] = 1;
    }
    else {
      memmove(reusableBuffer.sdmanager.lines[1], reusableBuffer.sdmanager.lines[0], 6*sizeof(reusableBuffer.sdmanager.lines[0]));
      memset(reusableBuffer.sdmanager.lines[0], 0, sizeof(reusableBuffer.sdmanager.lines[0]));
    }

    reusableBuffer.sdmanager.count = 0;

    FRESULT res = f_opendir(&dir, ".");        /* Open the directory */
    if (res == FR_OK) {
      for (;;) {
        res = f_readdir(&dir, &fno);                   /* Read a directory item */
        if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
        if (fno.fname[0] == '.' && fno.fname[1] == '\0') continue;             /* Ignore dot entry */
#if _USE_LFN
        fn = *fno.lfname ? fno.lfname : fno.fname;
#else
        fn = fno.fname;
#endif
        if (strlen(fn) > SD_SCREEN_FILE_LENGTH) continue;

        reusableBuffer.sdmanager.count++;

        bool isfile = !(fno.fattrib & AM_DIR);

        if (s_pgOfs == 0) {
          for (uint8_t i=0; i<LCD_LINES-1; i++) {
            char *line = reusableBuffer.sdmanager.lines[i];
            if (line[0] == '\0' || isFilenameLower(isfile, fn, line)) {
              if (i < 6) memmove(reusableBuffer.sdmanager.lines[i+1], line, sizeof(reusableBuffer.sdmanager.lines[i]) * (6-i));
              memset(line, 0, sizeof(reusableBuffer.sdmanager.lines[i]));
              strcpy(line, fn);
              line[SD_SCREEN_FILE_LENGTH+1] = isfile;
              break;
            }
          }
        }
        else if (reusableBuffer.sdmanager.offset == s_pgOfs) {
          for (int8_t i=6; i>=0; i--) {
            char *line = reusableBuffer.sdmanager.lines[i];
            if (line[0] == '\0' || isFilenameGreater(isfile, fn, line)) {
              if (i > 0) memmove(reusableBuffer.sdmanager.lines[0], reusableBuffer.sdmanager.lines[1], sizeof(reusableBuffer.sdmanager.lines[0]) * i);
              memset(line, 0, sizeof(reusableBuffer.sdmanager.lines[i]));
              strcpy(line, fn);
              line[SD_SCREEN_FILE_LENGTH+1] = isfile;
              break;
            }
          }
        }
        else if (s_pgOfs > reusableBuffer.sdmanager.offset) {
          if (isFilenameGreater(isfile, fn, reusableBuffer.sdmanager.lines[5]) && isFilenameLower(isfile, fn, reusableBuffer.sdmanager.lines[6])) {
            memset(reusableBuffer.sdmanager.lines[6], 0, sizeof(reusableBuffer.sdmanager.lines[0]));
            strcpy(reusableBuffer.sdmanager.lines[6], fn);
            reusableBuffer.sdmanager.lines[6][SD_SCREEN_FILE_LENGTH+1] = isfile;
          }
        }
        else {
          if (isFilenameLower(isfile, fn, reusableBuffer.sdmanager.lines[1]) && isFilenameGreater(isfile, fn, reusableBuffer.sdmanager.lines[0])) {
            memset(reusableBuffer.sdmanager.lines[0], 0, sizeof(reusableBuffer.sdmanager.lines[0]));
            strcpy(reusableBuffer.sdmanager.lines[0], fn);
            reusableBuffer.sdmanager.lines[0][SD_SCREEN_FILE_LENGTH+1] = isfile;
          }
        }
      }
    }
  }

  reusableBuffer.sdmanager.offset = s_pgOfs;

  for (uint8_t i=0; i<LCD_LINES-1; i++) {
    coord_t y = MENU_TITLE_HEIGHT + 1 + i*FH;
    lcdNextPos = 0;
    uint8_t attr = (m_posVert-1-s_pgOfs == i ? BSS|INVERS : BSS);
    if (reusableBuffer.sdmanager.lines[i][0]) {
      if (!reusableBuffer.sdmanager.lines[i][SD_SCREEN_FILE_LENGTH+1]) { lcd_putcAtt(0, y, '[', attr); }
      lcd_putsAtt(lcdNextPos, y, reusableBuffer.sdmanager.lines[i], attr);
      if (!reusableBuffer.sdmanager.lines[i][SD_SCREEN_FILE_LENGTH+1]) { lcd_putcAtt(lcdNextPos, y, ']', attr); }
    }
  }

  static vertpos_t sdBitmapIdx = 0xFFFF;
  static uint8_t sdBitmap[MODEL_BITMAP_SIZE];
  vertpos_t index = m_posVert-1-s_pgOfs;
  if (m_posVert > 0) {
    char * ext = reusableBuffer.sdmanager.lines[index];
    ext += strlen(ext) - 4;
    if (!strcasecmp(ext, BITMAPS_EXT)) {
      if (sdBitmapIdx != m_posVert) {
        sdBitmapIdx = m_posVert;
        uint8_t *dest = sdBitmap;
        if (bmpLoad(dest, reusableBuffer.sdmanager.lines[index], MODEL_BITMAP_WIDTH, MODEL_BITMAP_HEIGHT)) {
          memcpy(sdBitmap, logo_taranis, MODEL_BITMAP_SIZE);
        }
      }
      lcd_bmp(22*FW+2, 2*FH+FH/2, sdBitmap);
    }
  }
}
