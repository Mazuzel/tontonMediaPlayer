#include "volumesDb.h"
#include "ofxXmlSettings.h"

using namespace std;

void VolumesDb::getStoredSongVolumes(string songsRootDir, string songName, vector<pair<string, float>>& volumes)
{
	vector<pair<string, float>> result;
	ofxXmlSettings settings;
	if (settings.load(songsRootDir + songName + "/tracks.xml")) {
		bool success = settings.pushTag("tracks");
		if (!success)
		{
			ofLog() << "could not load song from song tracks file: bad xml structure";
		}
        
        int tracksCount = settings.getNumTags("track");
        
        for (int i = 0; i < tracksCount; i++) {
            string trackFile = settings.getAttribute("track", "file", "", i);
            
            // find matching loaded file
            for (int i = 0; i < volumes.size(); i++) {
                if (volumes[i].first == trackFile)
                {
                    int volume = stoi(settings.getAttribute("track", "volume", "100", i));
                    volumes[i].second = volume / 100.0;
                }
            }
        }
	}
	else
	{
		ofLog() << "could not load sont tracks file: tracks.xml";
	}
}

void VolumesDb::setStoredSongVolumes(string songsRootDir, string songName, vector<pair<std::string, float>>& volumes)
{
	ofxXmlSettings settings;
    settings.addTag("tracks");
    settings.pushTag("tracks");

	for (int i = 0; i < volumes.size(); i++)
	{
        settings.addTag("track");
        settings.addAttribute("track", "file", volumes[i].first, i);
        settings.addAttribute("track", "volume", to_string(int(100.0 * volumes[i].second)), i);
    }
    settings.popTag();

	settings.save(songsRootDir + songName + "/tracks.xml");
}
