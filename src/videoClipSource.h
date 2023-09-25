#pragma once

#include "ofMain.h"

class VideoClipSource {
public:
	void setup();
	void update(float currentSongTimeMs);
	void draw(int targetWidth, int targetHeight);

	void closeVideo();
	void loadVideo(std::string videoPath);
	void playVideo(float initTime);
	ofTexture& getTexture();


private:
	int m_videoWidth;
	int m_videoHeight;
	ofVideoPlayer m_videoPlayer;
	ofTexture m_videoTexture;
	bool m_isPlaying;
	float m_NextPlaybackTimeSpeedCheck = 0;
	float m_speedChangeDelayMs = 30.0;
	float m_nextTheoreticalPlaybackTime = 0.0;
};

