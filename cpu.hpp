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
            SP = 0x01FF;
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
                    // ADC instruction
                    case ADC_IM:{
                        A = FetchByte(PC);
                        LDASetStatusFlags();
                        break;
                    }
                    case ADC_ZP:{
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        A = ReadByte(ZeroPageAddress);
                        LDASetStatusFlags();
                        break;
                    }
                    case ADC_ZPX:{
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        ZeroPageAddress += X;
                        TotalCycles--;
                        A = ReadByte(ZeroPageAddress);
                        LDASetStatusFlags();
                        break;
                    }
                    case ADC_ABS:{
                        uint16_t Address = FetchWord(PC);
                        A = ReadByte(Address);
                        LDASetStatusFlags();
                        break;
                    }
                    case ADC_ABSX:{
                        uint16_t Address = FetchWord(PC);
                        A = ReadByte(Address + X);
                        LDASetStatusFlags();
                        break;
                    }
                    case ADC_ABSY:{
                        uint16_t Address = FetchWord(PC);
                        A = ReadByte(Address + Y);
                        LDASetStatusFlags();
                        break;
                    }
                    case ADC_INDX:{
                        uint16_t AddressToFetch = FetchWord(PC);
                        uint16_t AddressWithValue = ReadWord(AddressToFetch + X);
                        A = ReadByte(AddressWithValue);
                        LDASetStatusFlags();
                        break;
                    }
                    case ADC_INDY:{
                        uint16_t AddressToFetch = FetchWord(PC);
                        uint16_t AddressWithValue = ReadWord(AddressToFetch);
                        A = ReadByte(AddressWithValue + Y);
                        LDASetStatusFlags();
                        break;
                    }
                    // LDA instruction
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
                        LDASetStatusFlags();
                        break;
                    }
                    case LDA_INDY:{
                        uint16_t AddressToFetch = FetchWord(PC);
                        uint16_t AddressWithValue = ReadWord(AddressToFetch);
                        A = ReadByte(AddressWithValue + Y);
                        LDASetStatusFlags();
                        break;
                    }
                    // LDX instruction
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
                    // LDY instruction
                    case LDY_IM:{
                        Y = FetchByte(PC);
                        LDYSetStatusFlags();
                        break;
                    }
                    case LDY_ZP:{
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        Y = ReadByte(ZeroPageAddress);
                        LDYSetStatusFlags();
                        break;
                    }
                    case LDY_ZPX:{
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        ZeroPageAddress += X;
                        TotalCycles--;
                        Y = ReadByte(ZeroPageAddress);
                        LDYSetStatusFlags();
                        break;
                    }
                    case LDY_ABS:{
                        uint16_t Address = FetchWord(PC);
                        Y = ReadByte(Address);
                        LDYSetStatusFlags();
                        break;
                    }
                    case LDY_ABSX:{
                        uint16_t Address = FetchWord(PC);
                        Y = ReadByte(Address + X);
                        LDYSetStatusFlags();
                        break;
                    }
                    case NOP:{
                        TotalCycles--;
                        break;
                    }
                    // ORA instruction
                    case ORA_IM:{
                        A |= FetchByte(PC);
                        ORASetStatusFlags();
                        break;
                    }
                    case ORA_ZP:{
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        A |= ReadByte(ZeroPageAddress);
                        ORASetStatusFlags();
                        break;
                    }
                    case ORA_ZPX:{
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        ZeroPageAddress += X;
                        TotalCycles--;
                        A |= ReadByte(ZeroPageAddress);
                        ORASetStatusFlags();
                        break;
                    }
                    case ORA_ABS:{
                        uint16_t Address = FetchWord(PC);
                        A |= ReadByte(Address);
                        ORASetStatusFlags();
                        break;
                    }
                    case ORA_ABSX:{
                        uint16_t Address = FetchWord(PC);
                        A |= ReadByte(Address + X);
                        ORASetStatusFlags();
                        break;
                    }
                    case ORA_ABSY:{
                        uint16_t Address = FetchWord(PC);
                        A |= ReadByte(Address + Y);
                        ORASetStatusFlags();
                        break;
                    }
                    case ORA_INDX:{
                        uint16_t AddressToFetch = FetchWord(PC);
                        uint16_t AddressWithValue = ReadWord(AddressToFetch + X);
                        A |= ReadByte(AddressWithValue);
                        ORASetStatusFlags();
                        break;
                    }
                    case ORA_INDY:{
                        uint16_t AddressToFetch = FetchWord(PC);
                        uint16_t AddressWithValue = ReadWord(AddressToFetch);
                        A |= ReadByte(AddressWithValue + Y);
                        ORASetStatusFlags();
                        break;
                    }
                    // PHA instruction
                    case PHA:{
                        Mem[SP--] = A;
                        TotalCycles -= 2;
                        break;
                    }
                    // PHP instruction
                    case PHP:{
                        uint8_t Byte = 0b00000100;
                        if (C) Byte |= 0b10000000;
                        if (Z) Byte |= 0b01000000;
                        if (I) Byte |= 0b00100000;
                        if (D) Byte |= 0b00010000;
                        if (B) Byte |= 0b00001000;
                        if (V) Byte |= 0b00000010;
                        if (N) Byte |= 0b00000001;
                        Mem[SP--] = Byte;
                        TotalCycles -= 2;
                        break;
                    }
                    // PLA instruction
                    case PLA:{
                        A = Mem[++SP];
                        TotalCycles -= 3;
                        PLASetStatusFlags();
                        break;
                    }
                    // PLP instruction
                    case PLP:{
                        uint8_t Byte = Mem[++SP];
                        if (Byte & 0b10000000) C = 1;
                        if (Byte & 0b01000000) Z = 1;
                        if (Byte & 0b00100000) I = 1;
                        if (Byte & 0b00010000) D = 1;
                        if (Byte & 0b00001000) B = 1;
                        if (Byte & 0b00000010) V = 1;
                        if (Byte & 0b00000001) N = 1;
                        TotalCycles -= 3;
                        break;
                    }
                    // ROL instruction
                    case ROL_AC:{
                        uint8_t tmp_carry = C;
                        if (A & 0b10000000) C = 1;
                        A = (A << 1);
                        if (tmp_carry) A++;
                        if (!A) Z = 1;
                        TotalCycles--;
                        break;
                    }
                    case ROL_ZP:{
                        uint8_t tmp_carry = C;
                        uint16_t ZeroPageAddress = FetchByte(PC);
                        uint8_t Value = Mem[ZeroPageAddress];
                        if (Value & 0b1000000) C = 1;
                        Value = (Value << 1);
                        if (tmp_carry) Value++;
                        Mem[ZeroPageAddress] = Value;
                        TotalCycles -= 4;
                        break;
                    }
                    case ROL_ZPX:{
                        uint8_t tmp_carry = C;
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        uint8_t Value = Mem[ZeroPageAddress + X];
                        if (Value & 0b1000000) C = 1;
                        Value = (Value << 1);
                        if (tmp_carry) Value++;
                        Mem[ZeroPageAddress + X] = Value;
                        TotalCycles -= 4;
                        break;
                    }
                    case ROL_ABS:{
                        uint8_t tmp_carry = C;
                        uint16_t Address = FetchWord(PC);
                        uint8_t Value = Mem[Address];
                        if (Value & 0b1000000) C = 1;
                        Value = (Value << 1);
                        if (tmp_carry) Value++;
                        Mem[Address] = Value;
                        TotalCycles -= 5;
                        break;
                    }
                    case ROL_ABSX:{
                        uint8_t tmp_carry = C;
                        uint16_t Address = FetchWord(PC);
                        uint8_t Value = Mem[Address + X];
                        if (Value & 0b1000000) C = 1;
                        Value = (Value << 1);
                        if (tmp_carry) Value++;
                        Mem[Address + X] = Value;
                        TotalCycles -= 6;
                        break;
                    }
                    // ROR insruction
                    case ROR_AC:{
                        uint8_t tmp_carry = C;
                        if (A & 1) C = 1;
                        A = (A >> 1);
                        if (tmp_carry) A |= 0b10000000;
                        if (!A) Z = 1;
                        TotalCycles--;
                        break;
                    }
                    case ROR_ZP:{
                        uint8_t tmp_carry = C;
                        uint16_t ZeroPageAddress = FetchByte(PC);
                        uint8_t Value = Mem[ZeroPageAddress];
                        if (Value & 1) C = 1;
                        Value = (Value >> 1);
                        if (tmp_carry) Value |= 0b10000000;
                        Mem[ZeroPageAddress] = Value;
                        TotalCycles -= 4;
                        break;
                    }
                    case ROR_ZPX:{
                        uint8_t tmp_carry = C;
                        uint8_t ZeroPageAddress = FetchByte(PC);
                        uint8_t Value = Mem[ZeroPageAddress + X];
                        if (Value & 1) C = 1;
                        Value = (Value >> 1);
                        if (tmp_carry) Value |= 0b10000000;
                        Mem[ZeroPageAddress + X] = Value;
                        TotalCycles -= 4;
                        break;
                    }
                    case ROR_ABS:{
                        uint8_t tmp_carry = C;
                        uint16_t Address = FetchWord(PC);
                        uint8_t Value = Mem[Address];
                        if (Value & 1) C = 1;
                        Value = (Value >> 1);
                        if (tmp_carry) Value |= 0b10000000;
                        Mem[Address] = Value;
                        TotalCycles -= 5;
                        break;
                    }
                    case ROR_ABSX:{
                        uint8_t tmp_carry = C;
                        uint16_t Address = FetchWord(PC);
                        uint8_t Value = Mem[Address + X];
                        if (Value & 1) C = 1;
                        Value = (Value >> 1);
                        if (tmp_carry) Value |= 0b10000000;
                        Mem[Address + X] = Value;
                        TotalCycles -= 6;
                        break;
                    }
                    // JSR and RTS instructions
                    case JSR_ABS:{
                        uint16_t Address = FetchWord(PC);
                        SP -= 2;
                        WriteWord(PC - 3, SP + 1);
                        PC = Address;
                        TotalCycles--;
                        break;
                    }
                    case RTS:{
                        uint16_t Address = FetchWord(SP + 1);
                        SP += 2;
                        PC = Address;
                        TotalCycles -= 3;
                        break;
                    }
                    default:
                        printf("Unknown instruction! CPU halted.\n");
                        StateDump();
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