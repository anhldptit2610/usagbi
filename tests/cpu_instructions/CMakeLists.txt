add_executable(cpu_instr_test cpu_instructions_tests.cpp)

set_property(TARGET cpu_instr_test PROPERTY
    VS_DEBUGGER_COMMAND_ARGUMENTS "C:/Users/Admin/source/repos/usagbi/tests/cpu_instructions/sm83/v1/00.json"
)

target_link_libraries(cpu_instr_test PRIVATE
	nlohmann_json::nlohmann_json
	spdlog::spdlog
	gb_core
)

target_include_directories(cpu_instr_test PUBLIC
	${CMAKE_SOURCE_DIR}/core
)