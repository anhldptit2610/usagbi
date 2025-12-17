#pragma once

#include "common.h"
#include "rom.h"
#include "bus.h"
#include <fstream>

typedef struct Instruction {
	u8 opcode;
	u8 opr1;
	u8 opr2;
} Instruction;

typedef struct CpuState {
	u16 PC;
	u16 SP;
	u16 BC;
	u16 DE;
	u16 HL;
	union {
		u16 val;
		struct {
			u8 A;
			union {
				u8 F;
				struct {
					u8 unused0 : 1;
					u8 unused1 : 1;
					u8 unused2 : 1;
					u8 unused3 : 1;
					u8 c : 1;
					u8 h : 1;
					u8 n : 1;
					u8 z : 1;
				};
			};
		};
	} AF;
	std::array<u8, 4> romData;		// store 4 next ROM bytes, start from the address PC.
	Instruction currInstr;
} CpuState;

typedef enum {
	FLAG_Z = (1U << 7),
	FLAG_N = (1U << 6),
	FLAG_H = (1U << 5),
	FLAG_C = (1U << 4),
} CpuFlag;

typedef struct RegisterPair {
private:
	union {
		u16 val;
		struct {
			u8 lowByte;
			u8 highByte;
		};
	};
public:
	RegisterPair& operator=(const u16 other)
	{
		val = other;
		return *this;
	}
	RegisterPair& operator=(const RegisterPair& other)
	{
		this->val = other.val;
		return *this;
	}
	RegisterPair& operator+=(const u8 num)
	{
		val += num;
		return *this;
	}
	RegisterPair& operator^=(const u8 num)
	{
		val ^= num;
		return *this;
	}
	RegisterPair& operator-=(const u8 num)
	{
		val -= num;
		return *this;
	}
	RegisterPair& operator++(int)
	{
		val++;
		return *this;
	}
	RegisterPair& operator--(int)
	{
		val--;
		return *this;
	}
	u8& LB() { return lowByte; }
	u8& HB() { return highByte; }
	u16& Get() { return val; }
} RegPair;

typedef struct AFRegisterPair {
private:
	union {
		u16 val;
		struct {
			u8 a;
			union {
				u8 f;
				struct {
					u8 unused0 : 1;
					u8 unused1 : 1;
					u8 unused2 : 1;
					u8 unused3 : 1;
					u8 c : 1;
					u8 h : 1;
					u8 n : 1;
					u8 z : 1;
				};
			};
		};
	};
public:
	u8& LB() { return f; }
	u8& HB() { return a; }
	u16& Get() { return val; }
} AFRegPair;

typedef struct CpuRegisters {
private:
	AFRegPair af;
	RegPair bc;
	RegPair de;
	RegPair hl;
	RegPair sp;
	RegPair pc;
public:
	u16& AF() { return af.Get(); }
	u16& BC() { return bc.Get(); }
	u16& DE() { return de.Get(); }
	u16& HL() { return hl.Get(); }
	u16& SP() { return sp.Get(); }
	u16& PC() { return pc.Get(); }
	u8& A() { return af.HB(); }
	u8& F() { return af.LB(); }
	u8& B() { return bc.HB(); }
	u8& C() { return bc.LB(); }
	u8& D() { return de.HB(); }
	u8& E() { return de.LB(); }
	u8& H() { return hl.HB(); }
	u8& L() { return hl.LB(); }
} CpuRegs;

class Cpu {
private:
	CpuState state;
	int mCycles;
	CpuRegs regs;
	Bus* bus = nullptr;

	void StackPush(u8);
	u8 StackPop();
	void SetZNHC(bool, bool, bool, bool);
	u16& GetR16FromOpcode(u8);
	u8& GetR8FromOpcode(u8);
	/*
		The idea of grouping instructions into blocks is based on https://gbdev.io/pandocs/CPU_Instruction_Set.html.
		Basically all instructions in a block can be decoded the same way, so we can use that feature to reduce LOCs.
	*/
	u16& DecodeR16MemBlock0(u8);
	u8& DecodeR8Block1(u8);
	void FetchInstruction(Rom&);

	void NOP();

	/* load instructions */
	void LD_R16_U16();
	void LD_IR16_A();
	void LD_A_IR16();
	void LD_R8_U8();
	void LD_IHL_U8();
	void LD_R8_R8();
	void LD_R8_IHL();
	/* increment/decrement instructions */
	void INC_R16();
	void INC_R8();
	void INC_IHL();
	void DEC_R8();
	void DEC_IHL();

	/* bit shift instructions */

	/* jumps and subroutine instructions */
	bool CheckSubroutineCond(u8);
	void JR_COND();
	void RET_COND();
	/* 8-bit arithmetic instructions */
	void ADD_A_R8();
	void ADC_A_R8();
	void SUB_A_R8();
	void SBC_A_R8();
	void AND_A_R8();
	void OR_A_R8();
	void XOR_A_R8();
	void CP_A_R8();

	/* rotate instructions */
	void RLCA();
	void RLA();
	void RRCA();
	void RRA();

	/* stack manipulation instructions */
	void PushWord(u16);
	u16 PopWord();
	void PUSH_R16();
	void POP_R16();
	void RST();

protected:
public:
	CpuState GetCpuState() const;
	void SetFlag(CpuFlag flag, bool val);
	bool GetFlag(CpuFlag flag);
	int Step(Rom&);
	Cpu(Bus *);
	~Cpu();
};