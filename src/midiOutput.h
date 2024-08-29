#pragma once

#include "ofMain.h"
#include "ofxMidi.h"

#include "song.h"

class MidiOutput {
public:
    MidiOutput(int port, std::string deviceName, int deviceIndex, std::string deviceOsName);
    virtual ~MidiOutput();
    bool isOpen();
    
    bool sendTicks = false;
    bool sendTimecodes = false;
    int defaultChannel = 1;
    ofxMidiOut _midiOut;  // not a good practice of encapsulation here, but avoids writing a wrapper class :)
    int _deviceIndex;  // internal device index, for routing
    std::string _deviceName;
    std::string _deviceOsName;
    PatchFormat _patchFormat = PatchFormat::PROGRAM_NUMBER;
    std::map<std::string, unsigned int> _patchesMap;
};
