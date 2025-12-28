#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <cpu.h>
#include <common.h>
#include <bus.h>


using json = nlohmann::json;

void ParseTestCase(CpuState& beforeState, CpuState& afterState, const json& tests)
{
	beforeState.PC = tests["initial"]["pc"];
	beforeState.SP = tests["initial"]["sp"];
	beforeState.AF.A = tests["initial"]["a"];
	beforeState.BC = U16(tests["initial"]["c"], tests["initial"]["b"]);
	beforeState.DE = U16(tests["initial"]["e"], tests["initial"]["d"]);
	beforeState.AF.F = tests["initial"]["f"];
	beforeState.HL = U16(tests["initial"]["l"], tests["initial"]["h"]);
	for (const auto& i : tests["initial"]["ram"]) {
		std::pair<u16, u8> entry = {i[0], i[1]};
		beforeState.mem.push_back(entry);
	}
	afterState.PC = tests["final"]["pc"];
	afterState.SP = tests["final"]["sp"];
	afterState.AF.A = tests["final"]["a"];
	afterState.BC = U16(tests["final"]["c"], tests["final"]["b"]);
	afterState.DE = U16(tests["final"]["e"], tests["final"]["d"]);
	afterState.AF.F = tests["final"]["f"];
	afterState.HL = U16(tests["final"]["l"], tests["final"]["h"]);
	for (const auto& i : tests["final"]["ram"]) {
		std::pair<u16, u8> entry = {i[0], i[1]};
		afterState.mem.push_back(entry);
	}
}

void PrintState(const CpuState& state)
{
	spdlog::info("Regs info - PC:0x{0:x} SP:0x{1:x} A:0x{2:x} F:0x{3:x} B:0x{4:x} C:0x{5:x} D:0x{6:x} E:0x{7:x} H:0x{8:x} L:0x{9:x}", 
		state.PC, state.SP, state.AF.A, state.AF.F, MSB(state.BC), LSB(state.BC), MSB(state.DE), LSB(state.DE), MSB(state.HL), LSB(state.HL));
	spdlog::info("---- Mems info -----");
	for (const auto& i : state.mem)
		spdlog::info("0x{0:x} - 0x{1:x}", i.first, i.second);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		spdlog::error("Usage: {} <json_file>", argv[0]);
		return EXIT_FAILURE;
	}

	Bus bus;
	Cpu cpu(&bus);
	std::ifstream fs(argv[1]);
	json testCases = json::parse(fs);
	
	for (const auto& i : testCases) {
		CpuState beforeState, afterState;
		ParseTestCase(beforeState, afterState, i);
		cpu.SetCpuState(beforeState);
		if (cpu.Step() == -1) {
			spdlog::error("Test {} failed. Opcode has not implemented yet.");
			return EXIT_FAILURE;
		}
		if (!cpu.CompareCpuState(afterState)) {
			spdlog::error("Tests {} fails at case {}", argv[1], static_cast<std::string>(i["name"]));
			spdlog::error("before state:");
			PrintState(beforeState);
			spdlog::error("cpu state after running instruction: ");
			PrintState(cpu.GetCpuStateForDebug(afterState));
			spdlog::error("desired state: ");
			PrintState(afterState);
			fs.close();
			return EXIT_FAILURE;
		}
	}
	spdlog::info("Test {} passed", argv[1]);
	fs.close();
	return 0;
}