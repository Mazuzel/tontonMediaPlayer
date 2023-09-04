#include "videoClipSource.h"

void VideoClipSource::setup() {
	m_videoPlayer.setVolume(0);

	m_videoWidth = 800;
	m_videoHeight = 600;

	// Allocate our FBO source, decide how big it should be
	m_isPlaying = false;
}

void VideoClipSource::closeVideo() {
	m_videoPlayer.close();
	m_isPlaying = false;
}

void VideoClipSource::update() {
	m_videoPlayer.update();
}

void VideoClipSource::loadVideo(std::string videoPath) {
	m_videoPlayer.load(videoPath);
	m_videoWidth = m_videoPlayer.getWidth();
	m_videoHeight = m_videoPlayer.getHeight();
}

void VideoClipSource::playVideo(float initTime) {
	float duration = m_videoPlayer.getDuration();
	float pct = initTime / duration;
	ofLogError() << "duration:" << duration << ", pct:" << pct;
	m_videoPlayer.play();
	m_videoPlayer.setPosition(pct);
	m_isPlaying = true;
}

// No need to take care of fbo.begin() and fbo.end() here.
// All within draw() is being rendered into fbo;
void VideoClipSource::draw(int targetWidth, int targetHeight) {
	// Fill FBO with our rects
	ofClear(0);
	ofBackground(0);
	ofSetColor(255);

	if (m_isPlaying)
	{
		m_videoPlayer.draw(0, 0, targetWidth, targetHeight);
	}
}

ofTexture& VideoClipSource::getTexture()
{
	return m_videoPlayer.getTextureReference();
}