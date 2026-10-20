cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")
endif()

project(TabifyDrums VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(libs/JUCE)
add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL)

set(TABIFY_FORMATS VST3 LV2 Standalone)
if(APPLE)
    list(APPEND TABIFY_FORMATS AU)
endif()

juce_add_plugin(TabifyDrums
    COMPANY_NAME "Tabify"
    PRODUCT_NAME "Tabify Drums"
    BUNDLE_ID com.tabify.drums
    PLUGIN_MANUFACTURER_CODE Tbfy
    PLUGIN_CODE Tbd1
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Drum
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    LV2URI "urn:tabify:drums"
    FORMATS ${TABIFY_FORMATS})

clap_juce_extensions_plugin(TARGET TabifyDrums
    CLAP_ID "com.tabify.drums"
    CLAP_FEATURES instrument drum-machine stereo)

file(GLOB TABIFY_KIT_SAMPLES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/kits/*/*.wav"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/kits/*/*.flac")
juce_add_binary_data(TabifyDrumsKitData
    HEADER_NAME KitData.h
    NAMESPACE KitData
    SOURCES ${TABIFY_KIT_SAMPLES})

juce_add_binary_data(TabifyDrumsUiData
    HEADER_NAME UiData.h
    NAMESPACE UiData
    SOURCES resources/ui/kit.jpg resources/ui/logo.png)

target_sources(TabifyDrums PRIVATE
    src/KitDefinition.cpp
    src/DrumEngine.cpp
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/gui/DrumLookAndFeel.cpp
    src/gui/DrumKitView.cpp
    src/gui/MixerView.cpp)

target_include_directories(TabifyDrums PRIVATE src)

target_compile_definitions(TabifyDrums PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

# The standalone build needs ALSA headers on Linux; without them the plugin
# formats still build, but the standalone app has no audio device support.
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ALSA QUIET alsa)
    if(NOT ALSA_FOUND)
        message(WARNING "ALSA headers not found (install alsa-lib-devel): standalone app will have no audio")
        target_compile_definitions(TabifyDrums PUBLIC JUCE_ALSA=0)
    endif()
    pkg_check_modules(JACK QUIET jack)
    if(JACK_FOUND)
        target_compile_definitions(TabifyDrums PUBLIC JUCE_JACK=1)
    endif()
endif()

target_link_libraries(TabifyDrums
    PRIVATE
        TabifyDrumsKitData
        TabifyDrumsUiData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Headless host that loads the built VST3, renders audio and takes editor
# screenshots: cmake --build build --target TabifyDrumsTestHost
option(TABIFY_BUILD_TESTHOST "Build the plugin test host" ON)
if(TABIFY_BUILD_TESTHOST)
    juce_add_console_app(TabifyDrumsTestHost PRODUCT_NAME "TabifyDrumsTestHost")
    target_sources(TabifyDrumsTestHost PRIVATE tools/TestHost.cpp)
    target_compile_definitions(TabifyDrumsTestHost PRIVATE
        JUCE_PLUGINHOST_VST3=1
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    target_link_libraries(TabifyDrumsTestHost PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_gui_basics
        juce::juce_recommended_config_flags)
endif()

# Offscreen editor screenshots, linking the plugin code directly:
# cmake --build build --target TabifyDrumsSnapshot
if(TABIFY_BUILD_TESTHOST)
    add_executable(TabifyDrumsSnapshot tools/Snapshot.cpp)
    target_link_libraries(TabifyDrumsSnapshot PRIVATE TabifyDrums)
    target_include_directories(TabifyDrumsSnapshot PRIVATE $<TARGET_PROPERTY:TabifyDrums,INCLUDE_DIRECTORIES>)
    target_compile_definitions(TabifyDrumsSnapshot PRIVATE $<TARGET_PROPERTY:TabifyDrums,COMPILE_DEFINITIONS>)
endif()
