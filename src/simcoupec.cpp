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

#include "SimCoupe.h"
#include "Main.h"
#include "IO.h"
#include "CPU.h"
#include "Options.h"
#include "psp_kbd.h"
#include "psp_sdl.h"

extern "C" {

extern "C" int psp_fmgr_getExtId(const char *szFilePath) ;
extern "C" int sim_update_save_name(char *name);
extern "C" int psp_kbd_load_mapping(char *name);
extern "C" int psp_kbd_save_mapping(char *name);
extern "C" void sim_kbd_load(void);
extern "C" int sim_load_settings(void);

extern "C" CDiskDevice *pDrive1;
extern "C" CDiskDevice *pDrive2;

 int psp_screenshot_mode = 0;

//Get functions
const char *
sim_get_save_name()
{
  return GetOption(save_name);
}

const char *
sim_get_home_dir()
{
  return GetOption(home_dir);
}

int
sim_kbd_save(void)
{
  char TmpDirName[MAX_PATH];
  char TmpFileName[MAX_PATH];
  getcwd(TmpDirName, MAX_PATH);
  snprintf(TmpFileName, MAX_PATH, "%s/kbd/%s.kbd", TmpDirName, sim_get_save_name());
  return( psp_kbd_save_mapping(TmpFileName) );
}




void
sim_kbd_load(void)
{
  char        TmpDirName[MAX_PATH];
  char        TmpFileName[MAX_PATH];
  struct stat aStat;

  getcwd(TmpDirName, MAX_PATH);
  snprintf(TmpFileName, MAX_PATH, "%s/kbd/%s.kbd", TmpDirName, sim_get_save_name());
  if (! stat(TmpFileName, &aStat)) {
    psp_kbd_load_mapping(TmpFileName);
  }
}

int 
sim_disk_load(char *filename, int disk_id, int zip_mode)
{
  char  SaveName[MAX_PATH];
  char *scan;

  bool fInserted = ((disk_id == 0) ? pDrive1 : pDrive2)->Insert( filename );

  if (fInserted) {

    strncpy(SaveName, filename, MAX_PATH);
    scan = strrchr(SaveName,'.');
    if (scan) *scan = '\0';
    sim_update_save_name(SaveName);

    sim_kbd_load();
    sim_load_settings();
  }

  return ! fInserted;
}

void 
sim_key_down(int sam_key)
{
  PressSamKey(sam_key);
}

void 
sim_key_up(int sam_key)
{
  ReleaseSamKey(sam_key);
}

void 
sim_release_all_key()
{
  ReleaseAllSamKeys();
}

void
sim_emulator_reset()
{
  CPU::Reset(true);
  CPU::Reset(false);
}

int   
sim_get_snd_enabled()
{
  return GetOption(snd_enable);
}

int
sim_get_render_mode()
{
  return GetOption(render_mode);
}

int
sim_get_view_fps()
{
  return GetOption(view_fps);
}

int
sim_get_speed_limiter()
{
  return GetOption(speed_limiter);
}

int
sim_get_frame_skip()
{
  return GetOption(frameskip);
}

int
sim_get_psp_cpu_clock()
{
  return GetOption(psp_cpu_clock);
}

int
sim_get_psp_joystick_cursor()
{
  return GetOption(psp_joystick_cursor);
}

int
sim_get_psp_screenshot_id()
{
  return GetOption(psp_screenshot_id);
}

//Set functions

void
sim_set_snd_enabled(int value)
{
  SetOption(snd_enable, (value != 0));
}

void
sim_set_render_mode(int value)
{
  SetOption(render_mode, value);
}

void
sim_set_view_fps(int value)
{
  SetOption(view_fps, value);
}

void
sim_set_speed_limiter(int value)
{
  SetOption(speed_limiter, value);
}

void
sim_set_frame_skip(int value)
{
  SetOption(frameskip, value);
}

void
sim_set_psp_cpu_clock(int value) 
{
  SetOption(psp_cpu_clock, value);
}

void
sim_set_psp_joystick_cursor(int value)
{
  SetOption(psp_joystick_cursor, (value != 0));
}

void
sim_set_psp_screenshot_id(int value)
{
  SetOption(psp_screenshot_id, value);
}

# if 0 
int
sim_state_load(char *FileName) 
{
  char *scan;
  char  SaveName[MAX_PATH+1];
  char  TmpFileName[MAX_PATH + 1];
  int   error;

  error = 1;

  strncpy(SaveName,FileName,MAX_PATH);
  scan = strrchr(SaveName,'.');
  if (scan) *scan = '\0';
  sim_update_save_name(SaveName);
  error = Main::LoadStateFile(FileName);

  if (! error ) {
    sim_kbd_load();
    sim_load_settings();
  }

  return error;
}
# endif

void
sim_default_settings(void)
{
  sim_set_snd_enabled(1);
  sim_set_render_mode(0);
  sim_set_view_fps(0);
  sim_set_speed_limiter(0);
  sim_set_psp_joystick_cursor(0);
  sim_set_psp_cpu_clock(GP2X_EMU_CLOCK);
  sim_set_frame_skip(1);
  sim_set_psp_screenshot_id(0);

  gp2xPowerSetClockFrequency(GP2X_EMU_CLOCK);
}

int
loc_sim_save_settings(char *chFileName)
{
  FILE* FileDesc;
  int   error = 0;

  FileDesc = fopen(chFileName, "w");
  if (FileDesc != (FILE *)0 ) {

    fprintf(FileDesc, "psp_cpu_clock=%d\n"      , sim_get_psp_cpu_clock());
    fprintf(FileDesc, "psp_joystick_cursor=%d\n" , sim_get_psp_joystick_cursor());
    fprintf(FileDesc, "render_mode=%d\n"        , sim_get_render_mode());
    fprintf(FileDesc, "view_fps=%d\n"           , sim_get_view_fps());
    fprintf(FileDesc, "speed_limiter=%d\n"      , sim_get_speed_limiter());
    fprintf(FileDesc, "frame_skip=%d\n"         , sim_get_frame_skip());

    fclose(FileDesc);

  } else {
    error = 1;
  }

  return error;
}

int
sim_save_settings(void)
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/set/%s.set", sim_get_home_dir(), sim_get_save_name());
  error = loc_sim_save_settings(FileName);

  return error;
}

static int
loc_sim_load_settings(char *chFileName)
{
  char  Buffer[512];
  char *Scan;
  unsigned int Value;
  FILE* FileDesc;

  FileDesc = fopen(chFileName, "r");
  if (FileDesc == (FILE *)0 ) return 0;

  while (fgets(Buffer,512, FileDesc) != (char *)0) {

    Scan = strchr(Buffer,'\n');
    if (Scan) *Scan = '\0';
    /* For this #@$% of windows ! */
    Scan = strchr(Buffer,'\r');
    if (Scan) *Scan = '\0';
    if (Buffer[0] == '#') continue;

    Scan = strchr(Buffer,'=');
    if (! Scan) continue;

    *Scan = '\0';
    Value = atoi(Scan+1);

    if (!strcasecmp(Buffer,"psp_cpu_clock")) sim_set_psp_cpu_clock(Value);
    else
    if (!strcasecmp(Buffer,"psp_joystick_cursor")) sim_set_psp_joystick_cursor(Value);
    else
    if (!strcasecmp(Buffer,"render_mode")) sim_set_render_mode(Value);
    else
    if (!strcasecmp(Buffer,"view_fps")) sim_set_view_fps(Value);
    else
    if (!strcasecmp(Buffer,"speed_limiter")) sim_set_speed_limiter(Value);
    else
    if (!strcasecmp(Buffer,"frame_skip")) sim_set_frame_skip(Value);
  }

  fclose(FileDesc);

  int psp_cpu_clock = sim_get_psp_cpu_clock();
  gp2xPowerSetClockFrequency(psp_cpu_clock);

  return 0;
}

int
sim_load_settings()
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/set/%s.set", sim_get_home_dir(), sim_get_save_name());
  error = loc_sim_load_settings(FileName);

  return error;
}

int
sim_load_file_settings(char *FileName)
{
  return loc_sim_load_settings(FileName);
}

int
sim_update_save_name(char *Name)
{
  char        TmpFileName[MAX_PATH];
  struct stat aStat;
  int         index;
  char       *SaveName;
  char       *Scan1;
  char       *Scan2;

  SaveName = strrchr(Name,'/');
  if (SaveName != (char *)0) SaveName++;
  else                       SaveName = Name;

  if (!strncasecmp(SaveName, "sav_", 4)) {
    Scan1 = SaveName + 4;
    Scan2 = strrchr(Scan1, '_');
    if (Scan2 && (Scan2[1] >= '0') && (Scan2[1] <= '5')) {
      strncpy(Options::s_Options.save_name, Scan1, MAX_PATH);
      Options::s_Options.save_name[Scan2 - Scan1] = '\0';
    } else {
      strncpy(Options::s_Options.save_name, SaveName, MAX_PATH);
    }
  } else {
    strncpy(Options::s_Options.save_name, SaveName, MAX_PATH);
  }

  memset(Options::s_Options.save_used,0,sizeof(Options::s_Options.save_used));

  if (Options::s_Options.save_name[0] == '\0') {
    strcpy(Options::s_Options.save_name,"default");
  }

  for (index = 0; index < SIM_MAX_SAVE_STATE; index++) {
    snprintf(TmpFileName, MAX_PATH, "%s/save/sav_%s_%d.scs", sim_get_home_dir(), sim_get_save_name(), index);
    if (! stat(TmpFileName, &aStat)) {
      Options::s_Options.save_used[index] = 1;
    }
  }
  return 0;
}

void
sim_reset_save_name()
{
  if (pDrive1->IsInserted()) pDrive1->Eject();
  if (pDrive2->IsInserted()) pDrive2->Eject();

  sim_update_save_name("");
}

void 
sim_audio_pause(void)
{
  SDL_PauseAudio(1);
}

void 
sim_audio_resume(void)
{
  if (sim_get_snd_enabled()) 
  {
    SDL_PauseAudio(0);
  }
}

int
sim_is_save_used(int slot_id)
{
  return Options::s_Options.save_used[slot_id];
}

void
sim_treat_command_key(int sim_idx)
{
  switch (sim_idx) 
  {
    case SIM_C_FPS: Options::s_Options.view_fps = ! Options::s_Options.view_fps;
    break;
    case SIM_C_JOY: Options::s_Options.psp_joystick_cursor = ! Options::s_Options.psp_joystick_cursor;
    break;
  }
}

} /* extern "C" */
