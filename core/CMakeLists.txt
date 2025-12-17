add_library(gb_core STATIC 
	bus.cpp
	cpu.cpp
	rom.cpp
	emulator.cpp
)

target_link_libraries(gb_core PRIVATE
	spdlog::spdlog
	gb_utils
)

target_include_directories(gb_core PUBLIC
	${CMAKE_SOURCE_DIR}/utils
)