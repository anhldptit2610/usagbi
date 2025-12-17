add_library(gb_utils STATIC
	logger.cpp
)

target_link_libraries(gb_utils PRIVATE 
	gb_core
	spdlog::spdlog
)

target_include_directories(gb_utils PUBLIC 
	${CMAKE_SOURCE_DIR}/utils
	${CMAKE_SOURCE_DIR}/core
)