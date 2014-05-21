/*  
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


#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include "cpucaanoo.h"

#define msleep(x) usleep(x*1000)

enum _pollux_clock_ioctl {
    IOCTL_SET_PLL0,
    IOCTL_SET_PLL1,
    IOCTL_GET_PLL0,
    IOCTL_GET_PLL1,
    IOCTL_SET_CLK_DEFUALTS
};


static unsigned int loc_clock_in_mhz = GP2X_DEF_CLOCK;

static void 
loc_set_clock(unsigned int clock_in_mhz)
{
 	int index=0;
  if(clock_in_mhz<=300)
	   	index=1;
	else if(clock_in_mhz<=400)
		index=2;
	else if(clock_in_mhz<=466)
		index=3;
	else if(clock_in_mhz<=533)
		index=4;
	else if(clock_in_mhz<=633)
		index=6;
	else if(clock_in_mhz<=666)
		index=7;
	else if(clock_in_mhz<=700)
		index=9;
	else if(clock_in_mhz<=766)
		index=10;
	else if(clock_in_mhz<=800)
		index=11;
	else if(clock_in_mhz<=833)
		index=12;
	else if(clock_in_mhz<=866)
		index=13;
	else if(clock_in_mhz<=900)
		index=13;
		
	   	
  int fd_clk = open( "/dev/pollux_clock" , O_RDWR);
	if(fd_clk < 0) {
	    //printf("error: failed to open \n");
	}
	
	ioctl(fd_clk, IOCTL_SET_PLL0, &index);
  msleep(200);
  close(fd_clk);
}

void
cpu_init()
{
}

void
cpu_deinit()
{
}

void 
cpu_set_clock(unsigned int clock_in_mhz)
{
  if (clock_in_mhz == loc_clock_in_mhz) return;
  loc_clock_in_mhz = clock_in_mhz;
  
  loc_set_clock(clock_in_mhz);

  return;
}

unsigned int
cpu_get_clock()
{
  return loc_clock_in_mhz;
}

void
cpu_set_default()
{
  cpu_set_clock( GP2X_DEF_CLOCK );
}
