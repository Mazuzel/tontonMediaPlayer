#include "midiOutput.h"
#include "stringUtils.h"

using namespace std;

MidiOutput::MidiOutput(int port, string deviceName, int deviceIndex, string deviceOsName, std::string shortName) {
    _deviceName = deviceName;
    shortenString(_deviceName, 30, 8, 3);
    _deviceIndex = deviceIndex;
    _deviceOsName = deviceOsName;
    _shortName = shortName;
    
    if (deviceOsName.size() > 0)
    {
        _midiOut.openPort(deviceOsName);
        if (!_midiOut.isOpen())
        {
            ofLogError() << "Failed to open midi output on port " << port << " for device " << deviceName;
        }
    }
}

MidiOutput::~MidiOutput() {
    if (_midiOut.isOpen())
    {
        if (sendTicks)
        {
            _midiOut << StartMidi() << 0xFC << FinishMidi(); // stop playback
            _midiOut.sendProgramChange(10, 95);  // back to sync F16 program
        }

        // clean up
        _midiOut.closePort();
    }
}

bool MidiOutput::isOpen()
{
    return _midiOut.isOpen();
}

void MidiOutput::incrementManualPatchSelection(int increment)
{
    if (_automaticMode)
    {
        _automaticMode = false;
        _manualPatchSelection = 0;
    }
    else
    {
        int newManualPatch = _manualPatchSelection + increment;
        if (newManualPatch < 0 || newManualPatch >= _patchesMap.size())
        {
            _automaticMode = true;
            _manualPatchSelection = 0;
        }
        else
        {
            _manualPatchSelection = newManualPatch;
            _automaticMode = false;
        }
    }
    
    if (!_automaticMode)
    {
        map<string, unsigned int>::iterator itr(_patchesMap.begin());
        std::advance(itr, _manualPatchSelection);
        _manualPatchName = itr->first;
        _manualPatchProgram = itr->second;
    }
}

std::string MidiOutput::getManualPatchName() const
{
    return _manualPatchName;
}

int MidiOutput::getManualPatchProgram() const
{
    return _manualPatchProgram;
}
