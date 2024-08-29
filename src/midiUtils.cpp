#include <regex>
#include <string>

#include "midiUtils.h"

std::map<int, std::string> getMidiOutDevices()
{
	ofxMidiOut testMidiOut;
	auto devices = testMidiOut.getOutPortList();
	std::map<int, std::string> output;

	const std::regex pattern(R"((.*) (\d+$))");
    int port = 1;
	for (auto device : devices)
	{
        // FAILS TO PARSE MACOS IAC DRIVER PROPERLY
//		std::smatch m;
//		std::regex_search(device, m, pattern);
//		if (m.size() != 3)
//		{
//			continue;
//		}
//		output.insert({ std::atoi(m[2].str().c_str()), m[1].str() });

        output.insert({port, device});
        port += 1;
	}

	return output;
}
