#include "volumesDb.h"
#include "ofxXmlSettings.h"

using namespace std;

void VolumesDb::getStoredSongVolumes(string songName, vector<pair<string, float>>& volumes)
{
	vector<pair<string, float>> result;
	ofxXmlSettings settings;
	if (settings.loadFile("volumesDb.xml")) {
		settings.pushTag("settings");
		bool success = settings.pushTag(songName);
		if (!success)
		{
			ofLog() << "could not load song from volumes db file: " << songName;
		}
	
		for (int i = 0; i < volumes.size(); i++) {
			if (settings.tagExists(volumes[i].first))
			{
				int volume = settings.getValue(volumes[i].first, 100);
				volumes[i].second = volume / 100.0;
			}
		}
	}
	else
	{
		ofLog() << "could not load volumes db file: volumesDb.xml";
	}
}

void VolumesDb::setStoredSongVolumes(string songName, vector<pair<std::string, float>>& volumes)
{
	ofxXmlSettings settings;
	settings.loadFile("volumesDb.xml");

	for (int i = 0; i < volumes.size(); i++)
	{
		string tag = "settings:" + songName + ":" + volumes[i].first;
		int volume = int(volumes[i].second * 100);
		settings.setValue(tag, volume);
	}

	settings.saveFile("volumesDb.xml");
}