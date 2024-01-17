#include "cpu.hpp"
#include <cstdio>

int main(){
    EMU6502::CPU cpu;
    cpu.Reset();
    //Little program
    cpu.Mem[0xFFFC] = 2;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0224] = 0x04;
    cpu.Mem[0x0225] = 0x05;
    cpu.Mem[0x0506] = 0x69;
    cpu.Y = 2;
    cpu.Execute(6);
    printf("%x\n", cpu.A);
}