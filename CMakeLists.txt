cmake_minimum_required(VERSION 3.15)
project(MyAtariAgent LANGUAGES CXX)

if(WIN32)
	set(Torch_DIR "D:/Files/MakemoreC/ThirdParty/libtorch-win-shared-with-deps-2.13.0+cu132/libtorch/share/cmake/Torch")
elseif(LINUX)
	set(Torch_DIR "~/Files/TransformerC/ThirdParty/libtorch-shared-with-deps-2.13.0+cu132/libtorch/share/cmake/Torch")
endif()

find_package(Torch CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ALE_INCLUDE_DIR 
    "../Arcade-Learning-Environment/src"
    "../Arcade-Learning-Environment/build/src/ale"
)

set(ALE_BUILD_DIR "../Arcade-Learning-Environment/build/src/ale/$<CONFIG>")
set(ZLIB_BUILD_DIR "C:/vcpkg/packages/zlib_x64-windows/debug/lib")
set(SDL_BUILD_DIR "C:/vcpkg/packages/sdl2_x64-windows/debug/lib")
set(TORCH_DLL_DIR "${TORCH_INSTALL_PREFIX}/lib")

if(WIN32)
	file(GLOB TORCH_DLLS "${TORCH_DLL_DIR}/*.dll")
elseif(LINUX)
	file(GLOB TORCH_DLLS "${TORCH_DLL_DIR}/*.so*")
endif()

find_package(Torch REQUIRED)

add_executable(my_atari_runner main.cpp)

target_include_directories(my_atari_runner PRIVATE 
	${ALE_INCLUDE_DIR}
	${TORCH_INCLUDE_DIRS}
)

target_link_directories(my_atari_runner PRIVATE 
    ${ALE_BUILD_DIR} 
    ${ZLIB_BUILD_DIR}
	${SDL_BUILD_DIR}
)

target_link_libraries(my_atari_runner PRIVATE 
	ale 
	zlibd 
	SDL2d
	${TORCH_LIBRARIES}
)

if(WIN32)
    add_custom_command(TARGET my_atari_runner POST_BUILD

	# Copy shaders directory to both locations
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TORCH_DLLS} "${CMAKE_BINARY_DIR}/$<CONFIG>"

   	COMMENT "Deploying windows runtime DLL dependencies relative to Torch_DIR..."
)

add_custom_command(TARGET my_atari_runner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_RUNTIME_DLLS:my_atari_runner>"
    $<TARGET_FILE_DIR:my_atari_runner>
    COMMAND_EXPAND_LISTS
)
elseif(LINUX)
    add_custom_command(TARGET TransformerC POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/TransformerC/source/input.txt" "${CMAKE_BINARY_DIR}/bin/input.txt"

    # Copy shaders directory to both locations
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TORCH_DLLS} "${CMAKE_BINARY_DIR}/bin"
    COMMENT "Deploying linux  runtime SO dependencies relative to Torch_DIR..."
)
endif()