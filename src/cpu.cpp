#include "cpu.hpp"

int EMU6502::Memory::Initialize(){
    if (MemoryPointer) free(MemoryPointer);
    void* ptr = calloc(sizeof(uint8_t), MAX_MEM);
    if (!ptr) return 0;
    MemoryPointer = ptr;
    return 1;
}

int EMU6502::CPU::Reset(){
    PC = 0xFFFC;
    SP = 0x01FF;
    A = X = Y = 0;
    C = Z = I = D = B = V = N = 0;
    TotalCycles = 0;
    initialized = 0;
    ExecutedInstructions = 0;
    if (!Mem.Initialize()) return 0;
    return 1;
}

void EMU6502::CPU::LoadROM(char* romfile){
    FILE* rom = fopen(romfile, "rb");
    if (!rom){
        puts("Can't open ROM file!");
        exit(1);
    }
    fread(Mem.MemoryPointer, sizeof(char), 0xFFFF, rom);
    fclose(rom);
}

void EMU6502::CPU::ADCSetStatusFlags(uint8_t Byte){
    if (A + Byte + (C ? 1 : 0) > 127)
        C = 1;
    else C = 0;
    if (A == 0) Z = 1;
    else Z = 0;
    if (A & 0b10000000) N = 1;
    else N = 0;
}

void EMU6502::CPU::ANDSetStatusFlags(){
    Z = (A == 0);
    N = (A & 0b10000000) > 0;
}

void EMU6502::CPU::DECSetStatusFlags(uint8_t Byte){
    Z = (Byte == 0);
    N = (Byte & 0b10000000) > 0;
}

void EMU6502::CPU::DEXSetStatusFlags(){
    Z = (X == 0);
    N = (X & 0b10000000) > 0;
}

void EMU6502::CPU::DEYSetStatusFlags(){
    Z = (Y == 0);
    N = (Y & 0b10000000) > 0;
}

void EMU6502::CPU::EORSetStatusFlags(){
    Z = (A == 0);
    N = (A & 0b10000000) > 0;
}

void EMU6502::CPU::LDASetStatusFlags(){
    Z = (A == 0);
    N = (A & 0b10000000) > 0;
}

void EMU6502::CPU::LDXSetStatusFlags(){
    Z = (A == 0);
    N = (A & 0b10000000) > 0;
}
void EMU6502::CPU::LDYSetStatusFlags(){
    Z = (A == 0);
    N = (A & 0b10000000) > 0;
}
void EMU6502::CPU::ORASetStatusFlags(){
    Z = (A == 0);
    N = (A & 0b10000000) > 0;
}
void EMU6502::CPU::PLASetStatusFlags(){
    Z = (A == 0);
    N = (A & 0b10000000) > 0;
}

void EMU6502::CPU::SBCSetStatusFlags(uint8_t Byte){
    if (A - (Byte + (C ? 1 : 0)) < -128)
        C = 1;
    else C = 0;
    if (A == 0) Z = 1;
    else Z = 0;
    if (A & 0b10000000) N = 1;
    else N = 0;
}
void EMU6502::CPU::TAXSetStatusFlags(){
    Z = !X;
    N = X & 0b10000000;
}
void EMU6502::CPU::TAYSetStatusFlags(){
    Z = !Y;
    N = Y & 0b10000000;
}
void EMU6502::CPU::TSXSetStatusFlags(){
    Z = !X;
    N = X & 0b10000000;
}
void EMU6502::CPU::TXASetStatusFlags(){
    Z = !A;
    N = A & 0b10000000;
}
void EMU6502::CPU::TYASetStatusFlags(){
    Z = !A;
    N = A & 0b10000000;
}

void EMU6502::CPU::StateDump(){
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
void EMU6502::CPU::SaveDump(){
    uint8_t buf[65536 + 3 + 4 + 7];
    memset(buf, 0x00, sizeof(buf));
    memcpy(buf, Mem.MemoryPointer, 65536);
    buf[65536] = A;
    buf[65537] = X;
    buf[65538] = Y;
    memcpy(buf + 65539, &SP, 2);
    memcpy(buf + 65541, &PC, 2);
    buf[65543] = C;
    buf[65544] = Z;
    buf[65545] = I;
    buf[65546] = D;
    buf[65547] = B;
    buf[65548] = V;
    buf[65549] = N;
    FILE* dump = fopen("emudump.dmp", "w");
    fwrite(buf, sizeof(buf), sizeof(uint8_t), dump);
    fclose(dump);
}
void EMU6502::CPU::WriteByte(uint8_t Data, uint16_t Address){
    Mem[Address] = Data;
    TotalCycles--;
}
void EMU6502::CPU::WriteWord(uint16_t Data, uint16_t Address){
    Mem[Address] = Data & 0xFF;
    Mem[Address + 1] = (Data >> 8);
    TotalCycles -= 2;
}
uint8_t EMU6502::CPU::FetchByte(uint16_t Address){
    uint8_t byte = Mem[Address];
    TotalCycles--;
    PC++;
    return byte;
}
uint16_t EMU6502::CPU::FetchWord(uint16_t Address){
    uint16_t Word = FetchByte(Address);
    Word |= (FetchByte(Address + 1) << 8);
    return Word;
}
uint8_t EMU6502::CPU::ReadByte(uint16_t Address){
    uint8_t byte = Mem[Address];
    TotalCycles--;
    return byte;
}
uint16_t EMU6502::CPU::ReadWord(uint16_t Address){
    uint16_t Word = ReadByte(Address);
    Word |= (ReadByte(Address + 1) << 8);
    return Word;
}
void EMU6502::CPU::PushByteToStack(uint8_t Byte){
    TotalCycles--;
    Mem[SP--] = Byte;
}
uint8_t EMU6502::CPU::PopByteFromStack(){
    TotalCycles--;
        return Mem[++SP];
}
void EMU6502::CPU::PushWordToStack(uint16_t Word){
    Mem[SP--] = Word & 0xFF;
    Mem[SP--] = (Word >> 8) & 0xFF;
    TotalCycles -= 2;
}
uint16_t EMU6502::CPU::PopWordFromStack(){
    uint16_t Word = 0;
    Word = (Mem[++SP] << 8);
    Word |= Mem[++SP] & 0xFF;
    TotalCycles -= 2;
    return Word;
}
uint16_t EMU6502::CPU::GetIndirectXAddress(){
    return ReadWord(FetchWord(PC) + X);
}
uint16_t EMU6502::CPU::GetIndirectYAddress(){
    return ReadWord(FetchWord(PC)) + Y;
}
uint8_t EMU6502::CPU::GetZeroPageAddress(){
    return FetchByte(PC);
}
uint8_t EMU6502::CPU::GetZeroPageXAddress(){
    return FetchByte(PC) + X;
}
uint8_t EMU6502::CPU::GetZeroPageYAddress(){
    return FetchByte(PC) + Y;
}
uint16_t EMU6502::CPU::GetAbsoluteAddress(){
    return FetchWord(PC);
}
uint16_t EMU6502::CPU::GetAbsoluteXAddress(){
    return FetchWord(PC) + X;
}
uint16_t EMU6502::CPU::GetAbsoluteYAddress(){
    return FetchWord(PC) + Y;
}
EMU6502::CPU::~CPU(){
    free(Mem.MemoryPointer);
    TotalCycles = 0;
}
int EMU6502::CPU::Execute(int32_t Cycles){
    TotalCycles = Cycles;
    if (Cycles == 0xFFFF) CyclesUnlimited = true;
    if(!initialized){ PC = ReadWord(PC); initialized++;}
    if (Mem[0xFFFB] != 0x00) {putchar(Mem[0xFFFB]); Mem[0xFFFB] = 0x00;}
    while (CyclesUnlimited || TotalCycles > 0){
		if (Mem[0xFFFB] != 0x00) {putchar(Mem[0xFFFB]); Mem[0xFFFB] = 0x00;}
        uint8_t instruction = FetchByte(PC);
        switch(instruction){
            // ADC instruction
            case ADC_IM:{
                uint8_t Byte = FetchByte(PC);
                A += (Byte + (C ? 1 : 0));
                ADCSetStatusFlags(Byte);
                break;
            }
            case ADC_ZP:{
                uint8_t Byte = ReadByte(GetZeroPageAddress());
                A += Byte + (C ? 1 : 0);
                ADCSetStatusFlags(Byte);
                break;
            }
            case ADC_ZPX:{
                uint8_t Byte = ReadByte(GetZeroPageXAddress());
                A += Byte + (C ? 1 : 0);
                TotalCycles--;
                ADCSetStatusFlags(Byte);
                break;
            }
            case ADC_ABS:{
                uint8_t Byte = ReadByte(GetAbsoluteAddress());
                A += Byte + (C ? 1 : 0);
                ADCSetStatusFlags(Byte);
                break;
            }
            case ADC_ABSX:{
                uint8_t Byte = ReadByte(GetAbsoluteXAddress());
                A += Byte + (C ? 1 : 0);
                ADCSetStatusFlags(Byte);
                break;
            }
            case ADC_ABSY:{
                uint8_t Byte = ReadByte(GetAbsoluteYAddress());
                A += Byte + (C ? 1 : 0);
                ADCSetStatusFlags(Byte);
                break;
            }
            case ADC_INDX:{
                uint8_t Byte = ReadByte(GetIndirectXAddress());
                A += Byte + (C ? 1 : 0);
                ADCSetStatusFlags(Byte);
                break;
            }
            case ADC_INDY:{
                uint8_t Byte = ReadByte(GetIndirectYAddress());
                A += Byte + (C ? 1 : 0);
                ADCSetStatusFlags(Byte);
                break;
            }
            // AND instruction
            case AND_IM:{
                A &= FetchByte(PC);
                ANDSetStatusFlags();
                break;
            }
            case AND_ZP:{
                A &= ReadByte(GetZeroPageAddress());
                ANDSetStatusFlags();
                break;
            }
            case AND_ZPX:{
                A &= ReadByte(GetZeroPageXAddress());
                ANDSetStatusFlags();
                TotalCycles--;
                break;
            }
            case AND_ABS:{
                A &= ReadByte(GetAbsoluteAddress());
                ANDSetStatusFlags();
                break;
            }
            case AND_ABSX:{
                A &= ReadByte(GetAbsoluteXAddress());
                ANDSetStatusFlags();
                break;
            }
            case AND_ABSY:{
                A &= ReadByte(GetAbsoluteYAddress());
                ANDSetStatusFlags();
                break;
            }
            case AND_INDX:{
                A &= ReadByte(GetIndirectXAddress());
                ANDSetStatusFlags();
                break;
            }
            case AND_INDY:{
                A &= ReadByte(GetIndirectYAddress());
                ANDSetStatusFlags();
                break;
            }
            // ASL instruction
            case ASL_AC:{
                C = (A & 0b10000000) > 0;
                A = (A << 1);
                TotalCycles--;
                break;
            }
            case ASL_ZP:{
                uint8_t Address = GetZeroPageAddress();
                uint8_t Value = ReadByte(Address);
                C = (Value & 0b10000000) > 0;
                Value = (Value << 1);
                Mem[Address] = Value;
                TotalCycles -= 2;
                break;
            }
            case ASL_ZPX:{
                uint8_t Address = GetZeroPageXAddress();
                uint8_t Value = ReadByte(Address);
                C = (Value & 0b10000000) > 0;
                Value = (Value << 1);
                Mem[Address] = Value;
                TotalCycles -= 3;
                break;
            }
            case ASL_ABS:{
                uint16_t Address = GetAbsoluteAddress();
                uint8_t Value = ReadByte(Address);
                C = (Value & 0b10000000) > 0;
                Value = (Value << 1);
                Mem[Address] = Value;
                TotalCycles -= 2;
                break;
            }
            case ASL_ABSX:{
                uint16_t Address = GetAbsoluteXAddress();
                uint8_t Value = ReadByte(Address);
                C = (Value & 0b10000000) > 0;
                Value = (Value << 1);
                Mem[Address] = Value;
                TotalCycles -= 3;
                break;
            }
            // BCC instruction
            case BCC:{
                TotalCycles--;
                if (C) break;
                int8_t TargetOffset = FetchByte(PC);
                PC += TargetOffset;
                TotalCycles--;
                break;
            }
            // BEQ instruction
            case BEQ:{
                TotalCycles--;
                if (!Z) break;
                int8_t TargetOffset = FetchByte(PC);
                PC += TargetOffset;
                TotalCycles--;
                break;
            }
            // BIT instruction
            case BIT_ZP:{
                uint8_t Value = ReadByte(GetZeroPageAddress());
                Z = (A & Value) == 0;
                V = (Value & 0b01000000) != 0;
                N = (Value & 0b10000000) != 0;
                TotalCycles -= 2;
                break;
            }
            case BIT_ABS:{
                uint8_t Value = ReadByte(GetAbsoluteAddress());
                Z = (A & Value) == 0;
                V = (Value & 0b01000000) != 0;
                N = (Value & 0b10000000) != 0;
                TotalCycles -= 3;
                break;
            }
            // BCC instruction
            case BCS:{
                TotalCycles--;
                if (!C) break;
                int8_t TargetOffset = FetchByte(PC);
                PC += TargetOffset;
                TotalCycles--;
                break;
            }
            // BMI instruction
            case BMI:{
                TotalCycles--;
                if (!N) break;
                int8_t TargetOffset = FetchByte(PC);
                PC += TargetOffset;
                TotalCycles--;
                break;
            }
            // BNE instruction
            case BNE:{
                TotalCycles--;
                if (Z) break;
                int8_t TargetOffset = FetchByte(PC);
                PC += TargetOffset;
                TotalCycles--;
                break;
            }
            // BPL instruction
            case BPL:{
                TotalCycles--;
                if (N) break;
                int8_t TargetOffset = FetchByte(PC);
                PC += TargetOffset;
                TotalCycles--;
                break;
            }
            // BRK instruction
            case BRK:{
                PushWordToStack(PC);
                uint8_t Byte = 0b00000100;
                if (C) Byte |= 0b10000000;
                if (Z) Byte |= 0b01000000;
                if (I) Byte |= 0b00100000;
                if (D) Byte |= 0b00010000;
                if (B) Byte |= 0b00001000;
                if (V) Byte |= 0b00000010;
                if (N) Byte |= 0b00000001;
                PushByteToStack(Byte);
                PC = ReadWord(0xFFFE);
                B = 1;
                TotalCycles -= 4;
                break;
            }
            // BVC instruction
            case BVC:{
                TotalCycles--;
                if (V) break;
                int8_t TargetOffset = FetchByte(PC);
                PC += TargetOffset;
                TotalCycles--;
                break;
            }
            // BVS instruction
            case BVS:{
                TotalCycles--;
                if (!V) break;
                int8_t TargetOffset = FetchByte(PC);
                PC += TargetOffset;
                TotalCycles--;
                break;
            }
            // CLC instruction
            case CLC:{
                C = 0;
                TotalCycles--;
                break;
            }
            // CLD instruction
            case CLD:{
                D = 0;
                TotalCycles--;
                break;
            }
            // CLI instruction
            case CLI:{
                I = 0;
                TotalCycles--;
                break;
            }
            // CLI instruction
            case CLV:{
                V = 0;
                TotalCycles--;
                break;
            }
            // CMP instruction
            case CMP_IM:{
                uint8_t Byte = FetchByte(PC);
                if (A < Byte){
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (A == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else{
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                break;
            }
            case CMP_ZP:{
                uint8_t Byte = ReadByte(GetZeroPageAddress());
                if (A < Byte){
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (A == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else{
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                break;
            }
            case CMP_ZPX:{
                uint8_t Byte = ReadByte(GetZeroPageXAddress());
                if (A < Byte){
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (A == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else{
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                TotalCycles--;
                break;
            }
            case CMP_ABS:{
                uint8_t Byte = ReadByte(GetAbsoluteAddress());
                if (A < Byte){
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (A == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else{
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                break;
            }
            case CMP_ABSX:{
                uint8_t Byte = ReadByte(GetAbsoluteXAddress());
                if (A < Byte){
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (A == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else{
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                break;
            }
            case CMP_ABSY:{
                uint8_t Byte = ReadByte(GetAbsoluteYAddress());
                if (A < Byte){
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (A == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else{
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                break;
            }
            case CMP_INDX:{
                uint8_t Byte = ReadByte(GetIndirectXAddress());
                if (A < Byte){
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (A == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else{
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                break;
            }
            case CMP_INDY:{
                uint8_t Byte = ReadByte(GetIndirectYAddress());
                if (A < Byte){
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (A == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else {
                    N = (A & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                break;
            }
            // CPX instruction
            case CPX_IM:{
                uint8_t Byte = FetchByte(PC);
                if (X < Byte){
                    N = (X & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (X == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else{
                    N = (X & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                break;
            }
            case CPX_ZP:{
                uint8_t Byte = ReadByte(GetZeroPageAddress());
                if (X < Byte){
                    N = (X & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (X == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else{
                    N = (X & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                break;
            }
            case CPX_ABS:{
                uint8_t Byte = ReadByte(GetAbsoluteAddress());
                if (X < Byte){
                    N = (X & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (X == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else{
                    N = (X & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                break;
            }
            // CPY instruction
            case CPY_IM:{
                uint8_t Byte = FetchByte(PC);
                if (Y < Byte){
                    N = (Y & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (Y == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else{
                    N = (Y & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                break;
            }
            case CPY_ZP:{
                uint8_t Byte = ReadByte(GetZeroPageAddress());
                if (Y < Byte){
                    N = (Y & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (Y == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else{
                    N = (Y & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                break;
            }
            case CPY_ABS:{
                uint8_t Byte = ReadByte(GetAbsoluteAddress());
                if (Y < Byte){
                    N = (Y & 0b10000000);
                    Z = 0;
                    C = 0;
                } else if (Y == Byte){
                    N = 0;
                    Z = 1;
                    C = 1;
                } else{
                    N = (Y & 0b10000000);
                    Z = 0;
                    C = 1;
                }
                break;
            }
            // DEC instruction
            case DEC_ZP:{
                uint16_t Address = GetZeroPageAddress();
                uint8_t Byte = ReadByte(Address);
                Byte--;
                DECSetStatusFlags(Byte);
                WriteByte(Byte, Address);
                TotalCycles--;
                break;
            }
            case DEC_ZPX:{
                uint16_t Address = GetZeroPageXAddress();
                uint8_t Byte = ReadByte(Address);
                Byte--;
                DECSetStatusFlags(Byte);
                WriteByte(Byte, Address);
                TotalCycles -= 2;
                break;
            }
            case DEC_ABS:{
                uint16_t Address = GetAbsoluteAddress();
                uint8_t Byte = ReadByte(Address);
                Byte--;
                DECSetStatusFlags(Byte);
                WriteByte(Byte, Address);
                TotalCycles--;
                break;
            }
            case DEC_ABSX:{
                uint16_t Address = GetAbsoluteXAddress();
                uint8_t Byte = ReadByte(Address);
                Byte--;
                DECSetStatusFlags(Byte);
                WriteByte(Byte, Address);
                TotalCycles -= 2;
                break;
            }
            // DEX instruction
            case DEX:{
                X--;
                TotalCycles--;
                DEXSetStatusFlags();
                break;
            }
            // DEY instruction
            case DEY:{
                Y--;
                TotalCycles--;
                DEYSetStatusFlags();
                break;
            }
            // EOR instruction
            case EOR_IM:{
                A ^= FetchByte(PC);
                EORSetStatusFlags();
                break;
            }
            case EOR_ZP:{
                A ^= ReadByte(GetZeroPageAddress());
                EORSetStatusFlags();
                break;
            }
            case EOR_ZPX:{
                A ^= ReadByte(GetZeroPageXAddress());
                EORSetStatusFlags();
                TotalCycles--;
                break;
            }
            case EOR_ABS:{
                A ^= ReadByte(GetAbsoluteAddress());
                EORSetStatusFlags();
                break;
            }
            case EOR_ABSX:{
                A ^= ReadByte(GetAbsoluteXAddress());
                EORSetStatusFlags();
                break;
            }
            case EOR_ABSY:{
                A ^= ReadByte(GetAbsoluteYAddress());
                EORSetStatusFlags();
                break;
            }
            case EOR_INDX:{
                A ^= ReadByte(GetIndirectXAddress());
                EORSetStatusFlags();
                break;
            }
            case EOR_INDY:{
                A ^= ReadByte(GetIndirectYAddress());
                EORSetStatusFlags();
                break;
            }
            // INC instruction
            case INC_ZP:{
                uint16_t Address = GetZeroPageAddress();
                uint8_t Byte = ReadByte(Address);
                Byte++;
                DECSetStatusFlags(Byte);
                WriteByte(Byte, Address);
                TotalCycles--;
                break;
            }
            case INC_ZPX:{
                uint16_t Address = GetZeroPageXAddress();
                uint8_t Byte = ReadByte(Address);
                Byte++;
                DECSetStatusFlags(Byte);
                WriteByte(Byte, Address);
                TotalCycles -= 2;
                break;
            }
            case INC_ABS:{
                uint16_t Address = GetAbsoluteAddress();
                uint8_t Byte = ReadByte(Address);
                Byte++;
                DECSetStatusFlags(Byte);
                WriteByte(Byte, Address);
                TotalCycles--;
                break;
            }
            case INC_ABSX:{
                uint16_t Address = GetAbsoluteXAddress();
                uint8_t Byte = ReadByte(Address);
                Byte++;
                DECSetStatusFlags(Byte);
                WriteByte(Byte, Address);
                TotalCycles -= 2;
                break;
            }
            // DEX instruction
            case INX:{
                X++;
                TotalCycles--;
                DEXSetStatusFlags();
                break;
            }
            // DEY instruction
            case INY:{
                Y++;
                TotalCycles--;
                DEYSetStatusFlags();
                break;
            }
            // JMP instruction
            case JMP_ABS:{
                PC = FetchWord(PC);
                break;
            }
            case JMP_IND:{
                PC = FetchWord(FetchWord(PC));
                break;
            }
            // LDA instruction
            case LDA_IM:{
                A = FetchByte(PC);
                LDASetStatusFlags();
                break;
            }
            case LDA_ZP:{
                A = ReadByte(GetZeroPageAddress());
                LDASetStatusFlags();
                break;
            }
            case LDA_ZPX:{
                A = ReadByte(GetZeroPageXAddress());
                TotalCycles--;
                LDASetStatusFlags();
                break;
            }
            case LDA_ABS:{
                A = ReadByte(GetAbsoluteAddress());
                LDASetStatusFlags();
                break;
            }
            case LDA_ABSX:{
                A = ReadByte(GetAbsoluteXAddress());
                LDASetStatusFlags();
                break;
            }
            case LDA_ABSY:{
                A = ReadByte(GetAbsoluteYAddress());
                LDASetStatusFlags();
                break;
            }
            case LDA_INDX:{
                A = ReadByte(GetIndirectXAddress());
                LDASetStatusFlags();
                break;
            }
            case LDA_INDY:{
                A = ReadByte(GetIndirectYAddress());
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
                X = ReadByte(GetZeroPageAddress());
                LDXSetStatusFlags();
                break;
            }
            case LDX_ZPY:{
                X = ReadByte(GetZeroPageYAddress());
                TotalCycles--;
                LDXSetStatusFlags();
                break;
            }
            case LDX_ABS:{
                X = ReadByte(GetAbsoluteAddress());
                LDXSetStatusFlags();
                break;
            }
            case LDX_ABSY:{
                X = ReadByte(GetAbsoluteYAddress());
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
                Y = ReadByte(GetZeroPageAddress());
                LDYSetStatusFlags();
                break;
            }
            case LDY_ZPX:{
                Y = ReadByte(GetZeroPageXAddress());
                TotalCycles--;
                LDYSetStatusFlags();
                break;
            }
            case LDY_ABS:{
                Y = ReadByte(GetAbsoluteAddress());
                LDYSetStatusFlags();
                break;
            }
            case LDY_ABSX:{
                Y = ReadByte(GetAbsoluteXAddress());
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
                A |= ReadByte(GetZeroPageAddress());
                ORASetStatusFlags();
                break;
            }
            case ORA_ZPX:{
                A |= ReadByte(GetZeroPageXAddress());
                TotalCycles--;
                ORASetStatusFlags();
                break;
            }
            case ORA_ABS:{
                A |= ReadByte(GetAbsoluteAddress());
                ORASetStatusFlags();
                break;
            }
            case ORA_ABSX:{
                A |= ReadByte(GetAbsoluteXAddress());
                ORASetStatusFlags();
                break;
            }
            case ORA_ABSY:{
                A |= ReadByte(GetAbsoluteYAddress());
                ORASetStatusFlags();
                break;
            }
            case ORA_INDX:{
                A |= ReadByte(GetIndirectXAddress());
                ORASetStatusFlags();
                break;
            }
            case ORA_INDY:{
                A |= ReadByte(GetIndirectYAddress());
                ORASetStatusFlags();
                break;
            }
            // PHA instruction
            case PHA:{
                PushByteToStack(A);
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
                PushByteToStack(Byte);
                TotalCycles -= 2;
                break;
            }
            // PLA instruction
            case PLA:{
                A = PopByteFromStack();
                TotalCycles -= 3;
                PLASetStatusFlags();
                break;
            }
            // PLP instruction
            case PLP:{
                uint8_t Byte = PopByteFromStack();
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
                uint16_t ZeroPageAddress = GetZeroPageAddress();
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
                uint8_t ZeroPageAddress = GetZeroPageXAddress();
                uint8_t Value = Mem[ZeroPageAddress];
                if (Value & 0b1000000) C = 1;
                Value = (Value << 1);
                if (tmp_carry) Value++;
                Mem[ZeroPageAddress] = Value;
                TotalCycles -= 4;
                break;
            }
            case ROL_ABS:{
                uint8_t tmp_carry = C;
                uint16_t Address = GetAbsoluteAddress();
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
                uint16_t Address = GetAbsoluteXAddress();
                uint8_t Value = Mem[Address];
                if (Value & 0b1000000) C = 1;
                Value = (Value << 1);
                if (tmp_carry) Value++;
                Mem[Address] = Value;
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
            // RTI instruction
            case RTI:{
                uint8_t Flags = PopByteFromStack();
                if (Flags & 0b10000000) C = 1;
                if (Flags & 0b01000000) Z = 1;
                if (Flags & 0b00100000) I = 1;
                if (Flags & 0b00010000) D = 1;
                if (Flags & 0b00001000) B = 1;
                if (Flags & 0b00000010) V = 1;
                if (Flags & 0b00000001) N = 1;
                PC = PopWordFromStack();
                TotalCycles--;
                break;
            }
            // SBC instruction
            case SBC_IM:{
                uint8_t Byte = FetchByte(PC);
                A -= (Byte + (C ? 0 : 1));
                SBCSetStatusFlags(Byte);
                break;
            }
            case SBC_ZP:{
                uint8_t Byte = ReadByte(GetZeroPageAddress());
                A -= (Byte + (C ? 0 : 1));
                SBCSetStatusFlags(Byte);
                break;
            }
            case SBC_ZPX:{
                uint8_t Byte = ReadByte(GetZeroPageXAddress());
                A -= (Byte + (C ? 0 : 1));
                TotalCycles--;
                SBCSetStatusFlags(Byte);
                break;
            }
            case SBC_ABS:{
                uint8_t Byte = ReadByte(GetAbsoluteAddress());
                A -= (Byte + (C ? 0 : 1));
                SBCSetStatusFlags(Byte);
                break;
            }
            case SBC_ABSX:{
                uint8_t Byte = ReadByte(GetAbsoluteXAddress());
                A -= (Byte + (C ? 0 : 1));
                SBCSetStatusFlags(Byte);
                break;
            }
            case SBC_ABSY:{
                uint8_t Byte = ReadByte(GetAbsoluteYAddress());
                A -= (Byte + (C ? 0 : 1));
                SBCSetStatusFlags(Byte);
                break;
            }
            case SBC_INDX:{
                uint8_t Byte = ReadByte(GetIndirectXAddress());
                A -= (Byte + (C ? 0 : 1));
                SBCSetStatusFlags(Byte);
                break;
            }
            case SBC_INDY:{
                uint8_t Byte = ReadByte(GetIndirectYAddress());
                A -= (Byte + (C ? 0 : 1));
                SBCSetStatusFlags(Byte);
                break;
            }
            // SEC instruction
            case SEC:{
                C = 1;
                TotalCycles--;
                break;
            }
            // SED instruction
            case SED:{
                D = 1;
                TotalCycles--;
                break;
            }
            // SEI instruction
            case SEI:{
                I = 1;
                TotalCycles--;
                break;
            }
            // STA instruction
            case STA_ZP:{
                Mem[GetZeroPageAddress()] = A;
                TotalCycles--;
                break;
            }
            case STA_ZPX:{
                Mem[GetZeroPageXAddress()] = A;
                TotalCycles -= 2;
                break;
            }
            case STA_ABS:{
                Mem[GetAbsoluteAddress()] = A;
                TotalCycles--;
                break;
            }
            case STA_ABSX:{
                Mem[GetAbsoluteXAddress()] = A;
                TotalCycles -= 2;
                break;
            }
            case STA_ABSY:{
                Mem[GetAbsoluteYAddress()] = A;
                TotalCycles -= 2;
                break;
            }
            case STA_INDX:{
                Mem[GetIndirectXAddress()] = A;
                TotalCycles--;
                break;
            }
            case STA_INDY:{
                Mem[GetIndirectYAddress()] = A;
                TotalCycles--;
                break;
            }
            // STX instruction
            case STX_ZP:{
                Mem[GetZeroPageAddress()] = X;
                TotalCycles--;
                break;
            }
            case STX_ZPY:{
                Mem[GetZeroPageYAddress()] = X;
                TotalCycles -= 2;
                break;
            }
            case STX_ABS:{
                Mem[GetAbsoluteAddress()] = X;
                TotalCycles--;
                break;
            }
            // STY instruction
            case STY_ZP:{
                Mem[GetZeroPageAddress()] = Y;
                TotalCycles--;
                break;
            }
            case STY_ZPX:{
                Mem[GetZeroPageXAddress()] = Y;
                TotalCycles -= 2;
                break;
            }
            case STY_ABS:{
                Mem[GetAbsoluteAddress()] = Y;
                TotalCycles--;
                break;
            }
            case TAX:{
                X = A;
                TotalCycles--;
                TAXSetStatusFlags();
                break;
            }
            case TAY:{
                Y = A;
                TotalCycles--;
                TAYSetStatusFlags();
                break;
            }
            case TSX:{
                X = SP;
                TotalCycles--;
                TSXSetStatusFlags();
                break;
            }
            case TXA:{
                A = X;
                TotalCycles--;
                TXASetStatusFlags();
                break;
            }
            case TXS:{
                SP = X;
                TotalCycles--;
                break;
            }
            case TYA:{
                A = Y;
                TotalCycles--;
                TYASetStatusFlags();
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
			case KIL:{
				puts("CPU stopped.");
				return 0;
			}
            default:
                printf("Unknown opcode %X! CPU halted.\n", instruction);
                printf("Executed instructions: %ld\n", ExecutedInstructions);
                StateDump();
                SaveDump();
                return 0;
                break;
        }
        ExecutedInstructions++;
    }
    return 1;
}
