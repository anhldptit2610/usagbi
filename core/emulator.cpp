#include <fstream>
#include <filesystem>
#include <string>
#include "emulator.h"
#include "logger.h"

#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
using json = nlohmann::json;


int Emulator::Load(const char* romPath)
{
	return rom.Load(romPath);
}

void Emulator::Run()
{
	// create a JSON object
    json j =
    {
        {"pi", 3.141},
        {"happy", true},
        {"name", "Niels"},
        {"nothing", nullptr},
        {
            "answer", {
                {"everything", 42}
            }
        },
        {"list", {1, 0, 2}},
        {
            "object", {
                {"currency", "USD"},
                {"value", 42.99}
            }
        }
    };

    // add new values
    j["new"]["key"]["value"] = {"another", "list"};

    // count elements
    auto s = j.size();
    j["size"] = s;

    // pretty print with indent of 4 spaces
    std::cout << std::setw(4) << j << '\n';
	while(1) {
#ifdef LOGGER_ENABLE
		logger.LogCpuState(cpu.GetCpuState());
#endif
		if (cpu.Step() == -1)
			break;
	}
}

Emulator::Emulator(const char *romPath) : rom(), bus(&rom), cpu(&bus), logger()
{

}

Emulator::~Emulator()
{

}	