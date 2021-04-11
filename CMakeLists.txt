cmake_minimum_required(VERSION 3.19)

project(tonfc LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(LibNFC 1.8.0 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    "src/main.cpp"
    "src/device.cpp"
	"src/device.hpp"
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
	${LIBNFC_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
	${LIBNFC_LIBRARIES}
)
