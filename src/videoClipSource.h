#pragma once

#include "ofMain.h"

class VideoClipSource {
public:
	void setup();
	void update();
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
};

