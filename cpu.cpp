#include "cpu.hpp"

int EMU6502::CPU::Execute(int32_t Cycles){
            TotalCycles = Cycles;
            while (TotalCycles > 0){
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
                        SaveDump();
                        return 0;
                        break;
                }
            }
            return 1;
        }
