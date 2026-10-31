cmake_minimum_required(VERSION 3.22)

# macOS: Mindestversion. Universal-Binary (Apple Silicon + Intel) über
#   -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimale macOS-Version")

project(NovaLeim VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(LEIM_BUILD_TESTS  "DSP-Tests bauen" ON)
option(LEIM_MINIMAL_LINUX "Nur VST3, ohne ALSA/Xrandr/Xinerama (für schlanke Linux-Testbuilds)" OFF)
set(LEIM_JUCE_PATH "" CACHE PATH "Optional: lokaler JUCE-Ordner statt Download")
set(LEIM_JUCE_TAG "8.0.8" CACHE STRING "JUCE-Version (Git-Tag)")

#------------------------------------------------------------------------------
# JUCE
if(LEIM_JUCE_PATH)
    add_subdirectory(${LEIM_JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${LEIM_JUCE_TAG}
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(JUCE)
endif()

#------------------------------------------------------------------------------
# Plugin
if(LEIM_MINIMAL_LINUX)
    set(LEIM_FORMATS VST3)
else()
    set(LEIM_FORMATS VST3 Standalone)
    if(APPLE)
        list(APPEND LEIM_FORMATS AU)
    endif()
endif()

juce_add_plugin(NovaLeim
    COMPANY_NAME               "NOVA"
    BUNDLE_ID                  com.nova.leim
    PLUGIN_MANUFACTURER_CODE   Nova
    PLUGIN_CODE                Leim
    PRODUCT_NAME               "LEIM"
    DESCRIPTION                "Low-End Bus Compressor + Saturator"
    FORMATS                    ${LEIM_FORMATS}
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    IS_MIDI_EFFECT             FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD    FALSE
    VST3_CATEGORIES            Fx Dynamics Distortion
    AU_MAIN_TYPE               kAudioUnitType_Effect)

target_sources(NovaLeim PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/ui/LeimLookAndFeel.cpp
    Source/ui/Widgets.cpp)

target_include_directories(NovaLeim PRIVATE Source)

target_compile_definitions(NovaLeim PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0)

if(LEIM_MINIMAL_LINUX)
    target_compile_definitions(NovaLeim PUBLIC JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
    set(LEIM_JUCE_MODULES juce::juce_audio_processors juce::juce_dsp)
else()
    set(LEIM_JUCE_MODULES juce::juce_audio_utils juce::juce_dsp)
endif()

# Quelltexte enthalten UTF-8 (Umlaute, Pfeile)
target_compile_options(NovaLeim PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(NovaLeim
    PRIVATE
        ${LEIM_JUCE_MODULES}
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

#------------------------------------------------------------------------------
# DSP-Tests (ohne JUCE)
if(LEIM_BUILD_TESTS)
    enable_testing()
    add_executable(leim_dsp_test tests/dsp_test.cpp)
    target_include_directories(leim_dsp_test PRIVATE Source)
    target_compile_options(leim_dsp_test PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_test(NAME dsp COMMAND leim_dsp_test)
endif()
