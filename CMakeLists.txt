cmake_minimum_required(VERSION 3.15)
project(MyAtariAgent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ALE_INCLUDE_DIR 
    "../Arcade-Learning-Environment/src"
    "../Arcade-Learning-Environment/build/src/ale"
)

set(ALE_BUILD_DIR "../Arcade-Learning-Environment/build/src/ale/$<CONFIG>")
set(ZLIB_BUILD_DIR "C:/vcpkg/packages/zlib_x64-windows/debug/lib")
set(SDL_BUILD_DIR "C:/vcpkg/packages/sdl2_x64-windows/debug/lib")

add_executable(my_atari_runner main.cpp)
target_include_directories(my_atari_runner PRIVATE ${ALE_INCLUDE_DIR})
target_link_directories(my_atari_runner PRIVATE 
    ${ALE_BUILD_DIR} 
    ${ZLIB_BUILD_DIR}
	${SDL_BUILD_DIR}
)
target_link_libraries(my_atari_runner PRIVATE ale zlibd SDL2d)