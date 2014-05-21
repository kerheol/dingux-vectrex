#include <stdio.h>
#include <zlib.h>
#include <string.h>
#include "e6809.h"
#include "vecx.h"
#include "osint.h"

enum {
  VECTREX_PDECAY  = 30,      /* phosphor decay rate */
  
  /* number of 6809 cycles before a frame redraw */

  FCYCLES_INIT    = VECTREX_MHZ / VECTREX_PDECAY,

  /* max number of possible vectors that maybe on the screen at one time.
   * one only needs VECTREX_MHZ / VECTREX_PDECAY but we need to also store
   * deleted vectors in a single table
   */
  VECTOR_CNT    = 0x10000
};

  typedef struct vecx_save_t {
       long vector_draw_cnt;
       long vector_erse_cnt;
       unsigned snd_select;
       unsigned via_ora;
       unsigned via_orb;
       unsigned via_ddra;
       unsigned via_ddrb;
       unsigned via_t1on;  /* is timer 1 on? */
       unsigned via_t1int; /* are timer 1 interrupts allowed? */
       unsigned short via_t1c;
       unsigned char  via_t1ll;
       unsigned via_t1lh;
       unsigned via_t1pb7; /* timer 1 controlled version of pb7 */
       unsigned via_t2on;  /* is timer 2 on? */
       unsigned via_t2int; /* are timer 2 interrupts allowed? */
       unsigned short via_t2c;
       unsigned char via_t2ll;
       unsigned via_sr;
       unsigned via_srb;   /* number of bits shifted so far */
       unsigned char via_src;   /* shift counter */
       unsigned via_srclk;
       unsigned via_acr;
       unsigned via_pcr;
       unsigned via_ifr;
       unsigned via_ier;
       unsigned char via_ca2;
       unsigned via_cb2h;  /* basic handshake version of cb2 */
       unsigned via_cb2s;  /* version of cb2 controlled by the shift register */
       unsigned alg_rsh;  /* zero ref sample and hold */
       unsigned alg_xsh;  /* x sample and hold */
       unsigned alg_ysh;  /* y sample and hold */
       unsigned alg_zsh;  /* z sample and hold */
       unsigned alg_jch0;      /* joystick direction channel 0 */
       unsigned alg_jch1;      /* joystick direction channel 1 */
       unsigned alg_jch2;      /* joystick direction channel 2 */
       unsigned alg_jch3;      /* joystick direction channel 3 */
       unsigned alg_jsh;  /* joystick sample and hold */
       unsigned alg_compare;
       long alg_dx;     /* delta x */
       long alg_dy;     /* delta y */
       point_t alg_curr; /* current x position */
       unsigned alg_vectoring; /* are we drawing a vector right now? */
       point_t alg_vector0;
       point_t alg_vector1;
       long alg_vector_dx;
       long alg_vector_dy;
       unsigned char alg_vector_color;

  } vecx_save_t;

unsigned char rom[8192];
unsigned char cart[32768];
unsigned char ram[1024];

unsigned snd_regs[16];
static unsigned snd_select;

/* the via 6522 registers */

static unsigned via_ora;
static unsigned via_orb;
static unsigned via_ddra;
static unsigned via_ddrb;
static unsigned via_t1on;  /* is timer 1 on? */
static unsigned via_t1int; /* are timer 1 interrupts allowed? */
static unsigned short via_t1c;
static unsigned char  via_t1ll;
static unsigned via_t1lh;
static unsigned via_t1pb7; /* timer 1 controlled version of pb7 */
static unsigned via_t2on;  /* is timer 2 on? */
static unsigned via_t2int; /* are timer 2 interrupts allowed? */
static unsigned short via_t2c;
static unsigned char via_t2ll;
static unsigned via_sr;
static unsigned via_srb;   /* number of bits shifted so far */
static unsigned char via_src;   /* shift counter */
static unsigned via_srclk;
static unsigned via_acr;
static unsigned via_pcr;
static unsigned via_ifr;
static unsigned via_ier;
static unsigned char via_ca2;
static unsigned via_cb2h;  /* basic handshake version of cb2 */
static unsigned via_cb2s;  /* version of cb2 controlled by the shift register */

/* analog devices */

static unsigned alg_rsh;  /* zero ref sample and hold */
static unsigned alg_xsh;  /* x sample and hold */
static unsigned alg_ysh;  /* y sample and hold */
static unsigned alg_zsh;  /* z sample and hold */
       unsigned alg_jch0;      /* joystick direction channel 0 */
       unsigned alg_jch1;      /* joystick direction channel 1 */
       unsigned alg_jch2;      /* joystick direction channel 2 */
       unsigned alg_jch3;      /* joystick direction channel 3 */
static unsigned alg_jsh;  /* joystick sample and hold */

static unsigned alg_compare;

static long alg_dx;     /* delta x */
static long alg_dy;     /* delta y */
static point_t alg_curr; /* current x position */

static unsigned alg_vectoring; /* are we drawing a vector right now? */
static point_t alg_vector0;
static point_t alg_vector1;
static long alg_vector_dx;
static long alg_vector_dy;
static unsigned char alg_vector_color;
static vector_t vectors_set[2 * VECTOR_CNT];


long vector_draw_cnt;
long vector_erse_cnt;

vector_t *vectors_draw;
vector_t *vectors_erse;

/* update the snd chips internal registers when via_ora/via_orb changes */

static inline void snd_update (void)
{
  switch (via_orb & 0x18) {
  case 0x00:
    /* the sound chip is disabled */
    break;
  case 0x08:
    /* the sound chip is sending data */
    break;
  case 0x10:
    /* the sound chip is recieving data */

    if (snd_select != 14) {
      e8910_write(snd_select, via_ora);
    }

    break;
  case 0x18:
    /* the sound chip is latching an address */
    
    if ((via_ora & 0xf0) == 0x00) {
      snd_select = via_ora & 0x0f;
    }

    break;
  }
}

/* update the various analog values when orb is written. */

static inline void alg_update (void)
{
  switch (via_orb & 0x06) {
  case 0x00:
    alg_jsh = alg_jch0;

    if ((via_orb & 0x01) == 0x00) {
      /* demultiplexor is on */
      alg_ysh = alg_xsh;
    }

    break;
  case 0x02:
    alg_jsh = alg_jch1;

    if ((via_orb & 0x01) == 0x00) {
      /* demultiplexor is on */
      alg_rsh = alg_xsh;
    }

    break;
  case 0x04:
    alg_jsh = alg_jch2;

    if ((via_orb & 0x01) == 0x00) {
      /* demultiplexor is on */

      if (alg_xsh > 0x80) {
        alg_zsh = alg_xsh - 0x80;
      } else {
        alg_zsh = 0;
      }
    }

    break;
  case 0x06:
    /* sound output line */
    alg_jsh = alg_jch3;
    break;
  }

  /* compare the current joystick direction with a reference */

  if (alg_jsh > alg_xsh) {
    alg_compare = 0x20;
  } else {
    alg_compare = 0;
  }

  /* compute the new "deltas" */

  alg_dx = (long) alg_xsh - (long) alg_rsh;
  alg_dy = (long) alg_rsh - (long) alg_ysh;
}

/* update IRQ and bit-7 of the ifr register after making an adjustment to
 * ifr.
 */

static inline void int_update (void)
{
  if (via_ifr & via_ier & 0x7f) {
    via_ifr |= 0x80;
  } else {
    via_ifr &= 0x7f;
  }
}

unsigned char e6809_read8 (unsigned short address)
{
  __label__ l_0x00, l_0x01, l_0x02, l_0x03, l_0x04, l_0x05, l_0x06, l_0x07, 
  l_0x08, l_0x09, l_0x0a, l_0x0b, l_0x0c, l_0x0d, l_0x0e, l_0x0f;

    static const void* const a_jump_table[16] = {
  &&l_0x00, &&l_0x01, &&l_0x02, &&l_0x03, &&l_0x04, &&l_0x05, &&l_0x06, &&l_0x07, 
  &&l_0x08, &&l_0x09, &&l_0x0a, &&l_0x0b, &&l_0x0c, &&l_0x0d, &&l_0x0e, &&l_0x0f
   };

  if ((address & 0xe000) == 0xe000) {
    /* rom */
    return rom[address & 0x1fff];
  } 

  if ((address & 0xe000) == 0xc000) {
    if (address & 0x800) {
      /* ram */
      return ram[address & 0x3ff];
    }
    /* io */
    unsigned char data;
    //switch (address & 0xf) {
    goto *a_jump_table[address & 0xf];
    l_0x00:
      /* compare signal is an input so the value does not come from
       * via_orb.
       */

      if (via_acr & 0x80) {
        /* timer 1 has control of bit 7 */

        data = (unsigned char) ((via_orb & 0x5f) | via_t1pb7 | alg_compare);
      } else {
        /* bit 7 is being driven by via_orb */

        data = (unsigned char) ((via_orb & 0xdf) | alg_compare);
      }
      return data;
      
    l_0x01:
      /* register 1 also performs handshakes if necessary */

      if ((via_pcr & 0x0e) == 0x08) {
        /* if ca2 is in pulse mode or handshake mode, then it
         * goes low whenever ira is read.
         */
        via_ca2 = 0;
      }

      /* fall through */

    l_0x0f:
      if ((via_orb & 0x18) == 0x08) {
        /* the snd chip is driving port a */
        data = (unsigned char) snd_regs[snd_select];
      } else {
        data = (unsigned char) via_ora;
      }
      return data;
      
    l_0x02:
      data = (unsigned char) via_ddrb;
      return data;
      
    l_0x03:
      data = (unsigned char) via_ddra;
      return data;
      
    l_0x04:
      /* T1 low order counter */
      
      data = (unsigned char) via_t1c;
      via_ifr &= 0xbf; /* remove timer 1 interrupt flag */

      via_t1on = 0; /* timer 1 is stopped */
      via_t1int = 0;
      via_t1pb7 = 0x80;

      int_update ();
      return data;

      
    l_0x05:
      /* T1 high order counter */

      data = (unsigned char) (via_t1c >> 8);
      return data;
      
    l_0x06:
      /* T1 low order latch */

      data = (unsigned char) via_t1ll;
      return data;
      
    l_0x07:
      /* T1 high order latch */

      data = (unsigned char) via_t1lh;
      return data;
      
    l_0x08:
      /* T2 low order counter */

      data = (unsigned char) via_t2c;
      via_ifr &= 0xdf; /* remove timer 2 interrupt flag */

      via_t2on = 0; /* timer 2 is stopped */
      via_t2int = 0;

      int_update ();
      return data;
      
    l_0x09:
      /* T2 high order counter */

      data = (unsigned char) (via_t2c >> 8);
      return data;
      
    l_0x0a:
      data = (unsigned char) via_sr;
      via_ifr &= 0xfb; /* remove shift register interrupt flag */
      via_srb = 0;
      via_srclk = 1;

      int_update ();
      return data;
      
    l_0x0b:
      data = (unsigned char) via_acr;
      return data;
      
    l_0x0c:
      data = (unsigned char) via_pcr;
      return data;
      
    l_0x0d:
      /* interrupt flag register */

      data = (unsigned char) via_ifr;
      return data;
      
    l_0x0e:
      /* interrupt enable register */

      data = (unsigned char) (via_ier | 0x80);
      return data;
    //}
  } 

  if (address < 0x8000) {
    /* cartridge */
    return cart[address];
  }
  return 0xff;
}

unsigned e6809_read16 (unsigned short address)
{
  unsigned datahi, datalo;

  if ((address & 0xe000) == 0xe000) {
    /* rom */
    datahi = rom[address++ & 0x1fff];
    datalo = rom[address   & 0x1fff];

  } else if ((address & 0xe000) == 0xc000) {
    if (address & 0x800) {
      /* ram */
      datahi = ram[address++ & 0x3ff];
      datalo = ram[address   & 0x3ff];

    } else if (address & 0x1000) {
      /* io */
      datahi = e6809_read8(address++);
      datalo = e6809_read8(address  );
    }

  } else if (address < 0x8000) {
    /* cartridge */
    datahi = cart[address++];
    datalo = cart[address];
  } else {
    return 0xffff;
  }
  return (datahi << 8)|datalo;
}

void e6809_write8 (unsigned short address, unsigned char data)
{
  __label__ l_0x00, l_0x01, l_0x02, l_0x03, l_0x04, l_0x05, l_0x06, l_0x07, 
  l_0x08, l_0x09, l_0x0a, l_0x0b, l_0x0c, l_0x0d, l_0x0e, l_0x0f;

    static const void* const a_jump_table[16] = {
  &&l_0x00, &&l_0x01, &&l_0x02, &&l_0x03, &&l_0x04, &&l_0x05, &&l_0x06, &&l_0x07, 
  &&l_0x08, &&l_0x09, &&l_0x0a, &&l_0x0b, &&l_0x0c, &&l_0x0d, &&l_0x0e, &&l_0x0f
   };

  if ((address & 0xe000) == 0xc000) {
    /* it is possible for both ram and io to be written at the same! */

    if (address & 0x800) {
      ram[address & 0x3ff] = data;
    }
    if (address & 0x1000) {
      goto *a_jump_table[address & 0xf];
      //switch (address & 0xf) {
      l_0x00:
        via_orb = data;
        snd_update ();
        alg_update ();

        if ((via_pcr & 0xe0) == 0x80) {
          /* if cb2 is in pulse mode or handshake mode, then it
           * goes low whenever orb is written.
           */
          via_cb2h = 0;
        }

        return;
      l_0x01:
        /* register 1 also performs handshakes if necessary */

        if ((via_pcr & 0x0e) == 0x08) {
          /* if ca2 is in pulse mode or handshake mode, then it
           * goes low whenever ora is written.
           */

          via_ca2 = 0;
        }

        /* fall through */

      l_0x0f:
        via_ora = data;

        snd_update ();

        /* output of port a feeds directly into the dac which then
         * feeds the x axis sample and hold.
         */

        alg_xsh = data ^ 0x80;

        alg_update ();

        return;
      l_0x02:
        via_ddrb = data;
        return;
      l_0x03:
        via_ddra = data;
        return;
      l_0x04:
        /* T1 low order counter */
        
        via_t1ll = data;

        return;
      l_0x05:
        /* T1 high order counter */

        via_t1lh = data;
        via_t1c = (via_t1lh << 8) | via_t1ll;
        via_ifr &= 0xbf; /* remove timer 1 interrupt flag */

        via_t1on = 1; /* timer 1 starts running */
        via_t1int = 1;
        via_t1pb7 = 0;

        int_update ();

        return;
      l_0x06:
        /* T1 low order latch */

        via_t1ll = data;
        return;
      l_0x07:
        /* T1 high order latch */

        via_t1lh = data;
        return;
      l_0x08:
        /* T2 low order latch */

        via_t2ll = data;
        return;
      l_0x09:
        /* T2 high order latch/counter */

        via_t2c = (data << 8) | via_t2ll;
        via_ifr &= 0xdf;

        via_t2on = 1; /* timer 2 starts running */
        via_t2int = 1;

        int_update ();

        return;
      l_0x0a:
        via_sr = data;
        via_ifr &= 0xfb; /* remove shift register interrupt flag */
        via_srb = 0;
        via_srclk = 1;

        int_update ();

        return;
      l_0x0b:
        via_acr = data;
        return;
      l_0x0c:
        via_pcr = data;


        if ((via_pcr & 0x0e) == 0x0c) {
          /* ca2 is outputting low */

          via_ca2 = 0;
        } else {
          /* ca2 is disabled or in pulse mode or is
           * outputting high.
           */

          via_ca2 = 1;
        }

        if ((via_pcr & 0xe0) == 0xc0) {
          /* cb2 is outputting low */

          via_cb2h = 0;
        } else {
          /* cb2 is disabled or is in pulse mode or is
           * outputting high.
           */
          via_cb2h = 1;
        }
        return;
      l_0x0d:
        /* interrupt flag register */

        via_ifr &= ~(data & 0x7f);
        int_update ();
        return;
      l_0x0e:
        /* interrupt enable register */

        if (data & 0x80) {
          via_ier |= data & 0x7f;
        } else {
          via_ier &= ~(data & 0x7f);
        }
        int_update ();
        return;
      //}
    }
  }
}

void vecx_reset (void)
{
  unsigned r;
  int index;

  /* ram */

  for (r = 0; r < 1024; r++) {
    ram[r] = r & 0xff;
  }

  for (r = 0; r < 16; r++) {
    e8910_write(r, 0);
  }

  /* input buttons */

  e8910_write(14, 0xff);

  snd_select = 0;

  via_ora = 0;
  via_orb = 0;
  via_ddra = 0;
  via_ddrb = 0;
  via_t1on = 0;
  via_t1int = 0;
  via_t1c = 0;
  via_t1ll = 0;
  via_t1lh = 0;
  via_t1pb7 = 0x80;
  via_t2on = 0;
  via_t2int = 0; 
  via_t2c = 0;
  via_t2ll = 0;
  via_sr = 0;
  via_srb = 8;
  via_src = 0;
  via_srclk = 0;
  via_acr = 0;
  via_pcr = 0;
  via_ifr = 0;
  via_ier = 0;
  via_ca2 = 1;
  via_cb2h = 1;
  via_cb2s = 0;

  alg_rsh = 128;
  alg_xsh = 128;
  alg_ysh = 128;
  alg_zsh = 0;
  alg_jch0 = 128;
  alg_jch1 = 128;
  alg_jch2 = 128;
  alg_jch3 = 128;
  alg_jsh = 128;

  alg_compare = 0; /* check this */

  alg_dx = 0;
  alg_dy = 0;
  alg_curr.x = ALG_MAX_X / 2;
  alg_curr.y = ALG_MAX_Y / 2;

  alg_vectoring = 0;

  vector_draw_cnt = 0;
  vector_erse_cnt = 0;

  vectors_draw = vectors_set;
  vectors_erse = vectors_set + VECTOR_CNT;
  memset( vectors_set, 0, sizeof(vectors_set));

  for (index = 0; index < VECTOR_CNT*2; index++) {
    vectors_set[index].color = VECTREX_COLORS;
  }

  e6809_reset ();

  osint_video_reset();
}

int
vecx_save_state(char* filename)
{
  vecx_save_t V;

  gzFile* a_f = gzopen(filename, "w");
  if (! a_f) return 1;

  /* ram */
  gzwrite( a_f, rom     , sizeof(rom));
  gzwrite( a_f, cart    , sizeof(cart));
  gzwrite( a_f, ram     , sizeof(ram));

  V.snd_select = snd_select;
  V.via_ora    = via_ora;
  V.via_orb    = via_orb;
  V.via_ddra   = via_ddra;
  V.via_ddrb   = via_ddrb;
  V.via_t1on   = via_t1on;
  V.via_t1int  = via_t1int;
  V.via_t1c    = via_t1c;
  V.via_t1ll   = via_t1ll;
  V.via_t1lh   = via_t1lh;
  V.via_t1pb7  = via_t1pb7;
  V.via_t2on   = via_t2on;
  V.via_t2int  = via_t2int;
  V.via_t2c    = via_t2c;
  V.via_t2ll   = via_t2ll;
  V.via_sr     = via_sr;
  V.via_srb    = via_srb;
  V.via_src    = via_src;
  V.via_srclk  = via_srclk;
  V.via_acr    = via_acr;
  V.via_pcr    = via_pcr;
  V.via_ifr    = via_ifr;
  V.via_ier    = via_ier;
  V.via_ca2    = via_ca2;
  V.via_cb2h   = via_cb2h;
  V.via_cb2s   = via_cb2s;
  V.alg_rsh    = alg_rsh;
  V.alg_xsh    = alg_xsh;
  V.alg_ysh    = alg_ysh;
  V.alg_zsh    = alg_zsh;
  V.alg_jch0   = alg_jch0;
  V.alg_jch1   = alg_jch1;
  V.alg_jch2   = alg_jch2;
  V.alg_jch3   = alg_jch3;
  V.alg_jsh    = alg_jsh;

  V.alg_compare = alg_compare;
  V.alg_dx      = alg_dx;
  V.alg_dy      = alg_dy;
  V.alg_curr    = alg_curr;
  V.alg_vectoring = alg_vectoring;
  V.alg_vector0 = alg_vector0;
  V.alg_vector1 = alg_vector1;
  V.alg_vector_dx = alg_vector_dx;
  V.alg_vector_dy = alg_vector_dy;
  V.alg_vector_color = alg_vector_color;
  V.vector_draw_cnt = vector_draw_cnt;
  V.vector_erse_cnt = vector_erse_cnt;

  gzwrite( a_f, &V, sizeof(V));

  gzwrite( a_f, &snd_regs   , sizeof(snd_regs));
  gzwrite( a_f, &vectors_set, sizeof(vectors_set));

  long vectors_draw_offset = (vectors_draw - vectors_set) / sizeof(vector_t);
  gzwrite( a_f, &vectors_draw_offset, sizeof(vectors_draw_offset));

  e6809_save_t R;
  e6809_dump(&R);
  gzwrite( a_f, &R, sizeof(R));

  gzclose( a_f );
  return 0;
}

int
vecx_load_state(char* filename)
{
  vecx_save_t V;

  gzFile* a_f = gzopen(filename, "r");
  if (! a_f) return 1;

  /* ram */
  gzread( a_f, rom     , sizeof(rom));
  gzread( a_f, cart    , sizeof(cart));
  gzread( a_f, ram     , sizeof(ram));

  gzread( a_f, &V, sizeof(V));

  snd_select = V.snd_select;
  via_ora    = V.via_ora;
  via_orb    = V.via_orb;
  via_ddra   = V.via_ddra;
  via_ddrb   = V.via_ddrb;
  via_t1on   = V.via_t1on;
  via_t1int  = V.via_t1int;
  via_t1c    = V.via_t1c;
  via_t1ll   = V.via_t1ll;
  via_t1lh   = V.via_t1lh;
  via_t1pb7  = V.via_t1pb7;
  via_t2on   = V.via_t2on;
  via_t2int  = V.via_t2int;
  via_t2c    = V.via_t2c;
  via_t2ll   = V.via_t2ll;
  via_sr     = V.via_sr;
  via_srb    = V.via_srb;
  via_src    = V.via_src;
  via_srclk  = V.via_srclk;
  via_acr    = V.via_acr;
  via_pcr    = V.via_pcr;
  via_ifr    = V.via_ifr;
  via_ier    = V.via_ier;
  via_ca2    = V.via_ca2;
  via_cb2h   = V.via_cb2h;
  via_cb2s   = V.via_cb2s;
  alg_rsh    = V.alg_rsh;
  alg_xsh    = V.alg_xsh;
  alg_ysh    = V.alg_ysh;
  alg_zsh    = V.alg_zsh;
  alg_jch0   = V.alg_jch0;
  alg_jch1   = V.alg_jch1;
  alg_jch2   = V.alg_jch2;
  alg_jch3   = V.alg_jch3;
  alg_jsh    = V.alg_jsh;

  alg_compare = V.alg_compare;
  alg_dx      = V.alg_dx;
  alg_dy      = V.alg_dy;
  alg_curr    = V.alg_curr;
  alg_vectoring = V.alg_vectoring;
  alg_vector0 = V.alg_vector0;
  alg_vector1 = V.alg_vector1;
  alg_vector_dx = V.alg_vector_dx;
  alg_vector_dy = V.alg_vector_dy;
  alg_vector_color = V.alg_vector_color;
  vector_draw_cnt = V.vector_draw_cnt;
  vector_erse_cnt = V.vector_erse_cnt;

  gzread( a_f, &snd_regs, sizeof(snd_regs));
  gzread( a_f, &vectors_set, sizeof(vectors_set));

  long vectors_draw_offset;
  gzread( a_f, &vectors_draw_offset, sizeof(vectors_draw_offset));
  if (vectors_draw_offset) {
    vectors_draw = vectors_set + VECTOR_CNT;
    vectors_erse = vectors_set;
  } else {
    vectors_draw = vectors_set;
    vectors_erse = vectors_set + VECTOR_CNT;
  }

  e6809_save_t R;
  gzread( a_f, &R, sizeof(R));
  e6809_restore(&R);

  osint_video_reset();

  gzclose( a_f );
  return 0;
}

/* perform a single cycle worth of via emulation.
 * via_sstep0 is the first postion of the emulation.
 */

static void inline via_sstep0 (void)
{
  unsigned char t2shift;
  if (via_t1on) {
    if (! via_t1c--) {
      /* counter just rolled over */

      if (via_acr & 0x40) {
        /* continuous interrupt mode */

        via_ifr |= 0x40;
        int_update ();
        via_t1pb7 = 0x80 - via_t1pb7;

        /* reload counter */

        via_t1c = (via_t1lh << 8) | via_t1ll;
      } else {
        /* one shot mode */

        if (via_t1int) {
          via_ifr |= 0x40;
          int_update ();
          via_t1pb7 = 0x80;
          via_t1int = 0;
        }
      }
    }
  }

  if (via_t2on && (via_acr & 0x20) == 0x00) {
    if (! via_t2c--) {
      /* one shot mode */
      if (via_t2int) {
        via_ifr |= 0x20;
        int_update ();
        via_t2int = 0;
      }
    }
  }

  /* shift counter */
  if (! via_src--) {
    via_src   = via_t2ll;
    t2shift   = via_srclk;
    via_srclk = ! via_srclk;

  } else {
    t2shift = 0;
  }
  if (via_srb & 8) return;

  unsigned char tmp = via_acr & 0x1c;
  if (tmp == 0x18) {
    /* shift out under system clock control */
    via_cb2s = (via_sr & 0x80) != 0;
    via_sr   = (via_sr << 1)|via_cb2s;
    if (++via_srb == 8) {
      via_ifr |= 0x04;
      int_update ();
    }
    return;
  }
  if (tmp == 0x08) {
    /* shift in under system clk control */
    via_sr <<= 1;
    if (++via_srb == 8) {
      via_ifr |= 0x04;
      int_update ();
    }
    return;
  }
  if (t2shift) {
    if (tmp & 0x10) {
      via_cb2s = (via_sr & 0x80) != 0;
      via_sr   = (via_sr << 1)|via_cb2s;
    }
    if (tmp & 0x04) {
      if (++via_srb == 8) {
        via_ifr |= 0x04;
        int_update ();
      }
    }
  }
}

/* perform the second part of the via emulation */

static void inline via_sstep1 (void)
{
  if ((via_pcr & 0x0e) == 0x0a) {
    /* if ca2 is in pulse mode, then make sure
     * it gets restored to '1' after the pulse.
     */
    via_ca2 = 1;
  }

  if ((via_pcr & 0xe0) == 0xa0) {
    /* if cb2 is in pulse mode, then make sure
     * it gets restored to '1' after the pulse.
     */
    via_cb2h = 1;
  }
}

static inline void alg_addline (point_t* p0, point_t* p1, unsigned char color)
{
  vector_t* a_vector = &vectors_draw[vector_draw_cnt];
  a_vector->p0 = *p0;
  a_vector->p1 = *p1;
  a_vector->color = color;
  vector_draw_cnt = (vector_draw_cnt + 1) & 0xffff;
}

/* perform a single cycle worth of analog emulation */

static void inline alg_sstep (void)
{
  long sig_dx, sig_dy;
  unsigned sig_ramp;

  if (via_ca2 == 0) {
    /* need to force the current point to the 'orgin' so just
     * calculate distance to origin and use that as dx,dy.
     */
    sig_dx = ALG_MAX_X / 2 - alg_curr.x;
    sig_dy = ALG_MAX_Y / 2 - alg_curr.y;
  } else {
    if (via_acr & 0x80) {
      sig_ramp = via_t1pb7;
    } else {
      sig_ramp = via_orb & 0x80;
    }

    if (sig_ramp == 0) {
      sig_dx = alg_dx;
      sig_dy = alg_dy;
    } else {
      sig_dx = 0;
      sig_dy = 0;
    }
  }

  if (! alg_vectoring) {

    unsigned sig_blank;
    if ((via_acr & 0x10) == 0x10) {
      sig_blank = via_cb2s;
    } else {
      sig_blank = via_cb2h;
    }

    if (sig_blank == 1 &&
      alg_curr.x >= 0 && alg_curr.x < ALG_MAX_X &&
      alg_curr.y >= 0 && alg_curr.y < ALG_MAX_Y) 
    {
      /* start a new vector */

      alg_vectoring = 1;
      alg_vector0 = alg_curr;
      alg_vector1 = alg_curr;
      alg_vector_dx = sig_dx;
      alg_vector_dy = sig_dy;
      alg_vector_color = (unsigned char) alg_zsh;
    }

  } else {

    unsigned sig_blank;
    if ((via_acr & 0x10) == 0x10) {
      sig_blank = via_cb2s;
    } else {
      sig_blank = via_cb2h;
    }

    /* already drawing a vector ... check if we need to turn it off */

    if (sig_blank == 0) {
      /* blank just went on, vectoring turns off, and we've got a
       * new line.
       */
      alg_vectoring = 0;
      alg_addline (&alg_vector0, &alg_vector1, alg_vector_color);

    } else if (sig_dx != alg_vector_dx ||
           sig_dy != alg_vector_dy ||
           (unsigned char) alg_zsh != alg_vector_color) {

      /* the parameters of the vectoring processing has changed.
       * so end the current line.
       */
      alg_addline (&alg_vector0, &alg_vector1, alg_vector_color);

      /* we continue vectoring with a new set of parameters if the
       * current point is not out of limits.
       */

      if (alg_curr.x >= 0 && alg_curr.x < ALG_MAX_X &&
          alg_curr.y >= 0 && alg_curr.y < ALG_MAX_Y) {
        alg_vector0 = alg_curr;
        alg_vector1 = alg_curr;
        alg_vector_dx = sig_dx;
        alg_vector_dy = sig_dy;
        alg_vector_color = (unsigned char) alg_zsh;
      } else {
        alg_vectoring = 0;
      }
    }
  }

  alg_curr.x += sig_dx;
  alg_curr.y += sig_dy;

  if (alg_vectoring &&
    alg_curr.x >= 0 && alg_curr.x < ALG_MAX_X &&
    alg_curr.y >= 0 && alg_curr.y < ALG_MAX_Y) 
  {

    /* we're vectoring ... current point is still within limits so
     * extend the current vector.
     */

    alg_vector1 = alg_curr;
  }
}

void vecx_emu_loop ()
{
  int c, icycles;
  int fcycles = FCYCLES_INIT;

  while (1) {

    icycles = e6809_sstep (via_ifr & 0x80);
    fcycles -= icycles;

    while (--icycles >= 0) {
      via_sstep0 ();
      alg_sstep ();
      via_sstep1 ();
    }

    if (fcycles < 0) {

      fcycles += FCYCLES_INIT;
      psp_update_keys();
      dve_render();

      /* everything that was drawn during this pass now now enters
       * the erase list for the next pass.
       */
      vector_erse_cnt = vector_draw_cnt;
      vector_draw_cnt = 0;

      vector_t* tmp = vectors_erse;
      vectors_erse = vectors_draw;
      vectors_draw = tmp;
    }
  }
}
