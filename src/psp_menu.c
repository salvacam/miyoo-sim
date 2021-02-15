/*
 *  Copyright (C) 2006 Ludovic Jacomme (ludovic.jacomme@gmail.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include "simcoupec.h"
#include "global.h"
#include "psp_sdl.h"
#include "psp_kbd.h"
#include "psp_menu.h"
#include "psp_fmgr.h"
#include "psp_menu_kbd.h"
#include "psp_run.h"
#include "psp_menu_set.h"
#include "psp_menu_help.h"
#include "gp2x_psp.h"

extern SDL_Surface *back_surface;

# define MENU_SCREENSHOT   0
# define MENU_VOLUME       1

# define MENU_HELP         2

# define MENU_LOAD_DISK0   3
# define MENU_LOAD_DISK1   4

# define MENU_KEYBOARD     5
# define MENU_SETTINGS     6

# define MENU_RESET        7
# define MENU_BACK         8
# define MENU_EXIT         9

# define MAX_MENU_ITEM (MENU_EXIT + 1)

  static menu_item_t menu_list[] =
  {
    "Save Screenshot :",
    "Volume          :",

    "Help",

    "Load Disk 0",     
    "Load Disk 1",     
    "Keyboard", 
    "Settings",

    "Reset Sam",
    "Back to Sam",
    "Exit"
  };

  static int cur_menu_id = MENU_LOAD_DISK0;

void 
psp_menu_display_save_name()
{
  char buffer[128];
  int Length;

  snprintf(buffer, 30, "Game: %s", sim_get_save_name());
  Length = strlen(buffer);
  psp_sdl_back2_print(300 - (6*Length), 5, buffer, PSP_MENU_TEXT2_COLOR);
}


void
string_fill_with_space(char *buffer, int size)
{
  int length = strlen(buffer);
  int index;

  for (index = length; index < size; index++) {
    buffer[index] = ' ';
  }
  buffer[size] = 0;
}

static void 
psp_display_screen_menu(void)
{
  char buffer[64];
  int menu_id = 0;
# if 0 
  int slot_id = 0;
# endif
  int color   = 0;
  int x       = 0;
  int y       = 0;
  int y_step  = 0;

  psp_sdl_blit_background();
  
  x      = 10;
  y      =  5;
  y_step = 10;
  
  for (menu_id = 0; menu_id < MAX_MENU_ITEM; menu_id++) {
    color = PSP_MENU_TEXT_COLOR;
    if (cur_menu_id == menu_id) color = PSP_MENU_SEL_COLOR;
    else 
    if (menu_id == MENU_EXIT) color = PSP_MENU_WARNING_COLOR;
    else
    if (menu_id == MENU_HELP) color = PSP_MENU_GREEN_COLOR;

    psp_sdl_back2_print(x, y, menu_list[menu_id].title, color);

    if (menu_id == MENU_SCREENSHOT) {
      sprintf(buffer,"%d", sim_get_psp_screenshot_id());
      string_fill_with_space(buffer, 4);
      psp_sdl_back2_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_VOLUME) {
      sprintf(buffer,"%d", gp2xGetSoundVolume());
      string_fill_with_space(buffer, 4);
      psp_sdl_back2_print(120, y, buffer, color);
      y += y_step;
    } else
    if (menu_id == MENU_LOAD_DISK1) {
      y += y_step;
    } else
    if (menu_id == MENU_SETTINGS) {
      y += y_step;
    } else
    if (menu_id == MENU_BACK) {
      y += y_step;
    } else
    if (menu_id == MENU_HELP) {
      y += y_step;
    }

    y += y_step;
  }

  psp_menu_display_save_name();
}

static void
psp_main_menu_reset(void)
{
  /* Reset ! */
  psp_display_screen_menu();
  psp_sdl_back2_print(180,  70, "Reset Sam !", 
                     PSP_MENU_WARNING_COLOR);
  psp_sdl_flip();
  sim_emulator_reset();
  sim_reset_save_name();
  sleep(1);
}

static int
psp_main_menu_load(int format, int disk_id)
{
  int ret;

  ret = psp_fmgr_menu(format, disk_id);
  if (ret ==  1) /* load OK */
  {
    psp_display_screen_menu();
    psp_sdl_back2_print(180, 70, "File loaded !", 
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
    return 1;
  }
  else 
  if (ret == -1) /* Load Error */
  {
    psp_display_screen_menu();
    psp_sdl_back2_print(180,  70, "Can't load file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  return 0;
}


static void
psp_main_menu_screenshot(void)
{
  psp_screenshot_mode = 10;
}

static void
psp_main_menu_volume(int step)
{
  if (step == 1) {
    gp2xIncreaseVolume();
  } else if (step == -1) {
    gp2xDecreaseVolume();
  }
}

int
psp_main_menu_exit(void)
{
  gp2xCtrlData c;

  psp_display_screen_menu();
  #ifdef MIYOO_MODE
  psp_sdl_back2_print(180,  70, "press B to confirm !", PSP_MENU_WARNING_COLOR);
  #else
  psp_sdl_back2_print(180,  70, "press X to confirm !", PSP_MENU_WARNING_COLOR);
  #endif
  psp_sdl_flip();

  psp_kbd_wait_no_button();

  do
  {
    gp2xCtrlReadBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;

    if (c.Buttons & GP2X_CTRL_CROSS) {
      psp_sdl_clear_screen(0);
      psp_sdl_flip();
      psp_sdl_clear_screen(0);
      psp_sdl_flip();
      psp_sdl_exit(0);
    }

  } while (c.Buttons == 0);

  psp_kbd_wait_no_button();

  return 0;
}

int 
psp_main_menu(void)
{
  gp2xCtrlData c;
  long        new_pad;
  long        old_pad;
  int         last_time;
  int         end_menu;

  sim_audio_pause();

  psp_kbd_wait_no_button();

  old_pad   = 0;
  last_time = 0;
  end_menu  = 0;


  while (! end_menu)
  {
    psp_display_screen_menu();
    psp_sdl_flip();

    while (1)
    {
      gp2xCtrlReadBufferPositive(&c, 1);
      c.Buttons &= PSP_ALL_BUTTON_MASK;

      if (c.Buttons) break;
    }

    new_pad = c.Buttons;

    if ((old_pad != new_pad) || ((c.TimeStamp - last_time) > PSP_MENU_MIN_TIME)) {
      last_time = c.TimeStamp;
      old_pad = new_pad;

    } else continue;

    if ((c.Buttons & GP2X_CTRL_RTRIGGER) == GP2X_CTRL_RTRIGGER) {
      psp_main_menu_reset();
      end_menu = 1;
    } else
    if ((new_pad == GP2X_CTRL_LEFT ) || 
        (new_pad == GP2X_CTRL_RIGHT) ||
        (new_pad == GP2X_CTRL_CROSS) || 
        (new_pad == GP2X_CTRL_CIRCLE))
    {
      int step = 0;

      if (new_pad & GP2X_CTRL_RIGHT) {
        step = 1;
      } else
      if (new_pad & GP2X_CTRL_LEFT) {
        step = -1;
      }

      switch (cur_menu_id ) 
      {
        case MENU_LOAD_DISK0 : if (psp_main_menu_load(FMGR_FORMAT_DISK,0)) end_menu = 1;
                               old_pad = new_pad = 0;
        break;              

        case MENU_LOAD_DISK1 : if (psp_main_menu_load(FMGR_FORMAT_DISK,1)) end_menu = 1;
                               old_pad = new_pad = 0;
        break;

        case MENU_KEYBOARD   : psp_keyboard_menu();
                               old_pad = new_pad = 0;
        break;
        case MENU_SETTINGS   : psp_settings_menu();
                               old_pad = new_pad = 0;
        break;

        case MENU_SCREENSHOT : psp_main_menu_screenshot();
                               end_menu = 1;
        break;              
        case MENU_VOLUME     : psp_main_menu_volume(step);
                               old_pad = new_pad = 0;
        break;              

        case MENU_RESET     : psp_main_menu_reset();
                              end_menu = 1;
        break;

        case MENU_BACK      : end_menu = 1;
        break;

        case MENU_EXIT      : psp_main_menu_exit();
        break;

        case MENU_HELP      : psp_help_menu();
                              old_pad = new_pad = 0;
        break;              
      }

    } else
    if(new_pad & GP2X_CTRL_UP) {

      if (cur_menu_id > 0) cur_menu_id--;
      else                 cur_menu_id = MAX_MENU_ITEM-1;

    } else
    if(new_pad & GP2X_CTRL_DOWN) {

      if (cur_menu_id < (MAX_MENU_ITEM-1)) cur_menu_id++;
      else                                 cur_menu_id = 0;

    } else  
    if(new_pad & GP2X_CTRL_SQUARE) {
      /* Cancel */
      end_menu = -1;
    } else 
    if(new_pad & GP2X_CTRL_SELECT) {
      /* Back to SIM */
      end_menu = 1;
    }
  }
 
  psp_kbd_wait_no_button();

  psp_sdl_clear_screen( PSP_MENU_BLACK_COLOR );
  psp_sdl_flip();
  psp_sdl_clear_screen( PSP_MENU_BLACK_COLOR );
  psp_sdl_flip();

  sim_audio_resume();

  return 1;
}

