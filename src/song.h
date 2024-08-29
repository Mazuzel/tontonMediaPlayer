#pragma once

#include "ofMain.h"

enum PatchFormat {
    PROGRAM_NUMBER,
    PATCH_NAME,
    ELEKTRON_PATTERN
};

struct PatchEvent {
    unsigned int programNumber;
    unsigned int channel;
    unsigned int midiOutputIndex;
    std::string name;
};

struct songEvent {
    long tick; // duration of the part, in terms of tick count
    int program;
    string programName;
    float bpm;
    string shader;
    string name;
    std::vector<PatchEvent> patches;
};
