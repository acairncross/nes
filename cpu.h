#ifndef CPU_H
#define CPU_H
#include <cstdint>

enum class PFlag {C=0,Z=1,I=2,D=3,B=4,V=6,N=7};

struct CPU {
    uint16_t PC;
    uint8_t SP,A,X,Y,P; // N,V,B,D,I,Z,C;
    uint8_t memmap[0x10000];

    void run();
    void reset();
    void run_next_instruction();

    static bool get_bit(uint8_t, int n);
    bool get_flag(PFlag);
    void set_flag(PFlag, bool);
    void set_negative(uint8_t);
    void set_zero(uint8_t);
    uint8_t read_mem_val(uint16_t);
    uint16_t read_mem_addr(uint16_t);
    void write_mem_val(uint16_t, uint8_t);
    void push_val(uint8_t);
    void push_addr(uint16_t);
    uint8_t pull_val();
    uint16_t pull_addr();
};

#endif
