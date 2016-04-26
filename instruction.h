#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <cstdint>

enum OpType {
    ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI,
    BNE, BPL, BRK, BVC, BVS, CLC, CLD, CLI,
    CLV, CMP, CPX, CPY, DEC, DEX, DEY, EOR,
    INC, INX, INY, JMP, JSR, LDA, LDX, LDY,
    LSR, NOP, ORA, PHA, PHP, PLA, PLP, ROL,
    ROR, RTI, RTS, SBC, SEC, SED, SEI, STA,
    STX, STY, TAX, TAY, TYA, TSX, TXA, TXS,
    BAD_TYPE
};

enum AddrMode {
    ACCUMULATOR, IMMEDIATE, ZEROPAGE, ZEROPAGE_X, ZEROPAGE_Y,
    ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, IMPLIED, RELATIVE, INDIRECT, INDIRECT_X, INDIRECT_Y,
    BAD_MODE
};

struct Instruction {
    Instruction(uint8_t opcode, OpType type, AddrMode mode) :
        opcode(opcode), type(type), mode(mode), size(modeSizes[mode]) { }
    uint8_t opcode;
    OpType type;
    AddrMode mode;
    uint8_t size;
private:
    constexpr static uint8_t modeSizes[14] = {1, 2, 2, 2, 2, 3, 3, 3, 1, 2, 3, 2, 2, 0};
};

extern const Instruction decode_instruction[256];

#endif
