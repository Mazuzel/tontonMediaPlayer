#pragma once

#include "ofMain.h"
#include "FboSource.h"

class VideoClipSource : public ofx::piMapper::FboSource {
public:
	void setup();
	void update();
	void draw();

	void closeVideo();
	void loadVideo(std::string videoPath);
	void playVideo();

private:
	int m_videoWidth;
	int m_videoHeight;
	ofVideoPlayer m_videoPlayer;
	ofTexture m_videoTexture;
};

