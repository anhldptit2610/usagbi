#include "cpu.h"
#include <array>
#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <spdlog/spdlog.h>

#define OPCODE_TBL_SIZE			256
#define OPCODE_UNKNOWN			-1

#define FLAG_NO_SET				2

std::array<u8, 256> mainOpcodeMCycles = {
    // 0x0_
    1, 3, 2, 2, 1, 1, 2, 1, 5, 2, 2, 2, 1, 1, 2, 1,
    // 0x1_
    1, 3, 2, 2, 1, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 1,
    // 0x2_
    3, 3, 2, 2, 1, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 1,
    // 0x3_
    3, 3, 2, 2, 3, 3, 3, 1, 3, 2, 2, 2, 1, 1, 2, 1,
    // 0x4_
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    // 0x5_
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    // 0x6_
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    // 0x7_
    2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1,
    // 0x8_
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    // 0x9_
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    // 0xA_
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    // 0xB_
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    // 0xC_
    5, 3, 4, 4, 6, 4, 2, 4, 5, 4, 4, 1, 6, 6, 2, 4,
    // 0xD_
    5, 3, 4, 1, 6, 4, 2, 4, 5, 4, 4, 1, 6, 1, 2, 4,
    // 0xE_
    3, 3, 2, 1, 1, 4, 2, 4, 4, 1, 4, 1, 1, 1, 2, 4,
    // 0xF_
    3, 3, 2, 1, 1, 4, 2, 4, 3, 2, 4, 1, 1, 1, 2, 4
};

static enum COND {
	COND_NZ = 0,
	COND_Z,
	COND_NC,
	COND_C
};

static enum R16MEM {
	R16MEM_BC = 0,
	R16MEM_DE,
	R16MEM_HLI,
	R16MEM_HLD,
};

void Cpu::SetFlag(CpuFlag flag, bool val)
{
	regs.F() = (val) ? (regs.F() | flag) : (regs.F() & ~flag);
}

bool Cpu::GetFlag(CpuFlag flag)
{
	return (regs.F() & flag) != 0;
}

void Cpu::SetZNHC(bool z, bool n, bool h, bool c)
{
	SetFlag(FLAG_Z, z);
	SetFlag(FLAG_N, n);
	SetFlag(FLAG_H, h);
	SetFlag(FLAG_C, c);
}

void Cpu::StackPush(u8 val)
{
	bus->Write(regs.SP(), val);
	regs.SP()--;
}

u8 Cpu::StackPop()
{
	regs.SP()++;
	return bus->Read(regs.SP());
}

u16& Cpu::GetR16FromOpcode(u8 opcode)
{
	switch (opcode >> 4) {
	case 0x00:
		return regs.BC();
	case 0x01:
		return regs.DE();
	case 0x02:
		return regs.HL();
	case 0x03:
		return regs.SP();
	default:
		spdlog::error("GetR16FromOpcode: Invalid opcode {:02X}", opcode);
		throw std::runtime_error("GetR16FromOpcode: Invalid opcode");
	}
}

u16& Cpu::DecodeR16MemBlock0(u8 opcode)
{
	switch ((opcode >> 4) & 0x03) {
	case 0x00:
		return regs.BC();
	case 0x01:
		return regs.DE();
	case 0x02:
	case 0x03:
		return regs.HL();
	default:
		spdlog::error("DecodeR16MemBlock0: Invalid opcode {:02X}", opcode);
		throw std::runtime_error("DecodeR16MemBlock0: Invalid opcode");
	}
}

u8& Cpu::GetR8FromOpcode(u8 opcode)
{
	switch ((opcode >> 2) & 0x0F) {
	case 1:
		return regs.B();
	case 3:
		return regs.C();
	case 5:
		return regs.D();
	case 7:
		return regs.E();
	case 9:
		return regs.H();
	case 11:
		return regs.L();
	case 15:
		return regs.A();
	default:
		spdlog::error("GetR8FromOpcode: Invalid opcode {:02X}", opcode);
		throw std::runtime_error("GetR8FromOpcode: Invalid opcode");
	}
}

u8& Cpu::DecodeR8Block1(u8 byte)
{
	switch ((byte & 0x07)) {
	case 0:
		return regs.B();
	case 1:
		return regs.C();
	case 2:
		return regs.D();
	case 3:
		return regs.E();
	case 4:
		return regs.H();
	case 5:
		return regs.L();
	case 7:
		return regs.A();
	default:
		spdlog::error("DecodeR8Block1: Invalid byte {:02X}", byte);
		throw std::runtime_error("DecodeR8Block1: Invalid byte");
	}
}

CpuState Cpu::GetCpuRegState() const
{
	return state;
}

CpuState Cpu::GetCpuStateForDebug(const CpuState& state)
{
	CpuState cpuState;

	cpuState.PC = regs.PC();
	cpuState.SP = regs.SP();
	cpuState.BC = regs.BC();
	cpuState.DE = regs.DE();
	cpuState.HL = regs.HL();
	cpuState.AF.val = regs.AF();

	for (auto& i : state.mem)
		cpuState.mem.push_back(std::pair<u16, u8>(i.first, bus->Read(i.first)));
	return cpuState;
}

void Cpu::SetCpuState(const CpuState& state)
{
	regs.PC() = state.PC;
	regs.SP() = state.SP;
	regs.BC() = state.BC;
	regs.DE() = state.DE;
	regs.HL() = state.HL;
	regs.AF() = state.AF.val;

	for (auto& i : state.mem)
		bus->Write(i.first, i.second);
}

bool Cpu::CompareCpuState(const CpuState& state)
{
	bool ret = true;

	ret = (regs.PC() == state.BC) && (regs.SP() == state.SP) && (regs.BC() == state.BC)
			&& (regs.DE() == state.DE) && (regs.AF() == state.AF.val) && (regs.HL() == state.HL);
	for (auto& i : state.mem)
		ret = (i.second == bus->Read(i.first));
	return ret;
}

void Cpu::FetchInstruction()
{
	state.currInstr.opcode = bus->Read(regs.PC());
	regs.PC() += 1;
	this->state.currInstr.opr1 = bus->Read(regs.PC());
	this->state.currInstr.opr2 = bus->Read(regs.PC() + 1);
}


/*
	Instruction:	NOP
	Usage:			No operation.
	Cost:			1 CPU cycle
*/
void Cpu::NOP()
{

}

void Cpu::LD_R16_U16()
{
	u16& r16 = GetR16FromOpcode(state.currInstr.opcode);

	r16 = U16(this->state.currInstr.opr1, this->state.currInstr.opr2);
	regs.PC() += 2;
}

void Cpu::LD_IR16_A()
{
	u16& r16 = DecodeR16MemBlock0(state.currInstr.opcode);

	bus->Write(r16, regs.A());
	if ((state.currInstr.opcode >> 4) == R16MEM_HLI)
		regs.HL() += 1;
	else if ((state.currInstr.opcode >> 4) == R16MEM_HLD)
		regs.HL() -= 1;
}

void Cpu::LD_A_IR16()
{
	u16& r16 = DecodeR16MemBlock0(state.currInstr.opcode);

	regs.A() = bus->Read(r16);
	if ((state.currInstr.opcode >> 4) == R16MEM_HLI)
		regs.HL() += 1;
	else if ((state.currInstr.opcode >> 4) == R16MEM_HLD)
		regs.HL() -= 1;
}

void Cpu::LD_R8_U8()
{
	GetR8FromOpcode(state.currInstr.opcode) = this->state.currInstr.opr1;
}

void Cpu::LD_IHL_U8()
{
	bus->Write(regs.HL(), this->state.currInstr.opr1);
}

void Cpu::INC_R16()
{
	u16& r16 = GetR16FromOpcode(state.currInstr.opcode);

	r16 += 1;
}

void Cpu::INC_R8()
{
	u8& r8 = GetR8FromOpcode(state.currInstr.opcode);

	SetFlag(FLAG_H, (r8 & 0x0F) + (1U & 0x0F) > 0x0F);
	r8 += 1;
	SetZNHC(r8 == 0, false, GetFlag(FLAG_H), GetFlag(FLAG_C));
}

void Cpu::INC_IHL()
{	
	u8 val = bus->Read(regs.HL());

	SetFlag(FLAG_H, (val & 0x0F) + (1U & 0x0F) > 0x0F);
	val += 1;
	bus->Write(regs.HL(), val);
	SetZNHC(val == 0, false, GetFlag(FLAG_H), GetFlag(FLAG_C));
}

void Cpu::DEC_R8()
{
	u8& r8 = GetR8FromOpcode(state.currInstr.opcode);
	u8 res = r8 - 1, carryPerBit = res ^ r8 ^ 0xFF;

	r8 -= 1;
	SetZNHC(r8 == 0, true, !NTHBIT(carryPerBit, 4), GetFlag(FLAG_C));
}

void Cpu::DEC_IHL()
{
	u8 val = bus->Read(regs.HL()), res = val - 1, carryPerBit = res ^ val ^ 0xFF;

	val -= 1;
	bus->Write(regs.HL(), val);
	SetZNHC(val == 0, false, !NTHBIT(carryPerBit, 4), GetFlag(FLAG_C));
}

void Cpu::LD_R8_R8()
{
	u8& destR8 = DecodeR8Block1(state.currInstr.opcode >> 3), 
		srcR8 = DecodeR8Block1(state.currInstr.opcode);
	
	destR8 = srcR8;
}

void Cpu::LD_R8_IHL()
{
	u8& r8 = DecodeR8Block1(state.currInstr.opcode >> 3);

	r8 = bus->Read(regs.HL());
}

bool Cpu::CheckSubroutineCond(u8 opcode)
{
	bool cond = false;

	switch ((opcode >> 3) & 0x03) {
		case COND_NZ:
			cond = !GetFlag(FLAG_Z);
			break;
		case COND_Z:
			cond = GetFlag(FLAG_Z);
			break;
		case COND_NC:
			cond = !GetFlag(FLAG_C);
			break;
		case COND_C:
			cond = GetFlag(FLAG_C);
			break;
	}
	return cond;
}

void Cpu::JR_COND()
{
	bool jump = false;

	if (CheckSubroutineCond(state.currInstr.opcode)) {
		mCycles += 1;
		regs.PC() += (i8)state.currInstr.opr1;
	}
}

void Cpu::RET_COND()
{
	bool ret = false;
	u16 pc;

	if (CheckSubroutineCond(state.currInstr.opcode)) {
		mCycles += 3;
		pc = PopWord();
		regs.PC() = pc;
	}
}

void Cpu::RLA()
{
	u8 flagC = GetFlag(FLAG_C);

	SetZNHC(0, 0, 0, NTHBIT(regs.A(), 7));
	regs.A() = (regs.A() << 1) | flagC;
}

void Cpu::RLCA()
{
	SetZNHC(0, 0, 0, NTHBIT(regs.A(), 7));
	regs.A() = (regs.A() << 1) | (NTHBIT(regs.A(), 7));
}

void Cpu::RRA()
{
	u8 flagC = GetFlag(FLAG_C);

	SetZNHC(0, 0, 0, NTHBIT(regs.A(), 0));
	regs.A() = (regs.A() >> 1) | (flagC << 7);
}

void Cpu::RRCA()
{
	SetZNHC(0, 0, 0, NTHBIT(regs.A(), 0));
	regs.A() = (regs.A() >> 1) | ((NTHBIT(regs.A(), 0)) << 7);
}

void Cpu::ADD_A_R8()
{
	u8& r8 = DecodeR8Block1(state.currInstr.opcode);
	u16 res = regs.A() + r8, carryPerBit = regs.A() ^ r8 ^ res;

	regs.A() = res & 0x00FF;
	SetZNHC(!regs.A(), 0, NTHBIT(carryPerBit, 4), NTHBIT(carryPerBit, 8));
}

void Cpu::ADC_A_R8()
{
	u8& r8 = DecodeR8Block1(state.currInstr.opcode);
	u16 res = regs.A() + r8 + GetFlag(FLAG_C), carryPerBit = regs.A() ^ r8 ^ res;

	regs.A() = res;
	SetZNHC(!regs.A(), 0, NTHBIT(carryPerBit, 4), NTHBIT(carryPerBit, 8));
}

void Cpu::SUB_A_R8()
{
	u8& r8 = DecodeR8Block1(state.currInstr.opcode);
	u16 res = regs.A() + ~r8 + 1, carryPerBit = regs.A() ^ ~r8 ^ res;

	regs.A() = res & 0x00FF;
	SetZNHC(!regs.A(), 1, !NTHBIT(carryPerBit, 4), !NTHBIT(carryPerBit, 8));
}

void Cpu::SBC_A_R8()
{
	u8& r8 = DecodeR8Block1(state.currInstr.opcode);
	u16 res = regs.A() + ~r8 + 1 - GetFlag(FLAG_C), carryPerBit = regs.A() ^ ~r8 ^ res;

	regs.A() = res;
	SetZNHC(!regs.A(), 0, !NTHBIT(carryPerBit, 4), !NTHBIT(carryPerBit, 8));
}

void Cpu::AND_A_R8()
{
	regs.A() &= DecodeR8Block1(state.currInstr.opcode);
	SetZNHC(!regs.A(), 0, 1, 0);
}

void Cpu::OR_A_R8()
{
	regs.A() |= DecodeR8Block1(state.currInstr.opcode);
	SetZNHC(!regs.A(), 0, 0, 0);
}

void Cpu::XOR_A_R8()
{
	regs.A() ^= DecodeR8Block1(state.currInstr.opcode);
	SetZNHC(!regs.A(), 0, 0, 0);
}

void Cpu::CP_A_R8()
{
	u8& r8 = DecodeR8Block1(state.currInstr.opcode);
	u16 res = regs.A() + ~r8 + 1, carryPerBit = regs.A() ^ ~r8 ^ res;

	SetZNHC(!res, 1, NTHBIT(carryPerBit, 4), NTHBIT(carryPerBit, 8));
}

void Cpu::PushWord(u16 word)
{
	regs.SP()--;
	bus->Write(regs.SP(), MSB(word));
	regs.SP()--;
	bus->Write(regs.SP(), LSB(word));
}

u16 Cpu::PopWord()
{
	u8 lsb, msb;

	lsb = bus->Read(regs.SP());
	regs.SP()++;
	msb = bus->Read(regs.SP());
	regs.SP()++;
	return U16(lsb, msb);
}

void Cpu::PUSH_R16()
{
	u8 msb, lsb;

	msb = (state.currInstr.opcode == 0xF5) ? regs.A() : MSB(DecodeR16MemBlock0(state.currInstr.opcode));
	lsb = (state.currInstr.opcode == 0xF5) ? regs.F() : LSB(DecodeR16MemBlock0(state.currInstr.opcode));
	PushWord(U16(lsb, msb));
}

void Cpu::POP_R16()
{
	u16 word = PopWord();

	if (state.currInstr.opcode == 0xF1) {
		regs.AF() = word;
	} else {
		u16& r16 = DecodeR16MemBlock0(state.currInstr.opcode);
		r16 = word;
	}
}

void Cpu::RST()
{
	PushWord(regs.PC());
	regs.PC() = (state.currInstr.opcode >> 3) & 0x03;
}

void Cpu::LD_IR16_SP()
{
	u16 r16 = U16(state.currInstr.opr1, state.currInstr.opr2);

	bus->Write(r16, LSB(regs.SP()));
	bus->Write(r16 + 1, MSB(regs.SP()));
	regs.PC() += 2;
}

void Cpu::ADD_HL_R16()
{
	u16 r16 = GetR16FromOpcode(state.currInstr.opcode);
	u32 res = r16 + regs.HL(), carryPerBit = r16 ^ regs.HL() ^ res;

	regs.HL() = (u16)(res & 0x0000ffff);
	SetFlag(FLAG_H, NTHBIT(carryPerBit, 11));
	SetFlag(FLAG_C, NTHBIT(carryPerBit, 15));
}

void Cpu::DEC_R16()
{
	u16& r16 = GetR16FromOpcode(state.currInstr.opcode);

	r16 -= 1;
}

void Cpu::STOP()
{
}

void Cpu::DAA()
{
	u8 A = regs.A();

	if (!GetFlag(FLAG_N)) {
		if (GetFlag(FLAG_H) || regs.A() > 0x09)
			A += 0x06;
		if (GetFlag(FLAG_C) || regs.A() > 0x99) {
			A += 0x60;
			SetFlag(FLAG_C, 1);
		}
	} else {
		if (GetFlag(FLAG_H))
			A -= 0x06;
		if (GetFlag(FLAG_C))
			A -= 0x60;
	}
	SetFlag(FLAG_Z, !A);
	SetFlag(FLAG_H, 0);
	regs.A() = A;
}

void Cpu::CPL()
{
	regs.A() = ~regs.A();
	SetZNHC(GetFlag(FLAG_Z), 1, 1, GetFlag(FLAG_C));
}

void Cpu::SCF()
{
	SetFlag(FLAG_C, 1);
}

void Cpu::CCF()
{
	SetFlag(FLAG_C, 0);
}

void Cpu::LD_IHL_R8()
{
	u8 r8 = DecodeR8Block1(state.currInstr.opcode);

	bus->Write(regs.HL(), r8);
}

void Cpu::HALT()
{
	// TODO
}

void Cpu::ADD_A_IHL()
{
	u8 val = bus->Read(regs.HL());
	u16 res = regs.A() + val, carryPerBit = regs.A() ^ val ^ res;

	regs.A() = res & 0x00FF;
	SetZNHC(!regs.A(), 0, NTHBIT(carryPerBit, 4), NTHBIT(carryPerBit, 8));
}

void Cpu::SUB_A_IHL()
{
	u8 val = bus->Read(regs.HL());
	u16 res = regs.A() + ~val + 1, carryPerBit = regs.A() ^ ~val ^ res;

	regs.A() = res & 0x00FF;
	SetZNHC(!regs.A(), 1, !NTHBIT(carryPerBit, 4), !NTHBIT(carryPerBit, 8));
}

void Cpu::AND_A_IHL()
{
	regs.A() &= bus->Read(regs.HL());
	SetZNHC(!regs.A(), 0, 0, 0);
}

void Cpu::OR_A_IHL()
{
	regs.A() |= bus->Read(regs.HL());
	SetZNHC(!regs.A(), 0, 0, 0);
}

void Cpu::ADC_A_IHL()
{
	u8 val = bus->Read(regs.HL());
	u16 res = regs.A() + val + GetFlag(FLAG_C), carryPerBit = regs.A() ^ val ^ res;

	regs.A() = res;
	SetZNHC(!regs.A(), 0, NTHBIT(carryPerBit, 4), NTHBIT(carryPerBit, 8));
}

void Cpu::SBC_A_IHL()
{
	u8 val = bus->Read(regs.HL());
	u16 res = regs.A() + ~val + 1 - GetFlag(FLAG_C), carryPerBit = regs.A() ^ ~val ^ res;

	regs.A() = res;
	SetZNHC(!regs.A(), 0, !NTHBIT(carryPerBit, 4), !NTHBIT(carryPerBit, 8));
}

void Cpu::XOR_A_IHL()
{
	regs.A() ^= bus->Read(regs.HL());
	SetZNHC(!regs.A(), 0, 0, 0);
}

void Cpu::CP_A_IHL()
{
	u8 val = bus->Read(regs.HL());
	u16 res = regs.A() + ~val + 1, carryPerBit = regs.A() ^ ~val ^ res;

	SetZNHC(!res, 1, NTHBIT(carryPerBit, 4), NTHBIT(carryPerBit, 8));
}

void Cpu::ADD_A_U8()
{
	u8 val = state.currInstr.opr1;
	u16 res = regs.A() + val, carryPerBit = regs.A() ^ val ^ res;

	regs.A() = res & 0x00FF;
	regs.PC() += 1;
	SetZNHC(!regs.A(), 0, NTHBIT(carryPerBit, 4), NTHBIT(carryPerBit, 8));
}

void Cpu::SUB_A_U8()
{
	u8 val = state.currInstr.opr1;	
	regs.PC() += 1;
	u16 res = regs.A() + ~val + 1, carryPerBit = regs.A() ^ ~val ^ res;

	regs.A() = res & 0x00FF;
	SetZNHC(!regs.A(), 1, !NTHBIT(carryPerBit, 4), !NTHBIT(carryPerBit, 8));
}

void Cpu::AND_A_U8()
{
	u8 val = state.currInstr.opr1;	
	regs.PC() += 1;
	regs.A() &= bus->Read(regs.HL());
	SetZNHC(!regs.A(), 0, 0, 0);
}

void Cpu::OR_A_U8()
{	
	u8 val = state.currInstr.opr1;	
	regs.PC() += 1;
	regs.A() |= val;
	SetZNHC(!regs.A(), 0, 0, 0);
}

void Cpu::ADC_A_U8()
{
	u8 val = state.currInstr.opr1;
	regs.PC() += 1;
	u16 res = regs.A() + val + GetFlag(FLAG_C), carryPerBit = regs.A() ^ val ^ res;

	regs.A() = res;
	SetZNHC(!regs.A(), 0, NTHBIT(carryPerBit, 4), NTHBIT(carryPerBit, 8));
}

void Cpu::SBC_A_U8()
{
	u8 val = state.currInstr.opr1;
	regs.PC() += 1;
	u16 res = regs.A() + ~val + 1 - GetFlag(FLAG_C), carryPerBit = regs.A() ^ ~val ^ res;

	regs.A() = res;
	SetZNHC(!regs.A(), 0, !NTHBIT(carryPerBit, 4), !NTHBIT(carryPerBit, 8));
}

void Cpu::XOR_A_U8()
{
	regs.A() ^= state.currInstr.opr1;
	SetZNHC(!regs.A(), 0, 0, 0);
}

void Cpu::CP_A_U8()
{
	u8 val = state.currInstr.opr1;
	u16 res = regs.A() + ~val + 1, carryPerBit = regs.A() ^ ~val ^ res;

	SetZNHC(!res, 1, NTHBIT(carryPerBit, 4), NTHBIT(carryPerBit, 8));
}

void Cpu::RET()
{
	if (state.currInstr.opcode == 0xC9 || CheckSubroutineCond(state.currInstr.opcode)) {
		u16 pc = PopWord();
		regs.PC() = pc;
		mCycles += 3;
	}
}

void Cpu::JP()
{
	if (state.currInstr.opcode == 0xC3 || CheckSubroutineCond(state.currInstr.opcode)) {
		mCycles += 1;
		regs.PC() = U16(state.currInstr.opr1, state.currInstr.opr2);
	}
}

void Cpu::CALL()
{
	regs.PC() += 2;
	if (state.currInstr.opcode == 0xCD || CheckSubroutineCond(state.currInstr.opcode)) {
		mCycles += 3;
		PushWord(regs.PC());
		regs.PC() = U16(state.currInstr.opr1, state.currInstr.opr2);
	}
}

void Cpu::RLC()
{
	u8 val = (state.currInstr.opr1 == 0x06) ? bus->Read(regs.HL()) : DecodeR8Block1(state.currInstr.opr1);

	SetFlag(FLAG_C, NTHBIT(val, 7));
	val = (val << 1) | GetFlag(FLAG_C);
	SetZNHC(!val, 0, 0, GetFlag(FLAG_C));
	if (state.currInstr.opr1 != 0x06) {
		u8& r8 = DecodeR8Block1(state.currInstr.opr1);
		r8 = val;
	} else {
		bus->Write(regs.HL(), val);
	}
}

void Cpu::RRC()
{
	u8 val = (state.currInstr.opr1 == 0x0E) ? bus->Read(regs.HL()) : DecodeR8Block1(state.currInstr.opr1);

	SetFlag(FLAG_C, NTHBIT(val, 0));
	val = (val >> 1) | ((u8)GetFlag(FLAG_C) << 7);
	SetZNHC(!val, 0, 0, GetFlag(FLAG_C));
	if (state.currInstr.opr1 != 0x0E) {
		u8& r8 = DecodeR8Block1(state.currInstr.opr1);
		r8 = val;
	} else {
		bus->Write(regs.HL(), val);
	}
}

void Cpu::RL()
{
	u8 val = (state.currInstr.opr1 == 0x16) ? bus->Read(regs.HL()) : DecodeR8Block1(state.currInstr.opr1), newC = NTHBIT(val, 7);

	val = (val << 1) | GetFlag(FLAG_C);
	if (state.currInstr.opr1 != 0x16) {
		u8& r8 = DecodeR8Block1(state.currInstr.opr1);
		r8 = val;
	} else {
		bus->Write(regs.HL(), val);
	}
	SetZNHC(!val, 0, 0, newC);
}

void Cpu::RR()
{
	u8 val = (state.currInstr.opr1 == 0x1E) ? bus->Read(regs.HL()) : DecodeR8Block1(state.currInstr.opr1), newC = NTHBIT(val, 0);

	val = (val >> 1) | ((u8)GetFlag(FLAG_C) << 7);
	if (state.currInstr.opr1 != 0x1E) {
		u8& r8 = DecodeR8Block1(state.currInstr.opr1);
		r8 = val;
	} else {
		bus->Write(regs.HL(), val);
	}
	SetZNHC(!val, 0, 0, newC);
}

void Cpu::SLA()
{
	u8 val = (state.currInstr.opr1 == 0x26) ? bus->Read(regs.HL()) : DecodeR8Block1(state.currInstr.opr1);

	SetFlag(FLAG_C, NTHBIT(val, 7));
	val <<= 1;
	SetZNHC(!val, 0, 0, GetFlag(FLAG_C));
	if (state.currInstr.opr1 != 0x26) {
		u8& r8 = DecodeR8Block1(state.currInstr.opr1);
		r8 = val;
	} else {
		bus->Write(regs.HL(), val);
	}
}

void Cpu::SRA()
{
	u8 val = (state.currInstr.opr1 == 0x2E) ? bus->Read(regs.HL()) : DecodeR8Block1(state.currInstr.opr1);

	SetFlag(FLAG_C, NTHBIT(val, 0));
	val = (val & 0x80) | (val >> 1);
	SetZNHC(!val, 0, 0, GetFlag(FLAG_C));
	if (state.currInstr.opr1 != 0x2E) {
		u8& r8 = DecodeR8Block1(state.currInstr.opr1);
		r8 = val;
	} else {
		bus->Write(regs.HL(), val);
	}
}

void Cpu::SWAP()
{
	u8 val = (state.currInstr.opr1 == 0x36) ? bus->Read(regs.HL()) : DecodeR8Block1(state.currInstr.opr1);

	val = (val >> 4) | (val << 4);
	SetZNHC(!val, 0, 0, GetFlag(FLAG_C));
	if (state.currInstr.opr1 != 0x36) {
		u8& r8 = DecodeR8Block1(state.currInstr.opr1);
		r8 = val;
	} else {
		bus->Write(regs.HL(), val);
	}
}

void Cpu::SRL()
{
	u8 val = (state.currInstr.opr1 == 0x3E) ? bus->Read(regs.HL()) : DecodeR8Block1(state.currInstr.opr1);

	SetFlag(FLAG_C, NTHBIT(val, 0));
	val >>= 1;
	SetZNHC(!val, 0, 0, GetFlag(FLAG_C));
	if (state.currInstr.opr1 != 0x3E) {
		u8& r8 = DecodeR8Block1(state.currInstr.opr1);
		r8 = val;
	} else {
		bus->Write(regs.HL(), val);
	}
}

void Cpu::BIT()
{
	u8 val = (state.currInstr.opr1 == 0x46 || state.currInstr.opr1 == 0x4E
			|| state.currInstr.opr1 == 0x56 || state.currInstr.opr1 == 0x5E
			|| state.currInstr.opr1 == 0x66 || state.currInstr.opr1 == 0x6E
			|| state.currInstr.opr1 == 0x76 || state.currInstr.opr1 == 0x7E) ? bus->Read(regs.HL()) : DecodeR8Block1(state.currInstr.opr1), n8 = (state.currInstr.opr1 >> 3) & 0x07;

	SetZNHC(NTHBIT(val, n8), 0, 1, GetFlag(FLAG_C));
	if (state.currInstr.opr1 == 0x46 || state.currInstr.opr1 == 0x4E
			|| state.currInstr.opr1 == 0x56 || state.currInstr.opr1 == 0x5E
			|| state.currInstr.opr1 == 0x66 || state.currInstr.opr1 == 0x6E
			|| state.currInstr.opr1 == 0x76 || state.currInstr.opr1 == 0x7E) {
		bus->Write(regs.HL(), val);
	} else {
		u8& r8 = DecodeR8Block1(state.currInstr.opr1);
		r8 = val;
	}
}

void Cpu::RESET()
{
	u8 val = (state.currInstr.opr1 == 0x86 || state.currInstr.opr1 == 0x8E
			|| state.currInstr.opr1 == 0x96 || state.currInstr.opr1 == 0x9E
			|| state.currInstr.opr1 == 0xA6 || state.currInstr.opr1 == 0xAE
			|| state.currInstr.opr1 == 0xB6 || state.currInstr.opr1 == 0xBE) ? bus->Read(regs.HL()) : DecodeR8Block1(state.currInstr.opr1), n8 = (state.currInstr.opr1 >> 3) & 0x07;
	val &= ~(1U << n8);
	if (state.currInstr.opr1 == 0x86 || state.currInstr.opr1 == 0x8E
			|| state.currInstr.opr1 == 0x96 || state.currInstr.opr1 == 0x9E
			|| state.currInstr.opr1 == 0xA6 || state.currInstr.opr1 == 0xAE
			|| state.currInstr.opr1 == 0xB6 || state.currInstr.opr1 == 0xBE) {
		bus->Write(regs.HL(), val);
	} else {
		u8& r8 = DecodeR8Block1(state.currInstr.opr1);
		r8 = val;
	}
}

void Cpu::SETF()
{
	u8 val = (state.currInstr.opr1 == 0xC6 || state.currInstr.opr1 == 0xCE
			|| state.currInstr.opr1 == 0xD6 || state.currInstr.opr1 == 0xDE
			|| state.currInstr.opr1 == 0xE6 || state.currInstr.opr1 == 0xEE
			|| state.currInstr.opr1 == 0xF6 || state.currInstr.opr1 == 0xFE) ? bus->Read(regs.HL()) : DecodeR8Block1(state.currInstr.opr1), n8 = (state.currInstr.opr1 >> 3) & 0x07;
	val |= (1U << n8);
	if (state.currInstr.opr1 == 0xC6 || state.currInstr.opr1 == 0xCE
			|| state.currInstr.opr1 == 0xD6 || state.currInstr.opr1 == 0xDE
			|| state.currInstr.opr1 == 0xE6 || state.currInstr.opr1 == 0xEE
			|| state.currInstr.opr1 == 0xF6 || state.currInstr.opr1 == 0xFE) {
		bus->Write(regs.HL(), val);
	} else {
		u8& r8 = DecodeR8Block1(state.currInstr.opr1);
		r8 = val;
	}
}

int Cpu::RunCBInstruction(u8 opcode)
{
	switch (opcode) {
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		RLC();
		break;
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0c:
	case 0x0d:
	case 0x0e:
	case 0x0f:
		RRC();
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
		RL();
		break;
	case 0x18:
	case 0x19:
	case 0x1a:
	case 0x1b:
	case 0x1c:
	case 0x1d:
	case 0x1e:
	case 0x1f:
		RR();
		break;
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
		SLA();
		break;
	case 0x28:
	case 0x29:
	case 0x2a:
	case 0x2b:
	case 0x2c:
	case 0x2d:
	case 0x2e:
	case 0x2f:
		SRA();
		break;
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
		SWAP();
		break;
	case 0x38:
	case 0x39:
	case 0x3a:
	case 0x3b:
	case 0x3c:
	case 0x3d:
	case 0x3e:
	case 0x3f:
		SRL();
		break;
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x47:
	case 0x48:
	case 0x49:
	case 0x4a:
	case 0x4b:
	case 0x4c:
	case 0x4d:
	case 0x4e:
	case 0x4f:
	case 0x50:
	case 0x51:
	case 0x52:
	case 0x53:
	case 0x54:
	case 0x55:
	case 0x56:
	case 0x57:
	case 0x58:
	case 0x59:
	case 0x5a:
	case 0x5b:
	case 0x5c:
	case 0x5d:
	case 0x5e:
	case 0x5f:
	case 0x60:
	case 0x61:
	case 0x62:
	case 0x63:
	case 0x64:
	case 0x65:
	case 0x66:
	case 0x67:
	case 0x68:
	case 0x69:
	case 0x6a:
	case 0x6b:
	case 0x6c:
	case 0x6d:
	case 0x6e:
	case 0x6f:
	case 0x70:
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
	case 0x76:
	case 0x77:
	case 0x78:
	case 0x79:
	case 0x7a:
	case 0x7b:
	case 0x7c:
	case 0x7d:
	case 0x7e:
	case 0x7f:
		BIT();
		break;
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x87:
	case 0x88:
	case 0x89:
	case 0x8a:
	case 0x8b:
	case 0x8c:
	case 0x8d:
	case 0x8e:
	case 0x8f:
	case 0x90:
	case 0x91:
	case 0x92:
	case 0x93:
	case 0x94:
	case 0x95:
	case 0x96:
	case 0x97:
	case 0x98:
	case 0x99:
	case 0x9a:
	case 0x9b:
	case 0x9c:
	case 0x9d:
	case 0x9e:
	case 0x9f:
	case 0xa0:
	case 0xa1:
	case 0xa2:
	case 0xa3:
	case 0xa4:
	case 0xa5:
	case 0xa6:
	case 0xa7:
	case 0xa8:
	case 0xa9:
	case 0xaa:
	case 0xab:
	case 0xac:
	case 0xad:
	case 0xae:
	case 0xaf:
	case 0xb0:
	case 0xb1:
	case 0xb2:
	case 0xb3:
	case 0xb4:
	case 0xb5:
	case 0xb6:
	case 0xb7:
	case 0xb8:
	case 0xb9:
	case 0xba:
	case 0xbb:
	case 0xbc:
	case 0xbd:
	case 0xbe:
	case 0xbf:
		RESET();
		break;
	case 0xc0:
	case 0xc1:
	case 0xc2:
	case 0xc3:
	case 0xc4:
	case 0xc5:
	case 0xc6:
	case 0xc7:
	case 0xc8:
	case 0xc9:
	case 0xca:
	case 0xcb:
	case 0xcc:
	case 0xcd:
	case 0xce:
	case 0xcf:
	case 0xd0:
	case 0xd1:
	case 0xd2:
	case 0xd3:
	case 0xd4:
	case 0xd5:
	case 0xd6:
	case 0xd7:
	case 0xd8:
	case 0xd9:
	case 0xda:
	case 0xdb:
	case 0xdc:
	case 0xdd:
	case 0xde:
	case 0xdf:
	case 0xe0:
	case 0xe1:
	case 0xe2:
	case 0xe3:
	case 0xe4:
	case 0xe5:
	case 0xe6:
	case 0xe7:
	case 0xe8:
	case 0xe9:
	case 0xea:
	case 0xeb:
	case 0xec:
	case 0xed:
	case 0xee:
	case 0xef:
	case 0xf0:
	case 0xf1:
	case 0xf2:
	case 0xf3:
	case 0xf4:
	case 0xf5:
	case 0xf6:
	case 0xf7:
	case 0xf8:
	case 0xf9:
	case 0xfa:
	case 0xfb:
	case 0xfc:
	case 0xfd:
	case 0xfe:
	case 0xff:
		SETF();
		break;
	default:
		spdlog::error("Opcode invalid - ${:02X}", state.currInstr.opcode);
		return OPCODE_UNKNOWN;
	}
	return 2;
}

void Cpu::RETI()
{
	RET();
	// TODO: interrupt
}

void Cpu::LDH_IA8_A()
{
	regs.PC() += 1;
	bus->Write(0xFF00 + state.currInstr.opr1, regs.A());
}

void Cpu::LDH_IC_A()
{
	bus->Write(0xFF00 + regs.C(), regs.A());
}

void Cpu::LDH_A_IA8()
{
	regs.PC() += 1;
	regs.A() = bus->Read(0xFF00 + state.currInstr.opr1);
}

void Cpu::LDH_A_IC()
{
	regs.A() = bus->Read(0xFF00 +regs.C());
}

void Cpu::ADD_SP_E8()
{
	regs.PC() += 1;
	u16 res = regs.SP() + (i8)state.currInstr.opr1, carryPerBit = res ^ regs.SP() ^ state.currInstr.opr1;

	regs.SP() = res;
	SetZNHC(0, 0, NTHBIT(carryPerBit, 3), NTHBIT(carryPerBit, 7));
}

void Cpu::JP_HL()
{
	regs.PC() = regs.HL();
}

void Cpu::LD_IA16_A()
{
	bus->Write(U16(state.currInstr.opr1, state.currInstr.opr2), regs.A());
	regs.PC() += 2;
}

void Cpu::LD_A_IA16()
{
	regs.A() = bus->Read(U16(state.currInstr.opr1, state.currInstr.opr2));
	regs.PC() += 2;
}

void Cpu::DI()
{
}

void Cpu::EI()
{
}

void Cpu::LD_HL_SP_i8()
{
	regs.PC() += 1;
	u8 s8 = state.currInstr.opr1;
	u16 carryPerBit = (regs.SP() + s8) ^ regs.SP() ^ s8;

	regs.HL() = regs.SP() + (i8)s8;
	SetZNHC(0, 0, NTHBIT(carryPerBit, 4), NTHBIT(carryPerBit, 8));
}

void Cpu::LD_SP_HL()
{
	regs.SP() = regs.HL();
}

int Cpu::Step()
{

#ifdef LOGGER_ENABLE
	state.AF.val = regs.AF();
	state.BC = regs.BC();
	state.DE = regs.DE();
	state.HL = regs.HL();
	state.PC = regs.PC();
	state.SP = regs.SP();
	state.romData[0] = rom.Read(regs.PC());
	state.romData[1] = rom.Read(regs.PC() + 1);
	state.romData[2] = rom.Read(regs.PC() + 2);
	state.romData[3] = rom.Read(regs.PC() + 3);
#endif

	FetchInstruction();
	mCycles = 0;
	switch (state.currInstr.opcode) {
	case 0xF9:
		LD_SP_HL();
		break;
	case 0xF3:
		DI();
		break;
	case 0xFB:
		EI();
		break;
	case 0xF8:
		LD_HL_SP_i8();
		break;
	case 0xEA:
		LD_IA16_A();
		break;
	case 0xFA:
		LD_A_IA16();
		break;
	case 0xE9:
		JP_HL();
		break;
	case 0xE8:
		ADD_SP_E8();
		break;
	case 0xE0:
		LDH_IA8_A();
		break;
	case 0xF0:
		LDH_A_IA8();
		break;
	case 0xE2:
		LDH_IC_A();
		break;
	case 0xF2:
		LDH_A_IC();
		break;
	case 0xD9:
		RETI();
		break;
	case 0xCB:
		mCycles = 2;
		regs.PC() += 1;
		return RunCBInstruction(state.currInstr.opr1);
	case 0xC6:
		ADD_A_U8();
		break;
	case 0xD6:
		SUB_A_U8();
		break;
	case 0xE6:
		AND_A_U8();
		break;
	case 0xF6:
		OR_A_U8();
		break;
	case 0xCE:
		ADC_A_U8();
		break;
	case 0xDE:
		SBC_A_U8();
		break;
	case 0xEE:
		XOR_A_U8();
		break;
	case 0xFE:
		CP_A_U8();
		break;
	case 0xC4:
	case 0xD4:
	case 0xCC:
	case 0xDC:
	case 0xCD:
		CALL();
		break;
	case 0xC2:
	case 0xD2:
	case 0xC3:
	case 0xCA:
	case 0xDA:
		JP();
		break;
	case 0xC0:
	case 0xD0:
	case 0xC8:
	case 0xD8:
	case 0xC9:
		RET();
		break;
	case 0x86:
		ADD_A_IHL();
		break;
	case 0x8E:
		ADC_A_IHL();
		break;
	case 0x96:
		SUB_A_IHL();
		break;
	case 0x9E:
		SBC_A_IHL();
		break;
	case 0xA6:
		AND_A_IHL();
		break;
	case 0xAE:
		XOR_A_IHL();
		break;
	case 0xB6:
		OR_A_IHL();
		break;
	case 0xBE:
		CP_A_IHL();
		break;
	case 0x76:
		HALT();
		break;
	case 0x70:
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
	case 0x77:
		LD_IHL_R8();
		break;
	case 0x3F:
		CCF();
		break;
	case 0x37:
		SCF();
		break;
	case 0x2F:
		CPL();
		break;
	case 0x27:
		DAA();
		break;
	case 0x10:
		STOP();
		break;
	case 0x0B:
	case 0x1B:
	case 0x2B:
	case 0x3B:
		DEC_R16();
		break;
	case 0x09:
	case 0x19:
	case 0x29:
	case 0x39:
		ADD_HL_R16();
		break;
	case 0x08:
		LD_IR16_SP();
		break;
	case 0x00:
		NOP();
		break;
	case 0x01:
	case 0x11:
	case 0x21:
	case 0x31:
		LD_R16_U16();
		break;
	case 0x02:
	case 0x12:
	case 0x22:
	case 0x32:
		LD_IR16_A();
		break;
	case 0x03:
	case 0x13:
	case 0x23:
	case 0x33:
		INC_R16();
		break;
	case 0x04:
	case 0x14:
	case 0x24:
	case 0x0C:
	case 0x1C:
	case 0x2C:
	case 0x3C:
		INC_R8();
		break;
	case 0x34:
		INC_IHL();
		break;
	case 0x05:
	case 0x15:
	case 0x25:
	case 0x0D:
	case 0x1D:
	case 0x2D:
	case 0x3D:
		DEC_R8();
		break;
	case 0x35:
		DEC_IHL();
		break;
	case 0x06:
	case 0x16:
	case 0x26:
	case 0x0E:
	case 0x1E:
	case 0x2E:
	case 0x3E:
		LD_R8_U8();
		break;
	case 0x36:
		LD_IHL_U8();
		break;
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x47:
	case 0x48:
	case 0x49:
	case 0x4A:
	case 0x4B:
	case 0x4C:
	case 0x4D:
	case 0x4F:
	case 0x50:
	case 0x51:
	case 0x52:
	case 0x53:
	case 0x54:
	case 0x55:
	case 0x57:
	case 0x58:
	case 0x59:
	case 0x5A:
	case 0x5B:
	case 0x5C:
	case 0x5D:
	case 0x5F:
	case 0x60:
	case 0x61:
	case 0x62:
	case 0x63:
	case 0x64:
	case 0x65:
	case 0x67:
	case 0x68:
	case 0x69:
	case 0x6A:
	case 0x6B:
	case 0x6C:
	case 0x6D:
	case 0x6F:
	case 0x78:
	case 0x79:
	case 0x7A:
	case 0x7B:
	case 0x7C:
	case 0x7D:
	case 0x7F:
		LD_R8_R8();
		break;
	case 0x46:
	case 0x56:
	case 0x66:
	case 0x4e:
	case 0x5e:
	case 0x6e:
	case 0x7e:
		LD_R8_IHL();
		break;
	case 0x0A:
	case 0x1A:
	case 0x2A:
	case 0x3A:
		LD_A_IR16();
		break;
	case 0x18:
		regs.PC() += (i8)this->state.currInstr.opr1;
		break;
	case 0x20:
	case 0x30:
	case 0x28:
	case 0x38:
		JR_COND();
		break;
	case 0x07:
		RLCA();
		break;
	case 0x17:
		RLA();
		break;
	case 0x0F:
		RRCA();
		break;
	case 0x1F:
		RRA();
		break;
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
	case 0x85:
	case 0x87:
		ADD_A_R8();
		break;
	case 0x88:
	case 0x89:
	case 0x8A:
	case 0x8B:
	case 0x8C:
	case 0x8D:
	case 0x8F:
		ADC_A_R8();
		break;
	case 0x90:
	case 0x91:
	case 0x92:
	case 0x93:
	case 0x94:
	case 0x95:
	case 0x97:
		SUB_A_R8();
		break;
	case 0x98:
	case 0x99:
	case 0x9A:
	case 0x9B:
	case 0x9C:
	case 0x9D:
	case 0x9F:
		SBC_A_R8();
		break;
	case 0xA0:
	case 0xA1:
	case 0xA2:
	case 0xA3:
	case 0xA4:
	case 0xA5:
	case 0xA7:
		AND_A_R8();
		break;
	case 0xA8:
	case 0xA9:
	case 0xAA:
	case 0xAB:
	case 0xAC:
	case 0xAD:
	case 0xAF:
		XOR_A_R8();
		break;
	case 0xB0:
	case 0xB1:
	case 0xB2:
	case 0xB3:
	case 0xB4:
	case 0xB5:
	case 0xB7:
		OR_A_R8();
		break;
	case 0xB8:
	case 0xB9:
	case 0xBA:
	case 0xBB:
	case 0xBC:
	case 0xBD:
	case 0xBF:
		CP_A_R8();
		break;
	case 0xC1:
	case 0xD1:
	case 0xE1:
	case 0xF1:
		POP_R16();
		break;
	case 0xC5:
	case 0xD5:
	case 0xE5:
	case 0xF5:
		PUSH_R16();
		break;
	case 0xC7:
	case 0xD7:
	case 0xE7:
	case 0xF7:
	case 0xCF:
	case 0xDF:
	case 0xEF:
	case 0xFF:
		RST();
		break;
	default:
		spdlog::error("Opcode invalid - ${:02X}", state.currInstr.opcode);
		return OPCODE_UNKNOWN;
	};
	return mCycles;
}

Cpu::Cpu(Bus *pBus) : bus(pBus)
{
	// DMG's registers start up value. Src:
	// https://gbdev.io/pandocs/Power_Up_Sequence.html#power-up-sequence
	//regs.af.a = 0x01;
	//regs.af.f.z = 1;
	//regs.af.f.n = 0;
	//regs.bc.b = 0x00;
	//regs.bc.c = 0x13;
	//regs.de.d = 0x00;
	//regs.de.e = 0xd8;
	//regs.hl.h = 0x01;
	//regs.hl.l = 0x4d;
	//regs.pc = 0x0100;
	//regs.sp = 0xfffe;
	regs.PC() = 0x0000;
}

Cpu::~Cpu()
{

}