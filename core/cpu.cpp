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
	switch (opcode >> 4) {
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

CpuState Cpu::GetCpuState() const
{
	return state;
}

void Cpu::FetchInstruction(Rom& rom)
{
	state.currInstr.opcode = rom.Read(regs.PC());
	regs.PC() += 1;
	this->state.currInstr.opr1 = rom.Read(regs.PC() + 1);
	this->state.currInstr.opr2 = rom.Read(regs.PC() + 2);
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


int Cpu::Step(Rom& rom)
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

	FetchInstruction(rom);
	mCycles = 0;
	switch (state.currInstr.opcode) {
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
	case 0x70:
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
	case 0x77:
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
	case 0xAE:
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
	case 0xC0:
	case 0xD0:
	case 0xC8:
	case 0xD8:
		RET_COND();
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