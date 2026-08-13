cmake_minimum_required(VERSION 3.22)
project(AtomikPolar VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_gui_app(AtomikPolar
    PRODUCT_NAME "Atomik Polar"
    COMPANY_NAME "Atomik"
    BUNDLE_ID com.atomik.polar
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE)

juce_generate_juce_header(AtomikPolar)

target_sources(AtomikPolar PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/PolarPlotComponent.cpp
    Source/MeasurementLibrary.cpp)

target_compile_definitions(AtomikPolar PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:AtomikPolar,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:AtomikPolar,JUCE_VERSION>")

target_link_libraries(AtomikPolar PRIVATE
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Copy measurement CSVs next to the executable
add_custom_command(TARGET AtomikPolar POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Data"
        "$<TARGET_FILE_DIR:AtomikPolar>/Data"
    COMMENT "Copying measurement Data folder")
