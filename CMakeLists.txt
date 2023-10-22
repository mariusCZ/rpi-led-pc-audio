# CMakeList.txt : CMake project for fft-leds, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("fft-leds")

find_package(portaudio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PATH "D:\\Software\\Qt\\6.5.2\\msvc2019_64\\bin;%PATH%")

# Without this line, cmake does not find the package
set(CMAKE_PREFIX_PATH "D:\\Software\\Qt\\6.5.2\\msvc2019_64")

find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)
qt_standard_project_setup()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

QT_WRAP_UI( UI_HEADERS ${UIS} )

find_package(ixwebsocket)

# Add source to this project's executable.
add_executable (fft-leds "src/fft-leds.cpp" "include/fft-leds.h")

file(GLOB SRC_FILES    
    "src/*.cpp"
    "src/*.c"
    "include/*.h"
)

target_sources(fft-leds PUBLIC
    ${SRC_FILES}
)

target_link_libraries(fft-leds PRIVATE portaudio ixwebsocket::ixwebsocket Qt6::Widgets Qt6::SerialPort)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET fft-leds PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
