#include "midiOutput.h"
#include "stringUtils.h"

using namespace std;

MidiOutput::MidiOutput(int port, string deviceName, int deviceIndex, string deviceOsName) {
    _deviceName = deviceName;
    shortenString(_deviceName, 30, 8, 3);
    _deviceIndex = deviceIndex;
    _deviceOsName = deviceOsName;
    
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
