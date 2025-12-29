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

target_link_libraries(SteamAutoCracker winhttp dwmapi comctl32 version uxtheme)

add_custom_command(TARGET SteamAutoCracker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tools $<TARGET_FILE_DIR:SteamAutoCracker>/tools
    COMMAND cmd /C "if /I \"$<CONFIG>\" == \"Release\" powershell -NoProfile -Command \"Get-ChildItem -LiteralPath '$<TARGET_FILE_DIR:SteamAutoCracker>' | Where-Object { $_.Name -ne '$<TARGET_FILE_NAME:SteamAutoCracker>' -and $_.Name -ne 'tools' } | Remove-Item -Recurse -Force\""
)