cmake_minimum_required(VERSION 3.22)

project("Santos Leveler" VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use the static MSVC runtime on Windows so the built VST3 has fewer external runtime dependencies.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)

set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SantosLeveler
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "Santos"
    BUNDLE_ID "com.santos.leveler"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Snts
    PLUGIN_CODE SLv1
    FORMATS VST3
    VST3_CATEGORIES Fx Dynamics
    PRODUCT_NAME "Santos Leveler"
)

juce_generate_juce_header(SantosLeveler)

target_sources(SantosLeveler PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/LevelerEngine.h
    Source/TruePeakLimiter.h
    Source/LoudnessMeter.h
    Source/HistoryBuffer.h
)

target_compile_definitions(SantosLeveler
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SantosLeveler
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

option(SANTOS_BUILD_DSP_TESTS "Build lightweight DSP tests" ON)
if(SANTOS_BUILD_DSP_TESTS)
    enable_testing()
    add_executable(SantosLevelerDSPTests Tests/LevelerDSPTests.cpp)
    target_compile_features(SantosLevelerDSPTests PRIVATE cxx_std_17)
    add_test(NAME SantosLevelerDSPTests COMMAND SantosLevelerDSPTests)
endif()
