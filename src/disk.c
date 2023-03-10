/********************************************************************/
/*                                                                  */
/*            L   I  QQ  U U I DD    W   W  A  RR    555            */
/*            L   I Q  Q U U I D D   W   W A A R R   5              */
/*            L   I Q  Q U U I D D   W W W AAA RR    55             */
/*            L   I Q Q  U U I D D   WW WW A A R R     5            */
/*            LLL I  Q Q  U  I DD    W   W A A R R   55             */
/*                                                                  */
/*                             b                                    */
/*                             bb  y y                              */
/*                             b b yyy                              */
/*                             bb    y                              */
/*                                 yy                               */
/*                                                                  */
/*                     U U       FFF  O   O  TTT                    */
/*                     U U       F   O O O O  T                     */
/*                     U U TIRET FF  O O O O  T                     */
/*                     U U       F   O O O O  T                     */
/*                      U        F    O   O   T                     */
/*                                                                  */
/********************************************************************/

/*****************************************************************************/
/* Liquid War is a multiplayer wargame                                       */
/* Copyright (C) 1998-2020 Christian Mauduit                                 */
/*                                                                           */
/* This program is free software; you can redistribute it and/or modify      */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation; either version 2 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software               */
/* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */
/*                                                                           */
/* Liquid War homepage : https://ufoot.org/liquidwar/v5                   */
/* Contact author      : ufoot@ufoot.org                                     */
/*****************************************************************************/

/********************************************************************/
/* nom           : disk.c                                           */
/* contenu       : lecture des donnees du fichier .dat              */
/* date de modif : 3 mai 98                                         */
/********************************************************************/

/*==================================================================*/
/* includes                                                         */
/*==================================================================*/

#include <dirent.h>
#include <string.h>
#include <allegro5/allegro.h>

#include "alleg2.h"
#include "init.h"
#include "disk.h"
#include "log.h"
#include "map.h"
#include "startup.h"
#include "texture.h"
#include "macro.h"
#include "path.h"

/*==================================================================*/
/* defines                                                          */
/*==================================================================*/

#define SAMPLE_SFX_NUMBER  6

/*==================================================================*/
/* variables globales                                               */
/*==================================================================*/

int SAMPLE_WATER_NUMBER = 0;
int RAW_TEXTURE_NUMBER = 0;
int RAW_MAPTEX_NUMBER = 0;
int RAW_MAP_NUMBER = 0;
int MIDI_MUSIC_NUMBER = 0;

int LOADED_FONT = 0;
int LOADED_MAP = 0;
int LOADED_BACK = 0;
int LOADED_TEXTURE = 0;
int LOADED_MAPTEX = 0;
int LOADED_SFX = 0;
int LOADED_WATER = 0;
int LOADED_MUSIC = 0;

ALLEGRO_SAMPLE *SAMPLE_SFX_TIME = NULL;
ALLEGRO_SAMPLE *SAMPLE_SFX_WIN = NULL;
ALLEGRO_SAMPLE *SAMPLE_SFX_GO = NULL;
ALLEGRO_SAMPLE *SAMPLE_SFX_CLICK = NULL;
ALLEGRO_SAMPLE *SAMPLE_SFX_LOOSE = NULL;
ALLEGRO_SAMPLE *SAMPLE_SFX_CONNECT = NULL;

ALLEGRO_SAMPLE *SAMPLE_WATER[SAMPLE_WATER_MAX_NUMBER];
char *RAW_MAP[RAW_MAP_MAX_NUMBER];
char *RAW_MAP_ORDERED[RAW_MAP_MAX_NUMBER];
ALLEGRO_BITMAP *RAW_TEXTURE[RAW_TEXTURE_MAX_NUMBER];
ALLEGRO_BITMAP *RAW_MAPTEX[RAW_TEXTURE_MAX_NUMBER];
//MIDI *MIDI_MUSIC[MIDI_MUSIC_MAX_NUMBER];

ALLEGRO_BITMAP *BACK_IMAGE = NULL;

ALLEGRO_FONT *BIG_FONT = NULL;
ALLEGRO_FONT *SMALL_FONT = NULL;
ALLEGRO_BITMAP *BIG_MOUSE_CURSOR = NULL;
ALLEGRO_BITMAP *SMALL_MOUSE_CURSOR = NULL;
ALLEGRO_BITMAP *INVISIBLE_MOUSE_CURSOR = NULL;

//static RGB *FONT_PALETTE = NULL;
//static RGB *BACK_PALETTE = NULL;

static int CUSTOM_TEXTURE_OK = 0;
static int CUSTOM_MAP_OK = 0;
static int CUSTOM_MUSIC_OK = 0;

/*------------------------------------------------------------------*/
/* chargement des effets sonores                                    */
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
static void
lock_sound (ALLEGRO_SAMPLE * smp)
{
  LOCK_VARIABLE (*smp);
#ifdef DOS
  _go32_dpmi_lock_data (smp->data, (smp->bits / 8) * smp->len);
#else
  LW_MACRO_NOP (smp);
#endif
}

/*------------------------------------------------------------------*/
static ALLEGRO_SAMPLE *read_sample(const char* subdir, const char *filename) {
  ALLEGRO_SAMPLE *ret = NULL;

  char * path = lw_path_join3(STARTUP_DAT_PATH, subdir, filename);
  if (path == NULL) {
    return NULL;
  }
  ret = al_load_sample(path);
  free(path);
  return ret;
}

/*------------------------------------------------------------------*/
static bool
read_sfx_dat ()
{
  SAMPLE_SFX_TIME = read_sample("sfx", "clock1.wav");
  SAMPLE_SFX_WIN = read_sample("sfx", "crowd1.wav");
  SAMPLE_SFX_CONNECT = read_sample("sfx", "cuckoo.wav");
  SAMPLE_SFX_GO = read_sample("sfx", "foghorn.wav");
  SAMPLE_SFX_CLICK = read_sample("sfx", "spash1.wav");
  SAMPLE_SFX_LOOSE = read_sample("sfx", "war.wav");

  return SAMPLE_SFX_TIME != NULL &&
    SAMPLE_SFX_WIN != NULL &&
    SAMPLE_SFX_CONNECT != NULL &&
    SAMPLE_SFX_GO != NULL &&
    SAMPLE_SFX_CLICK != NULL &&
    SAMPLE_SFX_LOOSE != NULL;
}

/*------------------------------------------------------------------*/
static bool
read_water_dat ()
{
  int i;

  char* dir_path = lw_path_join2(STARTUP_DAT_PATH, "water");
  if (dir_path == NULL) {
      return false;
  }
  DIR* dir = opendir(dir_path);
  free(dir_path);
  if (dir == NULL) {
      return false;
  }

  struct dirent* ent;
  bool ret = true;
  for (i = 0; i < SAMPLE_WATER_DAT_NUMBER && (ent = readdir(dir)) != NULL; ++i)
    {
      if (ent->d_type != DT_REG || strcmp(ent->d_name, "Makefile.in") == 0) {
          --i;
          continue;
      }
      SAMPLE_WATER[i] = read_sample("water", ent->d_name);
      lock_sound (SAMPLE_WATER[i]);
      SAMPLE_WATER_NUMBER++;
      ret &= SAMPLE_WATER[i] != NULL;
    }

  return ret;
}

/*------------------------------------------------------------------*/
/* chargement des autres donnees                                    */
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
static ALLEGRO_BITMAP *read_bitmap(const char* subdir, const char *filename) {
  ALLEGRO_BITMAP *ret = NULL;

  char * path = lw_path_join3(STARTUP_DAT_PATH, subdir, filename);
  if (path == NULL) {
    return NULL;
  }
  ret = al_load_bitmap(path);
  free(path);
  return ret;
}

/*------------------------------------------------------------------*/
static bool
read_texture_dat ()
{
  int i;

  char* dir_path = lw_path_join2(STARTUP_DAT_PATH, "texture");
  if (dir_path == NULL) {
      return false;
  }
  DIR* dir = opendir(dir_path);
  free(dir_path);
  if (dir == NULL) {
      return false;
  }

  struct dirent* ent;
  bool ret = true;
  for (i = 0; i < RAW_TEXTURE_DAT_NUMBER && (ent = readdir(dir)) != NULL; ++i)
    {
      if (ent->d_type != DT_REG || strcmp(ent->d_name, "Makefile.in") == 0) {
          --i;
          continue;
      }
      RAW_TEXTURE[i] = read_bitmap("texture", ent->d_name);
      RAW_TEXTURE_NUMBER++;
      ret &= RAW_TEXTURE[i] != NULL;
    }

  return ret;
}

/*------------------------------------------------------------------*/
static bool
read_maptex_dat ()
{
  int i;

  char* dir_path = lw_path_join2(STARTUP_DAT_PATH, "map");
  if (dir_path == NULL) {
      return false;
  }
  DIR* dir = opendir(dir_path);
  free(dir_path);
  if (dir == NULL) {
      return false;
  }

  struct dirent* ent;
  bool ret = true;
  for (i = 0; i < RAW_MAP_DAT_NUMBER && (ent = readdir(dir)) != NULL; ++i)
    {
      if (ent->d_type != DT_REG || strcmp(ent->d_name + strlen(ent->d_name) - 4, "pcx") != 0) {
          --i;
          continue;
      }
      RAW_MAPTEX[i] = read_bitmap("map", ent->d_name);
      RAW_MAPTEX_NUMBER++;
      ret &= RAW_MAPTEX[i] != NULL;
    }

  return ret;
}

/*------------------------------------------------------------------*/
static bool
read_map_dat ()
{
  int i;

  char* dir_path = lw_path_join2(STARTUP_DAT_PATH, "map");
  if (dir_path == NULL) {
      return false;
  }
  DIR* dir = opendir(dir_path);
  free(dir_path);
  if (dir == NULL) {
      return false;
  }

  struct dirent* ent;
  bool ret = true;
  for (i = 0; i < RAW_MAP_DAT_NUMBER && (ent = readdir(dir)) != NULL; ++i)
    {
      if (ent->d_type != DT_REG || strcmp(ent->d_name + strlen(ent->d_name) - 4, "txt") != 0) {
          --i;
          continue;
      }
      char* file_path = lw_path_join3(STARTUP_DAT_PATH, "map", ent->d_name);
      if (path == NULL) {
          return false;
      }
      FILE* file = fopen(file_path, "rb");
      free(file_path);
      if (file == NULL) {
          return false;
      }
      char buf[1024];
      size_t len = fread(buf, 1, sizeof(buf), file);
      RAW_MAP[i] = malloc(len + 1);
      ret &= RAW_MAP[i] != NULL;
      RAW_MAP_NUMBER++;
      if (RAW_MAP[i] == NULL) {
          continue;
      }
      memcpy(RAW_MAP[i], buf, len);
      RAW_MAP[i][len] = 0;
    }

  return ret;
}

/*------------------------------------------------------------------*/
static void
read_back_dat ()
{
  int i, x, y;

  BACK_PALETTE = read_bitmap("back", "palette.pcx");
  BACK_IMAGE = read_bitmap("back", "lw5back.pcx");

  /*
   * strange, with Allegro 4.0, the liquidwarcol utility
   * and the datafile compiler do not work so well together,
   * and so the palette stored in the datafile always
   * start at color 0, which explains the "18 shift"
   */

  /*for (i = 0; i <= 45; ++i)
    GLOBAL_PALETTE[i + 18] = BACK_PALETTE[i];*/

  for (x = 0; x < BACK_IMAGE->w; ++x)
    for (y = 0; y < BACK_IMAGE->w; ++y)
      {
        putpixel (BACK_IMAGE, x, y, getpixel (BACK_IMAGE, x, y) + 18);
      }
}

/*------------------------------------------------------------------*/
static bool
read_font_dat ()
{
  FONT_PALETTE = read_bitmap("font", "palette.pcx");
  SMALL_FONT = read_bitmap("font", "degrad10.pcx");
  BIG_FONT = read_bitmap("font", "degrad20.pcx");
  SMALL_MOUSE_CURSOR = read_bitmap("font", "mouse20.pcx");
  BIG_MOUSE_CURSOR = read_bitmap("font", "mouse40.pcx");
  INVISIBLE_MOUSE_CURSOR = read_bitmap("font", "void1.pcx");

  return FONT_PALETTE != NULL && SMALL_FONT != NULL & BIG_FONT != NULL & SMALL_MOUSE_CURSOR != NULL & BIG_MOUSE_CURSOR != NULL & INVISIBLE_MOUSE_CURSOR != NULL;
}

/*------------------------------------------------------------------*/
static void
read_music_dat ()
{
  /* int i; */

  /* MIDI_MUSIC_NUMBER = 0; */
  /* for (i = 0; i < MIDI_MUSIC_DAT_NUMBER && df[i].type != DAT_END; ++i) */
  /*   { */
  /*     MIDI_MUSIC[i] = df[i].dat; */
  /*     MIDI_MUSIC_NUMBER++; */
  /*   } */
}


/*------------------------------------------------------------------*/
static int check_loadable() {
#ifdef DOS
  loadable = 1;
#else
  loadable = exists (STARTUP_DAT_PATH);
#endif

  // Checking for the existence of this file, to quickly spot whether
  // this is a genuine data folder. If that text file is not there, we
  // can just leave and assume this is an unkown random place.
  char * path = lw_path_join(STARTUP_DAT_PATH, "liquidwar-data.txt");
  if (path == NULL) {
    return 0;
  }
  loadable = exists(path);
  free(path);

  return loadable;
}

/*------------------------------------------------------------------*/
int
load_dat (void)
{
  int result = 1;
  int loadable = 0;

  log_print_str ("Loading data from \"");
  log_print_str (STARTUP_DAT_PATH);
  log_print_str ("\"");

  loadable = check_loadable();
  display_success (loadable);

  if (loadable)
    {
      log_print_str ("Loading fonts");
      log_flush ();
      LOADED_FONT = read_font_dat ();
      display_success (LOADED_FONT);
      result &= LOADED_FONT;
    }
  if (loadable)
    {
      log_print_str ("Loading maps");
      log_flush ();
      LOADED_FONT = read_map_dat ();
      display_success (LOADED_MAP);
      result &= LOADED_MAP;
    }
  if (loadable && STARTUP_BACK_STATE)
    {
      log_print_str ("Loading background bitmap");
      log_flush ();
      LOADED_BACK = read_back_dat();
      display_success(LOADED_BACK);
      result &= LOADED_BACK;
    }

  if (loadable && STARTUP_SFX_STATE)
    {
      log_print_str ("Loading sound fx");
      log_flush ();
      if (read_sfx_dat())
        {
          LOADED_SFX = 1;
          display_success(1);
        }
      else
        {
          result &= !STARTUP_CHECK;
          display_success(0);
        }
    }
  if (loadable && STARTUP_TEXTURE_STATE)
    {
      log_print_str ("Loading textures");
      log_flush ();
      if (read_texture_dat())
        {
          LOADED_TEXTURE = 1;
          display_success(1);
        }
      else
        {
          result &= !STARTUP_CHECK;
          display_success(0);
        }

      log_print_str ("Loading map textures");
      log_flush ();
      if (read_maptex_dat())
        {
          LOADED_MAPTEX = 1;
          display_success(1);
        }
      else
        {
          result &= !STARTUP_CHECK;
          display_success(0);
        }
    }

  if (loadable && STARTUP_WATER_STATE)
    {
      log_print_str ("Loading water sounds");
      log_flush ();
      if (read_water_dat())
        {
          LOADED_WATER = 1;
          display_success(1);
        }
      else
        {
          result &= !STARTUP_CHECK;
          display_success(0);
        }
    }

  if (loadable && STARTUP_MUSIC_STATE)
    {
      log_print_str ("Loading midi music");
      log_flush ();
      if (read_music_dat())
        {
          LOADED_MUSIC = 1;
          display_success(1);
        }
      else
        {
          result &= !STARTUP_CHECK;
          display_success(0);
        }
    }

  return loadable && result;
}

/*------------------------------------------------------------------*/
static int
load_custom_texture_callback (const char *file, int mode, void *unused)
{
  void *pointeur;

  LW_MACRO_NOP (mode);
  LW_MACRO_NOP (unused);

  if ((pointeur = lw_texture_archive_raw (file)) != NULL)
    {
      RAW_TEXTURE[RAW_TEXTURE_NUMBER++] = pointeur;
      log_print_str ("+");
      CUSTOM_TEXTURE_OK = 1;
    }
  else
    {
      log_print_str ("-");
    }
  log_flush ();

  return 0;
}

/*------------------------------------------------------------------*/
static int
load_custom_texture (void)
{
  int result = 1;
  char buf[512];

  LW_MACRO_SPRINTF1 (buf, "%s\\*.*", STARTUP_TEX_PATH);

  fix_filename_case (buf);
  fix_filename_slashes (buf);

  CUSTOM_TEXTURE_OK = 0;
  for_each_file_ex (buf, 0, FA_DIREC, load_custom_texture_callback, NULL);
  result = CUSTOM_TEXTURE_OK;

  return result;
}

/*------------------------------------------------------------------*/
static int
load_custom_map_callback (const char *file, int mode, void *unused)
{
  void *pointeur;

  LW_MACRO_NOP (mode);
  LW_MACRO_NOP (unused);

  if ((pointeur = lw_map_archive_raw (file)) != NULL)
    {
      RAW_MAP[RAW_MAP_NUMBER++] = pointeur;
      log_print_str ("+");
      CUSTOM_MAP_OK = 1;
    }
  else
    {
      log_print_str ("-");
    }

  return 0;
}

/*------------------------------------------------------------------*/
static int
load_custom_map (void)
{
  int result = 1;
  char buf[512];

  LW_MACRO_SPRINTF1 (buf, "%s\\*.*", STARTUP_MAP_PATH);

  fix_filename_case (buf);
  fix_filename_slashes (buf);

  CUSTOM_MAP_OK = 0;
  for_each_file_ex (buf, 0, FA_DIREC, load_custom_map_callback, NULL);
  result = CUSTOM_MAP_OK;

  return result;
}

/*------------------------------------------------------------------*/
static int
load_custom_music_callback (const char *file, int mode, void *unused)
{
  void *pointeur;

  LW_MACRO_NOP (mode);
  LW_MACRO_NOP (unused);

  if ((pointeur = load_midi (file)) != NULL)
    {
      MIDI_MUSIC[MIDI_MUSIC_NUMBER++] = pointeur;
      log_print_str ("+");
      CUSTOM_MUSIC_OK = 1;
    }
  else
    {
      log_print_str ("-");
    }
  log_flush ();

  return 0;
}

/*------------------------------------------------------------------*/
static int
load_custom_music (void)
{
  int result = 1;
  char buf[512];

  LW_MACRO_SPRINTF1 (buf, "%s\\*.*", STARTUP_MID_PATH);

  fix_filename_case (buf);
  fix_filename_slashes (buf);

  CUSTOM_MUSIC_OK = 0;
  for_each_file_ex (buf, 0, FA_DIREC, load_custom_music_callback, NULL);
  result = CUSTOM_MUSIC_OK;

  return result;
}

/*------------------------------------------------------------------*/
int
load_custom (void)
{
  int success, result = 1;

  if (STARTUP_TEXTURE_STATE && STARTUP_CUSTOM_STATE)
    {
      log_print_str ("Loading custom textures from \"");
      log_print_str (STARTUP_TEX_PATH);
      log_print_str ("\" ");
      log_flush ();
      success = load_custom_texture ();
      if (!success)
        result &= !STARTUP_CHECK;
      display_success (success);
    }

  if (STARTUP_CUSTOM_STATE)
    {
      log_print_str ("Loading custom maps from \"");
      log_print_str (STARTUP_MAP_PATH);
      log_print_str ("\" ");
      log_flush ();
      success = load_custom_map ();
      if (!success)
        result &= !STARTUP_CHECK;
      display_success (success);
    }

  if (STARTUP_CUSTOM_STATE && STARTUP_MUSIC_STATE)
    {
      log_print_str ("Loading custom musics from \"");
      log_print_str (STARTUP_MID_PATH);
      log_print_str ("\" ");
      log_flush ();
      success = load_custom_music ();
      if (!success)
        result &= !STARTUP_CHECK;
      display_success (success);
    }

  return result;
}

/*------------------------------------------------------------------*/
void
order_map (void)
{
  int incorrect_order = 1;
  int i;


  char name1[LW_MAP_READABLE_NAME_SIZE + 1];
  char name2[LW_MAP_READABLE_NAME_SIZE + 1];
  void *temp;

  for (i = 0; i < RAW_MAP_NUMBER; ++i)
    {
      RAW_MAP_ORDERED[i] = RAW_MAP[i];
    }

  while (incorrect_order)
    {
      incorrect_order = 0;

      for (i = 0; i < RAW_MAP_NUMBER - 1; ++i)
        {
          LW_MACRO_STRCPY (name1, lw_map_get_readable_name (i, 0, 0));
          LW_MACRO_STRCPY (name2, lw_map_get_readable_name (i + 1, 0, 0));
          if (strcmp (name1, name2) > 0)
            {
              incorrect_order = 1;

              temp = RAW_MAP_ORDERED[i];
              RAW_MAP_ORDERED[i] = RAW_MAP_ORDERED[i + 1];
              RAW_MAP_ORDERED[i + 1] = temp;
            }
        }
    }
}
