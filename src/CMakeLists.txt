cmake_minimum_required(VERSION 3.7)


set(SOURCE_FILES main.cpp sdl_render.cpp vmath.cpp bitfont.cpp)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Find SDL2 and OpenGL
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(SDL2 REQUIRED COMPONENTS main)
find_package(SDL2_image REQUIRED)


include_directories(${SDL2_INCLUDE_DIRS} ${SDL2main_INCLUDE_DIRS}  ${SDL2_IMAGE_INCLUDE_DIRS} ${CMAKE_BINARY_DIR})

add_executable(breakout ${SOURCE_FILES})
target_link_libraries(breakout ${SDL2_LIBS} ${SDL2_IMAGE_LIBRARIES} EnTT)


# Copy SDL2 DLLs to output folder on Windows
if(WIN32)
    foreach(DLL ${SDL2_DLLS})
        add_custom_command(TARGET breakout POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:breakout>)
    endforeach()
	foreach(IMGDLL ${SDL2_IMAGE_DLLS}) 
        add_custom_command(TARGET breakout POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E copy_if_different ${IMGDLL} $<TARGET_FILE_DIR:breakout>)
    endforeach()
endif()
