//6502 emulation

#include <stdio.h>
#include <stdlib.h>

using Byte = unsigned char;  //8 bit
using Word = unsigned short; //16 bit
using u32 = unsigned int;

struct Mem {
    static constexpr u32 MAX_MEM = 1024 * 64;
    Byte Data[MAX_MEM];

    void Initialize() {
        for (u32 i = 0; i < MAX_MEM; i++) {
            Data[i] = 0;
        }
    }

    //read 1 byte
    Byte operator[](u32 Address) const {
        //assert address is valid
        return Data[Address];
    }
    //write 1 byte
    Byte &operator[](u32 Address) {
        return Data[Address];
    }

    //write 2 bytes
    void WriteWord(Word Value, u32 Address, u32 &Cycles) {
        Data[Address] = Value & 0xff; //zero second half and write whole thing
        Cycles--;
        Data[Address + 1] = (Value >> 8); //push in second half
        Cycles--;
    }
};

struct CPU {
    Word PC; //program counter
    Word SP; //stack pointer

    Byte A, X, Y; //registers

    //status flags
    Byte C : 1;
    Byte Z : 1;
    Byte I : 1;
    Byte D : 1;
    Byte B : 1;
    Byte V : 1;
    Byte N : 1;

    void Reset(Mem &memory) {
        PC = 0xFFFC;                   //start execution from here
        SP = 0x0100;                   //set stack pointer in first 256 byte
        C = X = I = D = B = V = N = 0; //clear flags
        A = X = Y = 0;
        memory.Initialize();
    }

    Byte FetchByte(u32 &Cycles, Mem &memory) {
        Byte Data = memory[PC];
        PC++;
        Cycles--;
        return Data;
    }
    Word FetchWord(u32 &Cycles, Mem &memory) {
        Word Data = memory[PC];
        PC++;
        Cycles--;
        //processor is little endian
        Data |= (memory[PC] << 8);
        PC++;
        Cycles--;

        //handle endianness maybe
        //if(PLATFORM_BIG_ENDIAN){
        //  SwapBytesInWord(Data);
        //}

        return Data;
    }
    //this doesnt increment PC
    Byte ReadByte(u32 &Cycles, Byte Address, Mem &memory) {
        Byte Data = memory[Address];
        Cycles--;
        return Data;
    }
    //opcodes
    static constexpr Byte
        INS_LDA_IM = 0xA9,
        INS_LDA_ZP = 0x45,
        INS_LDA_ZPX = 0xB5,
        INS_JSR = 0x20;

    void LDASetStatus() {
        Z = (A == 0);
        N = (A & 0b10000000) > 0;
    }

    void Execute(u32 Cycles, Mem &memory) {
        while (Cycles > 0) {
            Byte ins = FetchByte(Cycles, memory);
            switch (ins) {
                case INS_LDA_IM: {
                    /*Load val into A an set zero and neg appropriately*/
                    Byte Value = FetchByte(Cycles, memory);
                    A = Value;
                    LDASetStatus();
                } break;
                case INS_LDA_ZP: {
                    Byte ZeroPageAddress = FetchByte(Cycles, memory);
                    A = ReadByte(Cycles, ZeroPageAddress, memory);
                    LDASetStatus();
                } break;
                case INS_LDA_ZPX: {
                    Byte ZeroPageAddress = FetchByte(Cycles, memory);
                    ZeroPageAddress += X;
                    Cycles--;
                    A = ReadByte(Cycles, ZeroPageAddress, memory);
                    LDASetStatus();
                } break;
                case INS_JSR: {
                    Word JumpAddr = FetchWord(Cycles, memory);
                    memory.WriteWord(PC - 1, SP, Cycles);
                    SP++;
                    PC = JumpAddr;
                    Cycles--;
                } break;
                default: {
                    printf("Unhandled instruction: %d\n", ins);
                } break;
            }
        }
    }
};

int main() {
    Mem mem;
    CPU cpu;
    cpu.Reset(mem);

    //*start program
    mem[0xFFFC] = CPU::INS_JSR;
    mem[0xFFFD] = 0x42;
    mem[0xFFFE] = 0xAA;
    mem[0xAA42] = CPU::INS_JSR;
    mem[0xAA43] = 0x15;
    mem[0xAA44] = 0xAE;
    mem[0xAE15] = CPU::INS_LDA_IM;
    mem[0xAE16] = 5;
    

    //*end program

    cpu.Execute(20, mem);

    //debugging
    printf("%d", cpu.A);
    return 0;
}