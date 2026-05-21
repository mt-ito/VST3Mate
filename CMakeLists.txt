cmake_minimum_required(VERSION 3.22)
project(VST3Mate VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE を FetchContent で取得
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# Assets/sprites 以下の PNG をバイナリデータとして埋め込む
file(GLOB IDLE_SPRITES    CONFIGURE_DEPENDS "Assets/sprites/idle/*.png")
file(GLOB CLICKED_SPRITES CONFIGURE_DEPENDS "Assets/sprites/clicked/*.png")

juce_add_binary_data(VST3MateBinaryData
    HEADER_NAME  BinaryData.h
    NAMESPACE    BinaryData
    SOURCES
        ${IDLE_SPRITES}
        ${CLICKED_SPRITES}
)

juce_add_plugin(VST3Mate
    COMPANY_NAME            "VST3Mate"
    PLUGIN_MANUFACTURER_CODE Vstm
    PLUGIN_CODE             Vm01
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "VST3Mate"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       TRUE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES         "Instrument|Tools"
)

target_sources(VST3Mate
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/CharacterComponent.cpp
        Source/OscHandler.cpp
        Source/NoteGenerator.cpp
        Source/ExtensionManager.cpp
)

target_compile_definitions(VST3Mate
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(VST3Mate
    PRIVATE
        VST3MateBinaryData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_osc
        juce::juce_graphics
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

juce_generate_juce_header(VST3Mate)
