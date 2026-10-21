// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) kmg. Adapted from MAME nes_turbofile_device::read_exp/write.
// Original source, license and pinned revision are retained in this directory.
#pragma once
#include <stdint.h>
typedef struct {
    uint8_t ram[8192];
    uint16_t addr;
    uint8_t bit,latch,enabled,locked;
} WiizTurboFile;
static inline uint8_t turbofile_read(WiizTurboFile* t,unsigned port) {
    return t->enabled&&port==1?((t->ram[t->addr]>>t->bit)&1)<<2:0;
}
static inline void turbofile_write(WiizTurboFile* t,uint8_t data) {
    if(!t->enabled)return;
    if(!(data&2)){t->addr=0;t->bit=0;}
    if(!(data&4)&&(t->latch&4)) {
        if(!t->locked){t->ram[t->addr]&=~(1<<t->bit);t->ram[t->addr]|=(data&1)<<t->bit;}
        t->bit=(t->bit+1)&7;
        if(!t->bit)t->addr=(t->addr+1)&0x1fff;
    }
    t->latch=data;
}
