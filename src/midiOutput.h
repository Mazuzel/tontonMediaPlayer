#pragma once

#include <string>

#include "ofMain.h"
#include "ofxMidi.h"

#include "song.h"

class MidiOutput {
public:
    MidiOutput(int port, std::string deviceName, int deviceIndex, std::string deviceOsName);
    virtual ~MidiOutput();
    bool isOpen();
    void incrementManualPatchSelection(int increment);
    std::string getManualPatchName() const;
    int getManualPatchProgram() const;
    
    bool sendTicks = false;
    bool sendTimecodes = false;
    int defaultChannel = 1;
    ofxMidiOut _midiOut;  // not a good practice of encapsulation here, but avoids writing a wrapper class :)
    int _deviceIndex;  // internal device index, for routing
    std::string _deviceName;
    std::string _deviceOsName;
    PatchFormat _patchFormat = PatchFormat::PROGRAM_NUMBER;
    std::map<std::string, unsigned int> _patchesMap;
    bool _automaticMode = true;  // follow automatically song patches
    bool _useLegacyProgram = false;  // use default program setting in song (inherits from first software versions with only 1 midi output)
private:
    int _manualPatchSelection = 0;
    int _manualPatchProgram = 0;
    std::string _manualPatchName = "";
};
