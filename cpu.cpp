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
                        LDASetStatusFlags();
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
                        } else{
                            N = (A & 0b10000000);
                            Z = 0;
                            C = 1;
                        }
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
