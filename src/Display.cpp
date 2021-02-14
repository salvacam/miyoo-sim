// Part of SimCoupe - A SAM Coupe emulator
//
// Display.cpp: SDL display rendering
//
//  Copyright (c) 1999-2006  Simon Owen
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

// ToDo:
//  - change to handle multiple dirty regions

#include "SimCoupe.h"
#include "simcoupec.h"

#include "Display.h"
#include "Frame.h"
#include "GUI.h"
#include "Options.h"
#include "Profile.h"
#include "Video.h"
#include "UI.h"

#include "psp_kbd.h"
#include "psp_sdl.h"
#include "psp_danzeff.h"

bool* Display::pafDirty;
SDL_Rect rSource, rTarget;

////////////////////////////////////////////////////////////////////////////////

// Writing to the display in DWORDs makes it endian sensitive, so we need to cover both cases
#if SDL_BYTEORDER == SDL_LIL_ENDIAN

inline DWORD PaletteDWORD (BYTE b1_, BYTE b2_, BYTE b3_, BYTE b4_, DWORD* pulPalette_)
    { return (pulPalette_[b4_] << 24) | (pulPalette_[b3_] << 16) | (pulPalette_[b2_] << 8) | pulPalette_[b1_]; }
inline DWORD PaletteDWORD (BYTE b1_, BYTE b2_, DWORD* pulPalette_)
    { return (pulPalette_[b2_] << 16) | pulPalette_[b1_]; }
inline DWORD MakeDWORD (BYTE b1_, BYTE b2_, BYTE b3_, BYTE b4_)
    { return (b4_ << 24) | (b3_ << 16) | (b2_ << 8) | b1_; }
inline DWORD MakeDWORD (BYTE b1_, BYTE b2_)
    { return ((b2_ << 16) | b1_) * 0x0101UL; }
inline DWORD JoinWORDs (DWORD w1_, DWORD w2_)
    { return ((w2_ << 16) | w1_); }
#else

inline DWORD PaletteDWORD (BYTE b1_, BYTE b2_, BYTE b3_, BYTE b4_, DWORD* pulPalette_)
    { return (pulPalette_[b1_] << 24) | (pulPalette_[b2_] << 16) | (pulPalette_[b3_] << 8) | pulPalette_[b4_]; }
inline DWORD PaletteDWORD (BYTE b1_, BYTE b2_, DWORD* pulPalette_)
    { return (pulPalette_[b1_] << 16) | pulPalette_[b2_]; }
inline DWORD MakeDWORD (BYTE b1_, BYTE b2_, BYTE b3_, BYTE b4_)
    { return (b1_ << 24) | (b2_ << 16) | (b3_ << 8) | b4_; }
inline DWORD MakeDWORD (BYTE b1_, BYTE b2_)
    { return ((b1_ << 16) | b2_) * 0x0101UL; }
inline DWORD JoinWORDs (DWORD w1_, DWORD w2_)
    { return ((w1_ << 16) | w2_); }
#endif

////////////////////////////////////////////////////////////////////////////////

bool Display::Init (bool fFirstInit_/*=false*/)
{
    Exit(true);

    pafDirty = new bool[Frame::GetHeight()];

    // These will be updated to the appropriate values on the first draw
    rSource.w = rTarget.w = Frame::GetWidth();
    rSource.h = rTarget.h = Frame::GetHeight() << 1;

    return Video::Init(fFirstInit_);
}

void Display::Exit (bool fReInit_/*=false*/)
{
    Video::Exit(fReInit_);

    if (pafDirty) { delete[] pafDirty; pafDirty = NULL; }
}

void Display::SetDirty ()
{
    // Mark all display lines dirty
    for (int i = 0, nHeight = Frame::GetHeight() ; i < nHeight ; i++)
        pafDirty[i] = true;
}

static void
loc_Draw_Normal(CScreen* pScreen_)
{
  SDL_Surface* pSurface_ = back_surface;
  short *pdsStart = (short *)pSurface_->pixels;

  long lPitchDW = pSurface_->pitch >> 1;
  bool *pfHiRes = pScreen_->GetHiRes();

  BYTE *pbSAM = pScreen_->GetLine(0);
  BYTE *pb    = pbSAM;
  long lPitch = pScreen_->GetPitch();

  int nShift  = 1;
  int nBottom = pScreen_->GetHeight() >> nShift;
  int nWidth  = pScreen_->GetPitch(), nRightHi = nWidth, nRightLo = nRightHi >> 1;

  nWidth <<= 1;
  int delta_y = (SIM_HEIGHT - nBottom) / 2;
  if (delta_y < 0) delta_y = 0;

  short* pdsScan = pdsStart + lPitchDW * delta_y;
  int delta_x_hi = (SIM_WIDTH - nRightHi)  / 2;
  if (delta_x_hi < 0) delta_x_hi = 0;
  int delta_x_lo = (SIM_WIDTH - nRightLo)  / 2;
  if (delta_x_lo < 0) delta_x_lo = 0;

  int y = nBottom;
  while (y-- > 0) {
    short *pds = pdsScan;
    int delta_x;

    if (*pfHiRes++) delta_x = delta_x_hi;
    else            delta_x = delta_x_lo;
    pds += delta_x;

    int len = SIM_WIDTH - delta_x;
    while (len > 0) {
      *pds++ = aulPalette[*pb++];
      *pds++ = aulPalette[*pb++];
      *pds++ = aulPalette[*pb++];
      *pds++ = aulPalette[*pb++];
      len -= 4;
    }
    pb  = pbSAM;
    pds = (short *)(pdsScan + lPitchDW) + delta_x;

    len = SIM_WIDTH - delta_x;
    while (len > 0) {
      *pds++ = aulScanline[*pb++];
      *pds++ = aulScanline[*pb++];
      *pds++ = aulScanline[*pb++];
      *pds++ = aulScanline[*pb++];
      len -= 4;
    }
    pb = (pbSAM += lPitch);
    pdsScan += lPitchDW;
  }
}

static int sim_current_fps = 0;

// Draw the changed lines in the appropriate colour depth and hi/low resolution
bool 
DrawChanges (CScreen* pScreen_, SDL_Surface* pSurface_)
{
  loc_Draw_Normal(pScreen_);
  //LUDO:
  if (psp_kbd_is_danzeff_mode()) {
    danzeff_moveTo(-100, -50);
    danzeff_render();
  }
  int view_fps = GetOption(view_fps);
  if (view_fps) {
    char buffer[32];
    sprintf(buffer, "%3d", (int)sim_current_fps );
    psp_sdl_fill_print(0, 0, buffer, 0xffffff, 0 );
  }
  return true;
}


static void
sim_synchronize(void)
{
  static u32 nextclock = 1;
  static u32 next_sec_clock = 0;
  static u32 cur_num_frame = 0;

  u32 curclock = SDL_GetTicks();

  int view_fps = GetOption(view_fps);
  int speed_limiter = GetOption(speed_limiter);

  if (speed_limiter) {
    while (curclock < nextclock) {
     curclock = SDL_GetTicks();
    }
    u32 f_period = 1000 / speed_limiter;
    nextclock += f_period;
    if (nextclock < curclock) nextclock = curclock + f_period;
  }

  if (view_fps) {
    cur_num_frame++;
    if (curclock > next_sec_clock) {
      next_sec_clock = curclock + 1000;
      sim_current_fps = cur_num_frame;
      cur_num_frame = 0;
    }
  }
}

// Update the display to show anything that's changed since last time
void Display::Update (CScreen* pScreen_)
{
# if 0 //LUDO:
    // Don't draw if fullscreen but not active
    if (GetOption(fullscreen) && !g_fActive)
        return;
# endif
    // Draw any changed lines
    DrawChanges(pScreen_, back_surface);
    psp_sdl_flip();

    if (psp_screenshot_mode) {
      psp_screenshot_mode--;
      if (psp_screenshot_mode <= 0) {
        psp_sdl_save_screenshot();
        psp_screenshot_mode = 0;
      }
    }

    sim_synchronize();
}


// Scale a client size/movement to one relative to the SAM view port size
// Should round down and be consistent with positive and negative values
void Display::DisplayToSamSize (int* pnX_, int* pnY_)
{
    int nHalfWidth = !GUI::IsActive(), nHalfHeight = 0;

    *pnX_ = *pnX_ * (rSource.w >> nHalfWidth)  / rTarget.w;
    *pnY_ = *pnY_ * (rSource.h >> nHalfHeight) / rTarget.h;
}

// Scale a size/movement in the SAM view port to one relative to the client
// Should round down and be consistent with positive and negative values
void Display::SamToDisplaySize (int* pnX_, int* pnY_)
{
    int nHalfWidth = !GUI::IsActive(), nHalfHeight = 0;

    *pnX_ = *pnX_ * rTarget.w / (rSource.w >> nHalfWidth);
    *pnY_ = *pnY_ * rTarget.h / (rSource.h >> nHalfHeight);
}

// Map a client point to one relative to the SAM view port
void Display::DisplayToSamPoint (int* pnX_, int* pnY_)
{
    *pnX_ -= rTarget.x;
    *pnY_ -= rTarget.y;
    DisplayToSamSize(pnX_, pnY_);
}

// Map a point in the SAM view port to a point relative to the client position
void Display::SamToDisplayPoint (int* pnX_, int* pnY_)
{
    SamToDisplaySize(pnX_, pnY_);
    *pnX_ += rTarget.x;
    *pnY_ += rTarget.y;
}
