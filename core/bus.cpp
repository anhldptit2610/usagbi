#include "bus.h"

/*
	For now we only think about memory as a plain array.
*/

void Bus::Write(const u16 addr, const u8 val)
{
	if (cpuInstrTest) {
		memory[addr] = val;
	} else {
		memory[addr] = val;
	}
}

u8 Bus::Read(const u16 addr)
{
	u8 ret;

	if (cpuInstrTest) {
		ret = memory[addr];
	} else {
		if (addr >= 0x0000 && addr <= 0x7FFF) {
			ret = rom->Read(addr);
		} else if (addr == 0xFF44) {
			ret = 0x90;
		} else {
			ret = memory[addr];
		}
	}
	return ret;
}

Bus::Bus()
{
	cpuInstrTest = true;
}

Bus::Bus(Rom* pRom) : rom(pRom)
{
	
}

Bus::~Bus()
{

}