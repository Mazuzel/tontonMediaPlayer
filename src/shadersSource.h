#pragma once

#include "ofMain.h"
#include <map>
#include "metronome.h"

struct shaderEvent {
	long tick; // tick at which the part triggers
	float bpm;
	string shader;
};

class ShadersSource {
public:
	void setup(std::vector<songEvent> songEvents);
	void draw(int targetWidth, int targetHeight, int ticks, float time);

private:
	std::map<std::string, ofShader> m_shaders;
	std::vector<shaderEvent> m_events;
	int m_currentSongPartIndex;
};