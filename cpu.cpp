#include <iostream>
#include <cstdio>
#include <cstdint>

#include "instruction.h"
#include "cpu.h"

bool CPU::get_bit(uint8_t val, int n)
{
    return 1 & val >> n;
}

bool CPU::get_flag(PFlag flag)
{
    return get_bit(P, static_cast<int>(flag));
}

void CPU::set_flag(PFlag flag, bool b)
{
    if (b) { P |= 1 << static_cast<int>(flag); }
    else { P &= ~(1 << static_cast<int>(flag)); }
}

void CPU::set_negative(uint8_t val)
{
    set_flag(PFlag::N, get_bit(val, 7));
}

void CPU::set_zero(uint8_t val)
{
    set_flag(PFlag::Z, val == 0);
}

uint8_t CPU::read_mem_val(uint16_t addr)
{
    if (addr < 0x2000) {
        uint16_t ram_addr = addr & 0x7FF;
        return memmap[ram_addr];
    } else {
        return memmap[addr];
    }
}

uint16_t CPU::read_mem_addr(uint16_t addr)
{
    if (addr < 0x2000) {
        uint16_t ram_addr = addr & 0x7FF; // Don't read from the upper 3 mirrors
        return memmap[ram_addr] | memmap[ram_addr + 1] << 8;
    } else {
        return memmap[addr] | memmap[addr + 1] << 8;
    }
}

void CPU::write_mem_val(uint16_t addr, uint8_t val)
{
    // Pre: addr < 0x2000
    // Ignore the 3 upper mirrors
    memmap[addr & 0x7FF] = val;
}

void CPU::push_val(uint8_t val)
{
    // SP is only 8 bits, and hardwired to page 0x01, so it never refers to any of the mirrors
    memmap[0x0100 + SP] = val;
    SP -= 1;
}

void CPU::push_addr(uint16_t addr)
{
    // SP is only 8 bits, and hardwired to page 0x01, so it never refers to any of the mirrors
    memmap[0x0100 + SP] = addr >> 8; // Higher
    memmap[0x0100 + SP-1] = addr & 0xFF; // Lower
    SP -= 2;
}

uint8_t CPU::pull_val()
{
    uint8_t val = read_mem_val(0x0100 + SP+1);
    SP += 1;
    return val;
}

uint16_t CPU::pull_addr()
{
    uint16_t addr = read_mem_addr(0x0100 + SP+1);
    SP += 2;
    return addr;
}

void CPU::run()
{
    while (true) run_next_instruction();
}

void CPU::reset()
{
    PC = read_mem_addr(0xFFFC);
}

void CPU::run_next_instruction()
{
    const Instruction ins = decode_instruction(memmap[PC]);
    uint8_t oper;
    uint16_t operAddr;
    uint8_t res;
    uint8_t prevPC = PC;

    switch (ins.mode)
    {
        case ACCUMULATOR:
            // No address
            oper = A;
            break;
        case IMMEDIATE:
            operAddr = PC+1;
            break;
        case ZEROPAGE:
            operAddr = read_mem_val(PC+1);
            break;
        case ZEROPAGE_X:
            operAddr = read_mem_val(PC+1) + X;
            break;
        case ZEROPAGE_Y:
            operAddr = read_mem_val(PC+1) + Y;
            break;
        case ABSOLUTE:
            operAddr = read_mem_addr(PC+1);
            break;
        case ABSOLUTE_X:
            operAddr = read_mem_addr(PC+1) + X;
            break;
        case ABSOLUTE_Y:
            operAddr = read_mem_addr(PC+1) + Y;
            break;
        case IMPLIED:
            break;
        case RELATIVE:
            // The only instructions that uses relative addressing are the branching
            // instructions, which exclusively use it.
            oper = read_mem_addr(PC+1);
            break;
        case INDIRECT:
            // The only instruction that uses absolute indirect addressing is JMP.
            operAddr = read_mem_addr((read_mem_addr(PC+1)));
            break;
        case INDIRECT_X: // Take the zero page address (val) add X reg then look up 2 byte address
            operAddr = read_mem_addr(read_mem_val(PC+1) + X);
            break;
        case INDIRECT_Y: // Look up zero page address, than add Y reg to address
            operAddr = read_mem_addr(read_mem_val(PC+1)) + Y;
            break;
    }

    PC += ins.size;

    switch (ins.type)
    {
        case ADC: {
            oper = read_mem_val(operAddr);
            uint16_t tmp = A + oper + get_flag(PFlag::C);
            res = tmp; // res is a uint8_t so it will be the lower 8 bits of tmp
            set_flag(PFlag::C, tmp >> 8);
            set_flag(PFlag::V, !get_bit(A^oper, 7) && get_bit(A^res, 7));
            set_zero(res);
            set_negative(res);
            break;
        }
        case AND:
            A &= read_mem_val(operAddr);
            set_zero(A);
            set_negative(A);
            break;
        case ASL:
            if (ins.mode != ACCUMULATOR) oper = read_mem_val(operAddr);
            set_flag(PFlag::C, get_bit(oper, 7));
            res = oper << 1;
            set_zero(res);
            set_negative(res);
            if (ins.mode == ACCUMULATOR) { A = res; }
            else { write_mem_val(operAddr, res); }
            break;
        case BCC:
            if (!get_flag(PFlag::C)) PC = PC + static_cast<int8_t>(oper);
            break;
        case BCS:
            if (!get_flag(PFlag::C)) PC = PC + static_cast<int8_t>(oper);
            break;
        case BEQ:
            if (get_flag(PFlag::Z)) PC = PC + static_cast<int8_t>(oper);
            break;
        case BIT: {
            uint8_t tmp = read_mem_val(operAddr);
            set_flag(PFlag::N, get_bit(tmp, 7));
            set_flag(PFlag::V, get_bit(tmp, 6));
            res = A & tmp;
            set_zero(res);
            break;
        }
        case BMI:
            if (get_flag(PFlag::N)) PC = PC + static_cast<int8_t>(oper);
            break;
        case BNE:
            if (!get_flag(PFlag::Z)) PC = PC + static_cast<int8_t>(oper);
            break;
        case BPL:
            if (!get_flag(PFlag::N)) PC = PC + static_cast<int8_t>(oper);
            break;
        case BRK:
            push_addr(prevPC+2);
            PC = read_mem_addr(0xFFFE);
            set_flag(PFlag::I, 1);
            break;
        case BVC:
            if (!get_flag(PFlag::V)) PC = PC + static_cast<int8_t>(oper);
            break;
        case BVS:
            if (get_flag(PFlag::V)) PC = PC + static_cast<int8_t>(oper);
            break;
        case CLC:
            set_flag(PFlag::C, 0);
            break;
        case CLD:
            set_flag(PFlag::D, 0);
            break;
        case CLI:
            set_flag(PFlag::I, 0);
            break;
        case CLV:
            set_flag(PFlag::V, 0);
            break;
        case CMP:
            oper = read_mem_val(operAddr);
            res = A - oper;
            set_flag(PFlag::C, A > oper);
            set_zero(res);
            set_negative(res);
            break;
        case CPX:
            oper = read_mem_val(operAddr);
            res = X - oper;
            set_flag(PFlag::C, X > oper);
            set_zero(res);
            set_negative(res);
            break;
        case CPY:
            oper = read_mem_val(operAddr);
            res = Y - oper;
            set_flag(PFlag::C, Y > oper);
            set_zero(res);
            set_negative(res);
            break;
        case DEC:
            res = read_mem_val(operAddr) - 1;
            write_mem_val(operAddr, res);
            set_zero(res);
            set_negative(res);
            break;
        case DEX:
            X -= 1;
            set_zero(X);
            set_negative(X);
            break;
        case DEY:
            Y -= 1;
            set_zero(Y);
            set_negative(Y);
            break;
        case EOR:
            A ^= read_mem_val(operAddr);
            set_zero(A);
            set_negative(A);
            break;
        case INC:
            res = read_mem_val(operAddr) + 1;
            write_mem_val(operAddr, res);
            set_zero(res);
            set_negative(res);
            break;
        case INX:
            X += 1;
            set_zero(X);
            set_negative(X);
            break;
        case INY:
            Y += 1;
            set_zero(Y);
            set_negative(Y);
            break;
        case JMP:
            PC = operAddr;
            break;
        case JSR:
            push_addr(prevPC+2);
            PC = operAddr;
            break;
        case LDA:
            A = read_mem_val(operAddr);
            set_zero(A);
            set_negative(A);
            break;
        case LDX:
            X = read_mem_val(operAddr);
            set_zero(X);
            set_negative(X);
            break;
        case LDY:
            Y = read_mem_val(operAddr);
            set_zero(Y);
            set_negative(Y);
            break;
        case LSR:
            if (ins.mode != ACCUMULATOR) oper = read_mem_val(operAddr);
            set_flag(PFlag::C, get_bit(oper, 0));
            res = oper >> 1;
            set_zero(res);
            set_negative(res); // Always resets
            if (ins.mode == ACCUMULATOR) { A = res; }
            else { write_mem_val(operAddr, res); }
            break;
        case NOP:
            break;
        case ORA:
            A |= read_mem_val(operAddr);
            set_zero(A);
            set_negative(A);
            break;
        case PHA:
            push_val(A);
            break;
        case PHP:
            push_val(P);
            push_val(res);
            break;
        case PLA:
            A = pull_val();
            set_zero(A);
            set_negative(A);
            break;
        case PLP:
            P = pull_val();
            break;
        case ROL:
            if (ins.mode != ACCUMULATOR) oper = read_mem_val(operAddr);
            res = oper << 1 | get_flag(PFlag::C);
            set_flag(PFlag::C, get_bit(oper, 7));
            set_zero(res);
            set_negative(res);
            if (ins.mode == ACCUMULATOR) { A = res; }
            else { write_mem_val(operAddr, res); }
            break;
        case ROR:
            if (ins.mode != ACCUMULATOR) oper = read_mem_val(operAddr);
            res = oper >> 1 | get_flag(PFlag::C) << 7;
            set_flag(PFlag::C, get_bit(oper, 0));
            set_zero(res);
            set_negative(res);
            if (ins.mode == ACCUMULATOR) { A = res; }
            else { write_mem_val(operAddr, res); }
            break;
        case RTI:
            P = pull_val();
            PC = pull_addr();
            break;
        case RTS:
            PC = pull_addr() + 1; // Add 1 to make up for JSR pushing the "wrong" address onto the stack
            break;
        case SBC: {
            oper = ~read_mem_val(operAddr);
            uint16_t tmp = A - oper - get_flag(PFlag::C);
            res = tmp; // res is a uint8_t so it will be the lower 8 bits of tmp
            set_flag(PFlag::C, tmp >> 8);
            set_flag(PFlag::V, !get_bit(A^oper, 7) && get_bit(A^res, 7)); 
            set_zero(res);
            set_negative(res);
            break;
        }
        case SEC:
            set_flag(PFlag::C, 1);
            break;
        case SED:
            set_flag(PFlag::D, 1);
            break;
        case SEI:
            set_flag(PFlag::I, 1);
            break;
        case STA:
            write_mem_val(operAddr, A);
            break;
        case STX:
            write_mem_val(operAddr, X);
            break;
        case STY:
            write_mem_val(operAddr, Y);
            break;
        case TAX:
            X = A;
            set_zero(X);
            set_negative(X);
            break;
        case TAY:
            Y = A;
            set_zero(Y);
            set_negative(Y);
            break;
        case TYA:
            A = Y;
            set_zero(A);
            set_negative(A);
            break;
        case TSX:
            X = SP;
            set_zero(X);
            set_negative(X);
            break;
        case TXA:
            A = X;
            set_zero(A);
            set_negative(A);
            break;
        case TXS:
            SP = X;
            break;
        default:
            std::cout << "Default op type (something went wrong)" << std::endl;
    }

}
