#pragma once

#include "ofMain.h"

#include <utility>
#include <string>

class VolumesDb {
public:
	static void getStoredSongVolumes(std::string songsRootDir, std::string songName, std::vector<std::pair<std::string, float>>& volumes);
	static void setStoredSongVolumes(std::string songsRootDir, std::string songName, std::vector<std::pair<std::string, float>>& volumes);
};
