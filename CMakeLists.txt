cmake_minimum_required(VERSION 3.21)

project(PSKAnalyzer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(psk-analyzer
    src/AppConfig.h
    src/MacroEngine.cpp
    src/MacroEngine.h
    src/MainWindow.cpp
    src/MainWindow.h
    src/MockDecoder.cpp
    src/MockDecoder.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/SignalTypes.h
    src/WaterfallWidget.cpp
    src/WaterfallWidget.h
    src/main.cpp
)

target_link_libraries(psk-analyzer PRIVATE Qt6::Widgets)

if(WIN32)
    set_target_properties(psk-analyzer PROPERTIES WIN32_EXECUTABLE ON)
endif()

install(TARGETS psk-analyzer RUNTIME DESTINATION bin)
