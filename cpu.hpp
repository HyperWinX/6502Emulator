#include <cstdint>
#include <cstdbool>
#include <cstdlib>
#include <cstdio>
#include "opcodes.hpp"

#define MAX_MEM 1024 * 64

class Memory{
    public:
        void* MemoryPointer = 0;
        int Initialize(){
            if (MemoryPointer) free(MemoryPointer);
            void* ptr = calloc(sizeof(uint8_t), MAX_MEM);
            if (!ptr) return 0;
            MemoryPointer = ptr;
            return 1;
        }
        uint8_t operator[](uint32_t address) const{
            return ((uint8_t*)MemoryPointer)[address];
        }
        uint8_t& operator[](uint32_t address){
            return ((uint8_t*)MemoryPointer)[address];
        }
};

class CPU{
    public:
        uint16_t PC, SP;
        uint8_t X, Y, A;
        bool C, Z, I, D, B, V, N;
        int32_t TotalCycles;
        Memory Mem;
        int Reset(){
            PC = 0xFFFC;
            SP = 0x0100;
            A = X = Y = 0;
            C = Z = I = D = B = V = N = 0;
            if (!Mem.Initialize()) return 0;
            return 1;
        }
        int Execute(int32_t Cycles){
            TotalCycles = Cycles;
            while (TotalCycles > 0){
                uint8_t instruction = FetchByte(PC);
                switch(instruction){
                    // LDA instructions
                    case LDA_IM:{
                        A = FetchByte(PC);
                        LDASetStatusFlags();
                        break;
                    }
                    case LDA_ZP:{
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        A = ReadByte(ZeroPageAddress);
                        LDASetStatusFlags();
                        break;
                    }
                    case LDA_ZPX:{
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        ZeroPageAddress += X;
                        TotalCycles--;
                        A = ReadByte(ZeroPageAddress);
                        LDASetStatusFlags();
                        break;
                    }
                    case LDA_ABS:{
                        uint16_t Address = FetchWord(PC);
                        A = ReadByte(Address);
                        LDASetStatusFlags();
                        break;
                    }
                    case LDA_ABSX:{
                        uint16_t Address = FetchWord(PC);
                        A = ReadByte(Address + X);
                        LDASetStatusFlags();
                        break;
                    }
                    case LDA_ABSY:{
                        uint16_t Address = FetchWord(PC);
                        A = ReadByte(Address + Y);
                        LDASetStatusFlags();
                        break;
                    }
                    case LDA_INDX:{
                        uint16_t AddressToFetch = FetchWord(PC);
                        uint16_t AddressWithValue = ReadWord(AddressToFetch + X);
                        A = ReadByte(AddressWithValue);
                        break;
                    }
                    case LDA_INDY:{
                        uint16_t AddressToFetch = FetchWord(PC);
                        uint16_t AddressWithValue = ReadWord(AddressToFetch);
                        A = ReadByte(AddressWithValue + Y);
                        break;
                    }
                    // LDX instructions
                    case LDX_IM:{
                        X = FetchByte(PC);
                        LDXSetStatusFlags();
                        break;
                    }
                    case LDX_ZP:{
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        X = ReadByte(ZeroPageAddress);
                        LDXSetStatusFlags();
                        break;
                    }
                    case LDX_ZPY:{
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        ZeroPageAddress += Y;
                        TotalCycles--;
                        X = ReadByte(ZeroPageAddress);
                        LDXSetStatusFlags();
                        break;
                    }
                    case LDX_ABS:{
                        uint16_t Address = FetchWord(PC);
                        X = ReadByte(Address);
                        LDXSetStatusFlags();
                        break;
                    }
                    case LDX_ABSY:{
                        uint16_t Address = FetchWord(PC);
                        X = ReadByte(Address + Y);
                        LDXSetStatusFlags();
                        break;
                    }
                    // LDY instructions
                    case LDY_IM:{
                        Y = FetchByte(PC);
                        LDXSetStatusFlags();
                        break;
                    }
                    case LDY_ZP:{
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        Y = ReadByte(ZeroPageAddress);
                        LDXSetStatusFlags();
                        break;
                    }
                    case LDY_ZPX:{
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        ZeroPageAddress += X;
                        TotalCycles--;
                        Y = ReadByte(ZeroPageAddress);
                        LDXSetStatusFlags();
                        break;
                    }
                    case LDY_ABS:{
                        uint16_t Address = FetchWord(PC);
                        Y = ReadByte(Address);
                        LDXSetStatusFlags();
                        break;
                    }
                    case LDY_ABSX:{
                        uint16_t Address = FetchWord(PC);
                        Y = ReadByte(Address + X);
                        LDXSetStatusFlags();
                        break;
                    }
                    case JSR_ABS:{
                        uint16_t Address = FetchWord(PC);
                        WriteWord(PC - 3, SP);
                        SP += 2;
                        PC = Address;
                        TotalCycles--;
                        break;
                    }
                    case RTS:{
                        uint16_t Address = FetchWord(SP - 2);
                        SP -= 2;
                        PC = Address;
                        TotalCycles -= 3;
                        break;
                    }
                    default:
                        printf("Unknown instruction! CPU halted.\n");
                        return 0;
                        break;
                }
            }
            return 1;
        }
        void LDASetStatusFlags(){
            Z = (A == 0);
            N = (A & 0b10000000) > 0;
        }
        void LDXSetStatusFlags(){
            Z = (A == 0);
            N = (A & 0b10000000) > 0;
        }
        void WriteWord(uint16_t Data, uint16_t Address){
            Mem[Address] = Data & 0xFF;
            Mem[Address + 1] = (Data >> 8);
            TotalCycles -= 2;
        }
        uint8_t FetchByte(uint16_t Address){
            uint8_t byte = Mem[Address];
            TotalCycles--;
            PC++;
            return byte;
        }
        uint16_t FetchWord(uint16_t Address){
            uint16_t Word = FetchByte(Address);
            Word |= (FetchByte(Address + 1) << 8);
            return Word;
        }
        uint8_t ReadByte(uint16_t Address){
            uint8_t byte = Mem[Address];
            TotalCycles--;
            return byte;
        }
        uint16_t ReadWord(uint16_t Address){
            uint16_t Word = ReadByte(Address);
            Word |= (ReadByte(Address + 1) << 8);
            return Word;
        }
};