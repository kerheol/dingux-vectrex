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

#include "cpuvoid.h"

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
}

unsigned int
cpu_get_clock()
{
  return 0;
}

void
cpu_set_default()
{
}
