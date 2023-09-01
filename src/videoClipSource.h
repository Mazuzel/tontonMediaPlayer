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
	void playVideo(float initTime);

private:
	int m_videoWidth;
	int m_videoHeight;
	ofVideoPlayer m_videoPlayer;
	ofTexture m_videoTexture;
	bool m_isPlaying;
};

