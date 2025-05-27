cmake_minimum_required(VERSION 3.5)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
project(xrtcserver)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")


add_definitions("-g -pipe -W -Wall -fPIC -std=gnu++14 -DWEBRTC_POSIX -DWEBRTC_LINUX")

include_directories(
    "./src"
    "./third_party/include"
    "../rtcbase/src"
)

link_directories(
    "./third_party/lib"
    "../rtcbase/out"
)

file(GLOB all_src
    "./src/*.cpp"
    "./src/base/*.cpp"
    "./src/server/*.cpp"
    "./src/stream/*.cpp"
    "./src/pc/*.cpp"
    "./src/ice/*.cpp"
)

add_executable(xrtcserver ${all_src})

target_link_libraries(xrtcserver libyaml-cpp.a librtcbase.a 
    libabsl_strings.a
    libabsl_throw_delegate.a
    libev.a
    libjsoncpp.a
    libssl.a
    libcrypto.a -lpthread -ldl
    
)
