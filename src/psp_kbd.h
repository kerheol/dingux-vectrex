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

# ifndef _KBD_H_
# define _KBD_H_

# define PSP_ALL_BUTTON_MASK 0xFFFF

 enum dve_keys_emum {

   DVE_JOY_1,          
   DVE_JOY_2,          
   DVE_JOY_3,          
   DVE_JOY_4,          
   DVE_JOY_UP,     
   DVE_JOY_DOWN,   
   DVE_JOY_LEFT,   
   DVE_JOY_RIGHT,  
   DVE_C_FPS,
   DVE_C_RENDER,
   DVE_C_LOAD,
   DVE_C_SAVE,
   DVE_C_RESET,
   DVE_C_AUTOFIRE,
   DVE_C_INCFIRE,
   DVE_C_DECFIRE,
   DVE_C_SCREEN,

   DVE_MAX_KEY      
  };

# define KBD_UP           0
# define KBD_RIGHT        1
# define KBD_DOWN         2
# define KBD_LEFT         3
# define KBD_TRIANGLE     4
# define KBD_CIRCLE       5
# define KBD_CROSS        6
# define KBD_SQUARE       7
# define KBD_SELECT       8
# define KBD_START        9
# define KBD_LTRIGGER    10
# define KBD_RTRIGGER    11

# define KBD_MAX_BUTTONS 12

# define KBD_JOY_UP      12
# define KBD_JOY_RIGHT   13
# define KBD_JOY_DOWN    14
# define KBD_JOY_LEFT    15

# define KBD_ALL_BUTTONS 16

# define KBD_UNASSIGNED         -1

# define KBD_LTRIGGER_MAPPING   -2
# define KBD_RTRIGGER_MAPPING   -3
# define KBD_NORMAL_MAPPING     -1

 struct dve_key_trans {
   int  key;
   int  bit_mask;
   char name[10];
 };
  
  extern int psp_screenshot_mode;
  extern int psp_kbd_mapping[ KBD_ALL_BUTTONS ];
  extern int psp_kbd_mapping_L[ KBD_ALL_BUTTONS ];
  extern int psp_kbd_mapping_R[ KBD_ALL_BUTTONS ];
  extern int psp_kbd_presses[ KBD_ALL_BUTTONS ];
  extern int kbd_ltrigger_mapping_active;
  extern int kbd_rtrigger_mapping_active;

  extern struct dve_key_trans psp_dve_key_info[DVE_MAX_KEY];

  extern int  psp_update_keys(void);
  extern void kbd_wait_start(void);
  extern void psp_init_keyboard(void);
  extern void psp_kbd_wait_no_button(void);
  extern int  psp_kbd_is_danzeff_mode(void);
  extern void psp_kbd_display_active_mapping(void);
  extern int psp_kbd_load_mapping(char *kbd_filename);
  extern int psp_kbd_save_mapping(char *kbd_filename);
  extern void psp_kbd_run_command(char *Command, int wait);
  extern int psp_kbd_reset_mapping(void);
  extern void kbd_change_auto_fire(int auto_fire);
  extern void kbd_get_analog_direction(int Analog_x, int Analog_y, int *x, int *y);
  extern int psp_kbd_reset_hotkeys(void);
# endif
