#include <regex>
#include <string>

#include "midiUtils.h"

std::map<int, std::string> getMidiOutDevices()
{
	ofxMidiOut testMidiOut;
	auto devices = testMidiOut.getOutPortList();
	std::map<int, std::string> output;

	const std::regex pattern(R"((.*) (\d+$))");
	for (auto device : devices)
	{
		std::smatch m;
		std::regex_search(device, m, pattern);
		if (m.size() != 3)
		{
			continue;
		}
		output.insert({ std::atoi(m[2].str().c_str()), m[1].str() });
	}

	return output;
}