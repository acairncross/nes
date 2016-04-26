#include <bitset>
#include <fstream>
#include <iomanip>
#include <iostream>

#include <curses.h>
#include <signal.h>

#include "instruction.h"
#include "cpu.h"

void finish(int);
void update_view(CPU&, WINDOW*, WINDOW*, WINDOW*, WINDOW*, WINDOW*, WINDOW*);

const char* optype_str[] = {
    "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI",
    "BNE", "BPL", "BRK", "BVC", "BVS", "CLC", "CLD", "CLI",
    "CLV", "CMP", "CPX", "CPY", "DEC", "DEX", "DEY", "EOR",
    "INC", "INX", "INY", "JMP", "JSR", "LDA", "LDX", "LDY",
    "LSR", "NOP", "ORA", "PHA", "PHP", "PLA", "PLP", "ROL",
    "ROR", "RTI", "RTS", "SBC", "SEC", "SED", "SEI", "STA",
    "STX", "STY", "TAX", "TAY", "TYA", "TSX", "TXA", "TXS",
    "---"
};

const char* addrmode_str[] = {
    "ACCUMULATOR", "IMMEDIATE", "ZEROPAGE", "ZEROPAGE_X", "ZEROPAGE_Y",
    "ABSOLUTE", "ABSOLUTE_X", "ABSOLUTE_Y", "IMPLIED", "RELATIVE", "INDIRECT", "INDIRECT_X", "INDIRECT_Y",
    "--------"
};

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cout << "Usage: nes <iNES ROM file>" << std::endl;
        return EXIT_FAILURE;
    }

    CPU cpu;

    std::string filenameIn = argv[1];
    std::ifstream in(filenameIn);
    std::cout << "Reading header..." << std::endl;
    in.seekg(4);
    uint8_t PRGROM_size = in.get();
    uint8_t CHRROM_size = in.get();
    in.seekg(16);
    in.read((char*)cpu.memmap+0x8000, 0x4000);
    in.read((char*)cpu.memmap+0xC000, 0x4000);
    in.close();

    cpu.reset();

    signal(SIGINT, finish);

    initscr();
    start_color();
    cbreak();
    noecho();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    refresh();

    WINDOW *mem_win;
    mem_win = newwin(23, 32, 0, 0);
    box(mem_win, 0, 0);
    mvwaddstr(mem_win, 0, 1, "Mem");
    // for (uint16_t i=0; i<=20; ++i) {
    //     uint16_t pc = cpu.PC + i - 10;
    //     if (pc == cpu.PC) { wattron(mem_win, COLOR_PAIR(1)); }
    //     mvwprintw(mem_win, i+1, 1, "%04x:\t%02x", pc, cpu.read_mem_val(pc));
    //     if (pc == cpu.PC) { wattroff(mem_win, COLOR_PAIR(1)); }
    // }
    // wrefresh(mem_win);

    WINDOW *a_win;
    a_win = newwin(3, 4, 0, 32);
    box(a_win, 0, 0);
    mvwaddstr(a_win, 0, 1, "A");
    // mvwprintw(a_win, 1, 1, "%02x", cpu.A);
    // wrefresh(a_win);

    WINDOW *x_win;
    x_win = newwin(3, 4, 3, 32);
    box(x_win, 0, 0);
    mvwaddstr(x_win, 0, 1, "X");
    // mvwprintw(x_win, 1, 1, "%02x", cpu.X);
    // wrefresh(x_win);

    WINDOW *y_win;
    y_win = newwin(3, 4, 6, 32);
    box(y_win, 0, 0);
    mvwaddstr(y_win, 0, 1, "Y");
    // mvwprintw(y_win, 1, 1, "%02x", cpu.Y);
    // wrefresh(y_win);

    WINDOW *p_win;
    p_win = newwin(4, 10, 9, 32);
    box(p_win, 0, 0);
    mvwaddstr(p_win, 0, 1, "P");
    // mvwprintw(p_win, 1, 1, std::bitset(cpu.P)"%02x", cpu.P);
    // wrefresh(p_win);

    WINDOW *sp_win;
    sp_win = newwin(3, 4, 13, 32);
    box(sp_win, 0, 0);
    mvwaddstr(sp_win, 0, 1, "SP");
    // mvwprintw(sp_win, 1, 1, "%02x", cpu.SP);
    // wrefresh(sp_win);

    update_view(cpu, mem_win, a_win, x_win, y_win, p_win, sp_win);

    for (;;) {
        if (getch() == ' ') {
            cpu.run_next_instruction();
            update_view(cpu, mem_win, a_win, x_win, y_win, p_win, sp_win);
        }
    }

    finish(EXIT_SUCCESS);
}

void update_view(CPU& cpu,
                 WINDOW *mem_win, WINDOW *a_win, WINDOW *x_win, WINDOW *y_win,
                 WINDOW *p_win, WINDOW *sp_win)
{
    for (uint16_t i=0; i<=20; ++i) {
        uint16_t pc = cpu.PC + i - 10;
        uint8_t insVal = cpu.read_mem_val(pc);
        Instruction ins = decode_instruction[insVal];
        if (pc == cpu.PC) { wattron(mem_win, COLOR_PAIR(1)); }
        mvwprintw(mem_win, i+1, 1, "%04x: %02x %s %-17s",
                                    pc, insVal, optype_str[ins.type], addrmode_str[ins.mode]);
        if (pc == cpu.PC) { wattroff(mem_win, COLOR_PAIR(1)); }
    }
    wrefresh(mem_win);
    mvwprintw(a_win, 1, 1, "%02x", cpu.A);
    wrefresh(a_win);
    mvwprintw(x_win, 1, 1, "%02x", cpu.X);
    wrefresh(x_win);
    mvwprintw(y_win, 1, 1, "%02x", cpu.Y);
    wrefresh(y_win);
    mvwprintw(p_win, 1, 1, std::bitset<8>(cpu.P).to_string().c_str(), cpu.P);
    mvwprintw(p_win, 2, 1, "NV-BDIZC");
    wrefresh(p_win);
    mvwprintw(sp_win, 1, 1, "%02x", cpu.SP);
    wrefresh(sp_win);
}

void finish(int sig)
{
    endwin();
    exit(EXIT_SUCCESS);
}
