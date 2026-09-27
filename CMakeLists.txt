cmake_minimum_required(VERSION 3.16)

project(ViPER4Linux VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Widgets)

qt_standard_project_setup()

qt_add_executable(ViPER4LinuxApp
    src/main.cpp
    src/ViPERState.h
    src/ViPERState.cpp
    src/AudioEngineLinux.h
    src/AudioEngineLinux.cpp
    src/EqGraphItem.cpp
)

# Add all QML files to a Qt Resource for the executable
qt_add_qml_module(ViPER4LinuxApp
    URI ViPER4Linux
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/CustomSlider.qml
        qml/components/CustomSwitch.qml
        qml/components/SectionHeader.qml
        qml/components/CustomSegmentedPicker.qml
        qml/components/DriverStatusModal.qml
        qml/components/EqBandSlider.qml
        qml/components/SidebarItem.qml
        qml/components/AnimatedSection.qml
)

# Add ViPERDSP without modifying its own directory
add_subdirectory(ViPERDSP)
# Force-include the missing headers into ViPERDSP compilation
target_compile_options(ViPERDSP PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/force_includes.h)
target_compile_options(ViPER4LinuxApp PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/force_includes.h)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3 libspa-0.2)

target_include_directories(ViPER4LinuxApp PRIVATE ${PIPEWIRE_INCLUDE_DIRS})

target_link_libraries(ViPER4LinuxApp
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Widgets
        ViPERDSP
        PkgConfig::PIPEWIRE
)

