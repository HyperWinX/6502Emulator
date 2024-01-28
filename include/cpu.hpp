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
        int Initialize();
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
        int initialized;
	    int Execute(int32_t Cycles);
        int Reset();
        void ADCSetStatusFlags(uint8_t Byte);
        void ANDSetStatusFlags();
        void DECSetStatusFlags(uint8_t Byte);
        void DEXSetStatusFlags();
        void DEYSetStatusFlags();
        void EORSetStatusFlags();
        void LDASetStatusFlags();
        void LDXSetStatusFlags();
        void LDYSetStatusFlags();
        void ORASetStatusFlags();
        void PLASetStatusFlags();
        void SBCSetStatusFlags(uint8_t Byte);
        void TAXSetStatusFlags();
        void TAYSetStatusFlags();
        void TSXSetStatusFlags();
        void TXASetStatusFlags();
        void TYASetStatusFlags();
        void StateDump();
        void SaveDump();
        void WriteByte(uint8_t Data, uint16_t Address);
        void WriteWord(uint16_t Data, uint16_t Address);
        uint8_t FetchByte(uint16_t Address);
        uint16_t FetchWord(uint16_t Address);
        uint8_t ReadByte(uint16_t Address);
        uint16_t ReadWord(uint16_t Address);
	    void PushByteToStack(uint8_t Byte);
	    uint8_t PopByteFromStack();
	    void PushWordToStack(uint16_t Word);
	    uint16_t PopWordFromStack();
        uint16_t GetIndirectXAddress();
	    uint16_t GetIndirectYAddress();
	    uint8_t GetZeroPageAddress();
	    uint8_t GetZeroPageXAddress();
	    uint8_t GetZeroPageYAddress();
        uint16_t GetAbsoluteAddress();
        uint16_t GetAbsoluteXAddress();
        uint16_t GetAbsoluteYAddress();
        ~CPU();
}__attribute__((packed));
