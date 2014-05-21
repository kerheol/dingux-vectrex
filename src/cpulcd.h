/*  EnergySaver for GP2X

	File: cpulcd.h
	
	Copyright (C) 2006  Kounch
	Parts Copyright (c) 2005 Rlyeh
	Parts Copyright (c) 2006 Hermes/PS2Reality

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __CPULCD_H__
#define __CPULCD_H__

#ifdef __cplusplus
extern "C" {
#endif

/*Definicion de funciones*/

void cpulcd_init(void);

int cpulcd_deinit(void);

void save_system_regs(void);

void load_system_regs(void);

unsigned get_speed_clock(void);

void set_speed_clock(int gp2x_speed);

unsigned get_freq_920_CLK();

unsigned short get_920_Div();

unsigned get_display_clock_div();

void set_display_clock_div(unsigned div);

void set_FCLK(unsigned MHZ);

void set_DCLK_Div( unsigned short div );

void set_920_Div(unsigned short div);

void Disable_940(void);

unsigned short Disable_Int_940(void);

unsigned get_status_UCLK();

unsigned get_status_ACLK();

void set_status_UCLK(unsigned s);

void set_status_ACLK(unsigned s);

void set_display(int mode);


/* Frecuencia de reloj de sistema */
#define SYS_CLK_FREQ 7372800

#ifdef __cplusplus
}
#endif

#endif
