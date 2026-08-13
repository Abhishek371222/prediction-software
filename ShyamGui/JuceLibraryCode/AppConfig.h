#pragma once

//==============================================================================
// JUCE module availability flags
//==============================================================================
#define JUCE_MODULE_AVAILABLE_juce_core             1
#define JUCE_MODULE_AVAILABLE_juce_data_structures  1
#define JUCE_MODULE_AVAILABLE_juce_events           1
#define JUCE_MODULE_AVAILABLE_juce_graphics         1
#define JUCE_MODULE_AVAILABLE_juce_gui_basics       1
#define JUCE_MODULE_AVAILABLE_juce_gui_extra        1

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED        1
#define JUCE_STANDALONE_APPLICATION                 1

//==============================================================================
// Platform
//==============================================================================
#define JUCE_WINDOWS 1

//==============================================================================
// Module options
//==============================================================================
#define JUCE_STRICT_REFCOUNTEDPOINTER               1
#define JUCE_VST3_CAN_REPLACE_VST2                  0
#define JUCE_USE_DIRECTWRITE                        1
#define JUCE_WASAPI                                 0
#define JUCE_DIRECTSOUND                            0
#define JUCE_ASIO                                   0
#define JUCE_USE_FLAC                               0
#define JUCE_USE_OGGVORBIS                          0
#define JUCE_USE_MP3AUDIOFORMAT                     0
#define JUCE_USE_WINDOWS_MEDIA_FORMAT               0
#define JUCE_PLUGINHOST_VST3                        0
#define JUCE_PLUGINHOST_AU                          0
