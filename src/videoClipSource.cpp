#include "videoClipSource.h"

void VideoClipSource::setup() {
	// Give our source a decent name
	name = "Video Clip Source";

	m_videoPlayer.setVolume(0);

	m_videoWidth = 800;
	m_videoHeight = 600;

	// Allocate our FBO source, decide how big it should be
	allocate(800, 600);

}

void VideoClipSource::closeVideo() {
	m_videoPlayer.close();
}

void VideoClipSource::update() {
	m_videoPlayer.update();
}

void VideoClipSource::loadVideo(std::string videoPath) {
	m_videoPlayer.load(videoPath);
	m_videoWidth = m_videoPlayer.getWidth();
	m_videoHeight = m_videoPlayer.getHeight();
	allocate(m_videoWidth, m_videoHeight);
}

void VideoClipSource::playVideo() {
	m_videoPlayer.play();
}

// No need to take care of fbo.begin() and fbo.end() here.
// All within draw() is being rendered into fbo;
void VideoClipSource::draw() {
	// Fill FBO with our rects
	ofClear(0);
	ofBackground(0);
	ofSetColor(255);

	m_videoPlayer.draw(0, 0);
}