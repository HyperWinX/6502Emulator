#include <cstdint>
#include <cstdbool>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <cstring>
#include "opcodes.hpp"

#define MAX_MEM 1024 * 64

namespace EMU6502{
	struct Memory;
	struct CPU;
}

struct EMU6502::Memory{
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
}__attribute__((packed));

struct EMU6502::CPU{
    public:
        uint16_t PC, SP;
        uint8_t X, Y, A;
        bool C, Z, I, D, B, V, N;
        int32_t TotalCycles;
        Memory Mem;
	    int Execute(int32_t Cycles);
        int Reset(){
            PC = 0xFFFC;
            SP = 0x01FF;
            A = X = Y = 0;
            C = Z = I = D = B = V = N = 0;
            if (!Mem.Initialize()) return 0;
            return 1;
        }
        void ADCSetStatusFlags(uint8_t Byte){
            if (A + Byte + (C ? 1 : 0) > 127)
                C = 1;
            else C = 0;
            if (A == 0) Z = 1;
            if (A & 0b10000000) N = 1;
        }
        void LDASetStatusFlags(){
            Z = (A == 0);
            N = (A & 0b10000000) > 0;
        }
        void LDXSetStatusFlags(){
            Z = (A == 0);
            N = (A & 0b10000000) > 0;
        }
        void LDYSetStatusFlags(){
            Z = (A == 0);
            N = (A & 0b10000000) > 0;
        }
        void ORASetStatusFlags(){
            Z = (A == 0);
            N = (A & 0b10000000) > 0;
        }
        void PLASetStatusFlags(){
            Z = (A == 0);
            N = (A & 0b10000000) > 0;
        }
        void StateDump(){
            printf("A=0x%x X=0x%x Y=0x%x\nPC=0x%x SP=0x%x\nC=%c Z=%c I=%c\nD=%c B=%c V=%c\n    N=%c\n",
                    A, X, Y, PC, SP,
                    C ? '1' : '0',
                    Z ? '1' : '0',
                    I ? '1' : '0',
                    D ? '1' : '0',
                    B ? '1' : '0',
                    V ? '1' : '0',
                    N ? '1' : '0');
        }
        void SaveDump(){
            std::ofstream dump("dump.dmp");
            dump << A; //Save registers
            dump << X;
            dump << Y;
            dump << PC;
            dump << SP;
            dump << C; //Save flags;
            dump << Z;
            dump << I;
            dump << D;
            dump << B;
            dump << V;
            dump << N;
            char buf[65536];
            memcpy(buf, Mem.MemoryPointer, 65536);
            dump << buf;
            dump.close();
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
	    void PushByteToStack(uint8_t Byte){
	    	Mem[SP--] = Byte;
	    }
	    uint8_t PopByteFromStack(){
	    	 return Mem[++SP];
	    }
	    void PushWordToStack(uint16_t Word){
	    	Mem[SP--] = Word & 0xFF;
	    	Mem[SP--] = (Word >> 8) & 0xFF;
	    }
	    uint16_t PopWordFromStack(){
		    uint16_t Word = 0;
		    Word = (Mem[++SP] << 8);
		    Word |= Mem[++SP] & 0xFF;
		    return Word;
	    }
        uint16_t GetIndirectXAddress(){
		    return ReadWord(FetchWord(PC) + X);
	    }
	    uint16_t GetIndirectYAddress(){
		    return ReadWord(FetchWord(PC)) + Y;
	    }
	    uint8_t GetZeroPageAddress(){
	    	return FetchByte(PC);
	    }
	    uint8_t GetZeroPageXAddress(){
	    	return FetchByte(PC) + X;
	    }
	    uint8_t GetZeroPageYAddress(){
	    	return FetchByte(PC) + Y;
	    }
        uint16_t GetAbsoluteAddress(){
            return FetchWord(PC);
        }
        uint16_t GetAbsoluteXAddress(){
            return FetchWord(PC) + X;
        }
        uint16_t GetAbsoluteYAddress(){
            return FetchWord(PC) + Y;
        }
}__attribute__((packed));
