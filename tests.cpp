#include <gtest/gtest.h>
#include "cpu.hpp"
#include "opcodes.hpp"

EMU6502::CPU cpu;

TEST(CPUEMU, CPU_INIT){
    EXPECT_TRUE(cpu.Reset());
}

TEST(CPUEMU, CPU_ADC_IM){
    cpu.Mem[0xFFFC] = ADC_IM;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.A = 2;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.A, 0x27);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ADC_ZP){
    cpu.Mem[0xFFFC] = ADC_ZP;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0x48;
    cpu.A = 2;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.A, 0x4B);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ADC_ZPX){
    cpu.Mem[0xFFFC] = ADC_ZPX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x26] = 0x48;
    cpu.X = 0x02;
    cpu.A = 2;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x4B);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ADC_ABS){
    cpu.Mem[0xFFFC] = ADC_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0x48;
    cpu.A = 2;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x4B);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ADC_ABSX){
    cpu.Mem[0xFFFC] = ADC_ABSX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x48;
    cpu.X = 2;
    cpu.A = 2;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x4B);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ADC_ABSY){
    cpu.Mem[0xFFFC] = ADC_ABSY;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x48;
    cpu.Y = 2;
    cpu.A = 2;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x4B);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ADC_INDX){
    cpu.Mem[0xFFFC] = ADC_INDX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x04;
    cpu.Mem[0x0227] = 0x05;
    cpu.Mem[0x0504] = 0x69;
    cpu.X = 2;
    cpu.A = 2;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.A, 0x6C);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ADC_INDY){
    cpu.Mem[0xFFFC] = ADC_INDY;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0224] = 0x04;
    cpu.Mem[0x0225] = 0x05;
    cpu.Mem[0x0506] = 0x69;
    cpu.Y = 2;
    cpu.A = 2;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.A, 0x6C);
    cpu.Reset();
}

TEST(CPUEMU, CPU_AND_IM){
    cpu.Mem[0xFFFC] = AND_IM;
    cpu.Mem[0xFFFD] = 0b00000101;
    cpu.A = 0b01010101;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.A, 0b00000101);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_AND_ZP){
    cpu.Mem[0xFFFC] = AND_ZP;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0b00000101;
    cpu.A = 0b01010101;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.A, 0b00000101);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_AND_ZPX){
    cpu.Mem[0xFFFC] = AND_ZPX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x26] = 0b00000101;
    cpu.X = 0x02;
    cpu.A = 0b01010101;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0b00000101);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_AND_ABS){
    cpu.Mem[0xFFFC] = AND_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0b00000101;
    cpu.A = 0b01010101;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0b00000101);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_AND_ABSX){
    cpu.Mem[0xFFFC] = AND_ABSX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0b00000101;
    cpu.X = 2;
    cpu.A = 0b01010101;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0b00000101);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_AND_ABSY){
    cpu.Mem[0xFFFC] = AND_ABSY;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0b00000101;
    cpu.Y = 2;
    cpu.A = 0b01010101;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0b00000101);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_AND_INDX){
    cpu.Mem[0xFFFC] = AND_INDX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x04;
    cpu.Mem[0x0227] = 0x05;
    cpu.Mem[0x0504] = 0b00000101;
    cpu.X = 2;
    cpu.A = 0b01010101;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.A, 0b00000101);
    cpu.Reset();
}

TEST(CPUEMU, CPU_AND_INDY){
    cpu.Mem[0xFFFC] = AND_INDY;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0224] = 0x04;
    cpu.Mem[0x0225] = 0x05;
    cpu.Mem[0x0506] = 0b00000101;
    cpu.Y = 2;
    cpu.A = 0b01010101;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.A, 0b00000101);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ASL_AC){
    cpu.Mem[0xFFFC] = ASL_AC;
    cpu.A = 0b11010101;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.A, 0b10101010);
    EXPECT_TRUE(cpu.C);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ASL_ZP){
    cpu.Mem[0xFFFC] = ASL_ZP;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0b11010101;
    EXPECT_TRUE(cpu.Execute(5));
    EXPECT_EQ(cpu.Mem[0x24], 0b10101010);
    EXPECT_TRUE(cpu.C);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ASL_ZPX){
    cpu.Mem[0xFFFC] = ASL_ZPX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x26] = 0b11010101;
    cpu.X = 0x02;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.Mem[0x26], 0b10101010);
    EXPECT_TRUE(cpu.C);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ASL_ABS){
    cpu.Mem[0xFFFC] = ASL_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0b11010101;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.Mem[0x24], 0b10101010);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ASL_ABSX){
    cpu.Mem[0xFFFC] = ASL_ABSX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0b01010101;
    cpu.X = 2;
    EXPECT_TRUE(cpu.Execute(7));
    EXPECT_EQ(cpu.Mem[0x0226], 0b10101010);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_BCC){
    cpu.Mem[0xFFFC] = BCC;
    cpu.Mem[0xFFFD] = -10;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.PC, 0xFFFE - 10);
    cpu.Reset();
}

TEST(CPUEMU, CPU_BCS){
    cpu.Mem[0xFFFC] = BCS;
    cpu.Mem[0xFFFD] = -10;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.PC, 0xFFFE - 10);
    cpu.Reset();
}

TEST(CPUEMU, CPU_BEQ){
    cpu.Mem[0xFFFC] = BEQ;
    cpu.Mem[0xFFFD] = -10;
    cpu.Z = 1;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.PC, 0xFFFE - 10);
    cpu.Reset();
}

TEST(CPUEMU, CPU_BIT_ZP){
    cpu.Mem[0xFFFC] = BIT_ZP;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0b10101010;
    cpu.A = 0b01010101;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.Z, 1);
    EXPECT_EQ(cpu.N, 1);
    EXPECT_EQ(cpu.V, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_BIT_ABS){
    cpu.Mem[0xFFFC] = BIT_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0224] = 0b10101010;
    cpu.A = 0b01010101;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.Z, 1);
    EXPECT_EQ(cpu.N, 1);
    EXPECT_EQ(cpu.V, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_BMI){
    cpu.Mem[0xFFFC] = BMI;
    cpu.Mem[0xFFFD] = -10;
    cpu.N = 1;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.PC, 0xFFFE - 10);
    cpu.Reset();
}

TEST(CPUEMU, CPU_BNE){
    cpu.Mem[0xFFFC] = BNE;
    cpu.Mem[0xFFFD] = -10;
    cpu.Z = 0;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.PC, 0xFFFE - 10);
    cpu.Reset();
}

TEST(CPUEMU, CPU_BPL){
    cpu.Mem[0xFFFC] = BPL;
    cpu.Mem[0xFFFD] = -10;
    cpu.N = 0;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.PC, 0xFFFE - 10);
    cpu.Reset();
}

TEST(CPUEMU, CPU_BRK){
    cpu.Mem[0xFFFC] = BRK;
    cpu.Mem[0xFFFE] = 0x69;
    cpu.Mem[0xFFFF] = 0x96;
    EXPECT_TRUE(cpu.Execute(7));
    EXPECT_EQ(cpu.PC, 0x9669);
    EXPECT_EQ(cpu.SP, 0x1FC);
    EXPECT_EQ(cpu.B, 1);
    cpu.Reset();
}

TEST(CPUEMU, CPU_BVC){
    cpu.Mem[0xFFFC] = BVC;
    cpu.Mem[0xFFFD] = -10;
    cpu.V = 0;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.PC, 0xFFFE - 10);
    cpu.Reset();
}

TEST(CPUEMU, CPU_BVS){
    cpu.Mem[0xFFFC] = BVS;
    cpu.Mem[0xFFFD] = -10;
    cpu.V = 1;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.PC, 0xFFFE - 10);
    cpu.Reset();
}

TEST(CPUEMU, CPU_CLC){
    cpu.Mem[0xFFFC] = CLC;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.C, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_CLD){
    cpu.Mem[0xFFFC] = CLD;
    cpu.D = 1;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.C, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_CLI){
    cpu.Mem[0xFFFC] = CLI;
    cpu.I = 1;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.C, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_CLV){
    cpu.Mem[0xFFFC] = CLV;
    cpu.V = 1;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.C, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_IM){
    cpu.Mem[0xFFFC] = CMP_IM;
    cpu.Mem[0xFFFD] = 0x24;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.A, 0x24);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_ZP){
    cpu.Mem[0xFFFC] = LDA_ZP;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0x48;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.A, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_ZPX){
    cpu.Mem[0xFFFC] = CMP_ZPX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x26] = 0x48;
    cpu.X = 0x02;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_ABS){
    cpu.Mem[0xFFFC] = CMP_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0x48;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_ABSX){
    cpu.Mem[0xFFFC] = CMP_ABSX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x48;
    cpu.X = 2;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_ABSY){
    cpu.Mem[0xFFFC] = CMP_ABSY;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x48;
    cpu.Y = 2;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_INDX){
    cpu.Mem[0xFFFC] = CMP_INDX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x04;
    cpu.Mem[0x0227] = 0x05;
    cpu.Mem[0x0504] = 0x69;
    cpu.X = 2;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.A, 0x69);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_INDY){
    cpu.Mem[0xFFFC] = CMP_INDY;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0224] = 0x04;
    cpu.Mem[0x0225] = 0x05;
    cpu.Mem[0x0506] = 0x69;
    cpu.Y = 2;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.A, 0x69);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_IM){
    cpu.Mem[0xFFFC] = LDA_IM;
    cpu.Mem[0xFFFD] = 0x24;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.A, 0x24);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_ZP){
    cpu.Mem[0xFFFC] = LDA_ZP;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0x48;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.A, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_ZPX){
    cpu.Mem[0xFFFC] = LDA_ZPX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x26] = 0x48;
    cpu.X = 0x02;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_ABS){
    cpu.Mem[0xFFFC] = LDA_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0x48;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_ABSX){
    cpu.Mem[0xFFFC] = LDA_ABSX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x48;
    cpu.X = 2;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_ABSY){
    cpu.Mem[0xFFFC] = LDA_ABSY;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x48;
    cpu.Y = 2;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_INDX){
    cpu.Mem[0xFFFC] = LDA_INDX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x04;
    cpu.Mem[0x0227] = 0x05;
    cpu.Mem[0x0504] = 0x69;
    cpu.X = 2;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.A, 0x69);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDA_INDY){
    cpu.Mem[0xFFFC] = LDA_INDY;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0224] = 0x04;
    cpu.Mem[0x0225] = 0x05;
    cpu.Mem[0x0506] = 0x69;
    cpu.Y = 2;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.A, 0x69);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDX_IM){
    cpu.Mem[0xFFFC] = LDX_IM;
    cpu.Mem[0xFFFD] = 0x24;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.X, 0x24);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDX_ZP){
    cpu.Mem[0xFFFC] = LDX_ZP;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0x48;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.X, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDX_ZPY){
    cpu.Mem[0xFFFC] = LDX_ZPY;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x26] = 0x48;
    cpu.Y = 0x02;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.X, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDX_ABS){
    cpu.Mem[0xFFFC] = LDX_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0x48;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.X, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDX_ABSY){
    cpu.Mem[0xFFFC] = LDX_ABSY;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x48;
    cpu.Y = 2;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.X, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDY_IM){
    cpu.Mem[0xFFFC] = LDY_IM;
    cpu.Mem[0xFFFD] = 0x24;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.Y, 0x24);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDY_ZP){
    cpu.Mem[0xFFFC] = LDY_ZP;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0x48;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.Y, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDY_ZPX){
    cpu.Mem[0xFFFC] = LDY_ZPX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x26] = 0x48;
    cpu.X = 0x02;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.Y, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDY_ABS){
    cpu.Mem[0xFFFC] = LDY_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0x48;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.Y, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_LDY_ABSX){
    cpu.Mem[0xFFFC] = LDY_ABSX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x48;
    cpu.X = 2;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.Y, 0x48);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_NOP){
    cpu.Mem[0xFFFC] = NOP;
    cpu.Execute(2);
    EXPECT_EQ(cpu.PC, 0xFFFD);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ORA_IM){
    cpu.Mem[0xFFFC] = ORA_IM;
    cpu.Mem[0xFFFD] = 0x02;
    cpu.A = 0x55;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.A, 0x57);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ORA_ZP){
    cpu.Mem[0xFFFC] = ORA_ZP;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0x02;
    cpu.A = 0x55;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.A, 0x57);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ORA_ZPX){
    cpu.Mem[0xFFFC] = ORA_ZPX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x26] = 0x02;
    cpu.X = 0x02;
    cpu.A = 0x55;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x57);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ORA_ABS){
    cpu.Mem[0xFFFC] = ORA_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0x02;
    cpu.A = 0x55;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x57);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ORA_ABSX){
    cpu.Mem[0xFFFC] = ORA_ABSX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x02;
    cpu.X = 0x02;
    cpu.A = 0x55;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x57);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ORA_ABSY){
    cpu.Mem[0xFFFC] = ORA_ABSY;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x02;
    cpu.Y = 0x02;
    cpu.A = 0x55;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.A, 0x57);
    EXPECT_EQ(cpu.TotalCycles, 0);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ORA_INDX){
    cpu.Mem[0xFFFC] = ORA_INDX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0226] = 0x04;
    cpu.Mem[0x0227] = 0x05;
    cpu.Mem[0x0504] = 0x02;
    cpu.X = 0x02;
    cpu.A = 0x55;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.A, 0x57);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ORA_INDY){
    cpu.Mem[0xFFFC] = ORA_INDY;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0224] = 0x04;
    cpu.Mem[0x0225] = 0x05;
    cpu.Mem[0x0506] = 0x02;
    cpu.Y = 0x02;
    cpu.A = 0x55;
    EXPECT_TRUE(cpu.Execute(5));
    EXPECT_EQ(cpu.A, 0x57);
    cpu.Reset();
}

TEST(CPUEMU, CPU_PHA){
    cpu.Mem[0xFFFC] = PHA;
    cpu.SP = 0x01FF;
    cpu.A = 0x69;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.SP, 0x01FE);
    EXPECT_EQ(cpu.Mem[cpu.SP + 1], 0x69);
    cpu.Reset();
}

TEST(CPUEMU, CPU_PHP){
    cpu.Mem[0xFFFC] = PHP;
    cpu.C = 0;
    cpu.Z = 1;
    cpu.I = 0;
    cpu.D = 1;
    cpu.B = 0;
    cpu.V = 0;
    cpu.N = 1;
    EXPECT_TRUE(cpu.Execute(3));
    EXPECT_EQ(cpu.SP, 0x01FE);
    EXPECT_EQ(cpu.Mem[cpu.SP + 1], 0b01010101);
    cpu.Reset();
}

TEST(CPUEMU, CPU_PLA){
    cpu.Mem[0xFFFC] = PHA;
    cpu.Mem[0xFFFD] = PLA;
    cpu.SP = 0x01FF;
    cpu.A = 0x69;
    cpu.Execute(3);
    cpu.A = 0x00;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.SP, 0x01FF);
    EXPECT_EQ(cpu.A, 0x69);
    cpu.Reset();
}

TEST(CPUEMU, CPU_PLP){
    cpu.Mem[0xFFFC] = PHP;
    cpu.Mem[0xFFFD] = PLP;
    cpu.C = 0;
    cpu.Z = 1;
    cpu.I = 0;
    cpu.D = 1;
    cpu.B = 0;
    cpu.V = 0;
    cpu.N = 1;
    cpu.Execute(3);
    cpu.C = cpu.Z = cpu.I = cpu.D = cpu.B = cpu.V = cpu.N = 0;
    EXPECT_TRUE(cpu.Execute(4));
    EXPECT_EQ(cpu.SP, 0x1FF);
    EXPECT_FALSE(cpu.C);
    EXPECT_TRUE(cpu.Z);
    EXPECT_FALSE(cpu.I);
    EXPECT_TRUE(cpu.D);
    EXPECT_FALSE(cpu.B);
    EXPECT_FALSE(cpu.V);
    EXPECT_TRUE(cpu.N);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ROL_AC){
    cpu.Mem[0xFFFC] = ROL_AC;
    cpu.A = 0b01010101;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.A, 0b10101011);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ROL_ZP){
    cpu.Mem[0xFFFC] = ROL_ZP;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0b01010101;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(5));
    EXPECT_EQ(cpu.Mem[0x24], 0b10101011);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ROL_ZPX){
    cpu.Mem[0xFFFC] = ROL_ZPX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x2A] = 0b01010101;
    cpu.C = 1;
    cpu.X = 6;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.Mem[0x2A], 0b10101011);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ROL_ABS){
    cpu.Mem[0xFFFC] = ROL_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0224] = 0b01010101;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.Mem[0x0224], 0b10101011);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ROL_ABSX){
    cpu.Mem[0xFFFC] = ROL_ABSX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x022A] = 0b01010101;
    cpu.C = 1;
    cpu.X = 6;
    EXPECT_TRUE(cpu.Execute(7));
    EXPECT_EQ(cpu.Mem[0x022A], 0b10101011);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ROR_AC){
    cpu.Mem[0xFFFC] = ROR_AC;
    cpu.A = 0b10101010;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(2));
    EXPECT_EQ(cpu.A, 0b11010101);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ROR_ZP){
    cpu.Mem[0xFFFC] = ROR_ZP;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x24] = 0b10101010;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(5));
    EXPECT_EQ(cpu.Mem[0x24], 0b11010101);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ROR_ZPX){
    cpu.Mem[0xFFFC] = ROR_ZPX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0x2A] = 0b10101010;
    cpu.C = 1;
    cpu.X = 6;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.Mem[0x2A], 0b11010101);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ROR_ABS){
    cpu.Mem[0xFFFC] = ROR_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0224] = 0b10101010;
    cpu.C = 1;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.Mem[0x0224], 0b11010101);
    cpu.Reset();
}

TEST(CPUEMU, CPU_ROR_ABSX){
    cpu.Mem[0xFFFC] = ROR_ABSX;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x022A] = 0b10101010;
    cpu.C = 1;
    cpu.X = 6;
    EXPECT_TRUE(cpu.Execute(7));
    EXPECT_EQ(cpu.Mem[0x022A], 0b11010101);
    cpu.Reset();
}

TEST(CPUEMU, CPU_JSR_ABS){
    cpu.Mem[0xFFFC] = JSR_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.PC, 0x0024);
    EXPECT_EQ(cpu.Mem[cpu.SP + 1], 0xFC);
    EXPECT_EQ(cpu.Mem[cpu.SP + 2], 0xFF);
    EXPECT_EQ(cpu.SP, 0x01FD);
    cpu.Reset();
}

TEST(CPUEMU, CPU_RTS){
    cpu.Mem[0xFFFC] = JSR_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0224] = RTS;
    EXPECT_TRUE(cpu.Execute(12));
    EXPECT_EQ(cpu.PC, 0xFFFC);
    EXPECT_EQ(cpu.SP, 0x01FF);
    cpu.Reset();
}

