#include <gtest/gtest.h>
#include "cpu.hpp"
#include "opcodes.hpp"

CPU cpu;

TEST(CPUEMU, CPU_INIT){
    EXPECT_TRUE(cpu.Reset());
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

TEST(CPUEMU, CPU_JSR_ABS){
    cpu.Mem[0xFFFC] = JSR_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    EXPECT_TRUE(cpu.Execute(6));
    EXPECT_EQ(cpu.PC, 0x0024);
    EXPECT_EQ(cpu.Mem[cpu.SP - 2], 0xFC);
    EXPECT_EQ(cpu.Mem[cpu.SP - 1], 0xFF);
    EXPECT_EQ(cpu.SP, 0x0102);
    cpu.Reset();
}

TEST(CPUEMU, CPU_RTS){
    cpu.Mem[0xFFFC] = JSR_ABS;
    cpu.Mem[0xFFFD] = 0x24;
    cpu.Mem[0xFFFE] = 0x02;
    cpu.Mem[0x0224] = RTS;
    EXPECT_TRUE(cpu.Execute(12));
    EXPECT_EQ(cpu.PC, 0xFFFC);
    EXPECT_EQ(cpu.SP, 0x0100);
    cpu.Reset();
}

