#include "emulator.h"

int main(int argc, char* argv[])
{
	if (argc < 2) {
		spdlog::error("Usage: {} <path_to_rom>", argv[0]);
		return EXIT_FAILURE;
	}

	Emulator emu(const_cast<const char *>(argv[1]));

	if (emu.Load(argv[1]) == STT_FAILED)
		return EXIT_FAILURE;
	emu.Run();
	return 0;
}
