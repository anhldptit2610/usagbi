add_executable(usagbi usagbi.cpp)

set_property(TARGET usagbi PROPERTY
    VS_DEBUGGER_COMMAND_ARGUMENTS "C:/Users/Admin/Documents/emulator/roms/GameBoy/Tetris.gb"
)

target_link_libraries(usagbi PRIVATE
    gb_core
    gb_utils
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)