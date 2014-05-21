#ifndef __E6809_H
#define __E6809_H

/* user defined read and write functions */
  typedef struct e6809_save_t {
    unsigned reg_x;
    unsigned reg_y;
    unsigned reg_u;
    unsigned reg_s;
    unsigned reg_pc;
    unsigned reg_a;
    unsigned reg_b;
    unsigned reg_dp;
    unsigned reg_cc;
    unsigned irq_status;

  } e6809_save_t;

extern unsigned char e6809_read8(unsigned short address);
extern void e6809_write8(unsigned short address, unsigned char data);

void e6809_reset (void);
unsigned e6809_sstep (unsigned irq_i);

extern void e6809_dump(e6809_save_t* a_state);
extern void e6809_restore(e6809_save_t* a_state);

#endif
