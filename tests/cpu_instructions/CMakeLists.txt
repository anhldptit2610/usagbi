add_executable(cpu_instr_test cpu_instructions_tests.cpp)

set_property(TARGET cpu_instr_test PROPERTY
    VS_DEBUGGER_COMMAND_ARGUMENTS "C:/Users/Admin/Documents/emulator/roms/GameBoy/Tetris.gb"
)

target_link_libraries(cpu_instr_test PRIVATE
	nlohmann_json::nlohmann_json
)