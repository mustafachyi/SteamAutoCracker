cmake_minimum_required(VERSION 3.20)
project(SteamAutoCracker-CPP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE)

include_directories(include)
include_directories(external)

set(SOURCES
    src/main.cpp
    src/core/AppIdResolver.cpp
    src/core/AutoCracker.cpp
    src/ui/MainWindow.cpp
    resources/sac_gui.rc
)

add_executable(SteamAutoCracker WIN32 ${SOURCES})

set_target_properties(SteamAutoCracker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(SteamAutoCracker winhttp dwmapi comctl32 version uxtheme)

add_custom_command(TARGET SteamAutoCracker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tools $<TARGET_FILE_DIR:SteamAutoCracker>/tools
)