cmake_minimum_required(VERSION 2.6)
project(Chip8)

file(GLOB Chip8_SRC "src/*.cpp")
file(GLOB Imgui_SRC "imgui/*.cpp")

set(OpenGL_GL_PREFERENCE "GLVND")

find_package(SFML 2.5 COMPONENTS window system graphics audio REQUIRED)
find_package(OpenGL REQUIRED)

include_directories("include")
		include_directories("imgui")
include_directories(${SFML_INCLUDE_DIRS})
include_directories(${OPENGL_INCLUDE_DIRS})

if(DEFINED CMAKE_BUILD_TYPE)
	if(CMAKE_BUILD_TYPE STREQUAL CMAKE_BUILD_TYPE)
		add_definitions(-DDEBUG)
	endif(CMAKE_BUILD_TYPE STREQUAL CMAKE_BUILD_TYPE)
endif(DEFINED CMAKE_BUILD_TYPE)

add_executable(Chip8 ${Chip8_SRC} ${Imgui_SRC})
target_link_libraries(Chip8 sfml-system sfml-window sfml-graphics sfml-audio ${OPENGL_LIBRARIES})
