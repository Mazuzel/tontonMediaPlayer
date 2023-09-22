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

void VideoClipSource::update(float currentSongTimeMs) {
	m_frameCountSinceSpeedAnalysis += 1;
	float pct = m_videoPlayer.getPosition();
	float duration = m_videoPlayer.getDuration();
	float videoTimeMs = 1000.0 * pct * duration;
	if ((m_frameCountSinceSpeedAnalysis % 120) == 0)
	{
		m_frameCountSinceSpeedAnalysis = 0;
		float delayMs = videoTimeMs - currentSongTimeMs;

		if (delayMs > -10 && delayMs < 10)
		{
			ofLog() << "set video speed to 1.0 " << delayMs;
			m_delayCompensationInProgress = false;
			m_videoPlayer.setSpeed(1.0);
		}
		else
		{
			float speedChangeDelayMs = -5.0;  // heuristic, we count a potential slow-down of the video during update
			float newSpeed = 1.0 - (delayMs + speedChangeDelayMs) / 2000.0;  // correct during 10 next seconds
			if (newSpeed > 1.1)
			{
				newSpeed = 1.1;
			}
			else if (newSpeed < 0.9)
			{
				newSpeed = 0.9;
			}

			ofLog() << "set video speed to " << newSpeed << " " << delayMs;
			m_delayCompensationInProgress = true;
			float currentSpeed = m_videoPlayer.getSpeed();
			// only update if important difference
			m_videoPlayer.setSpeed(newSpeed);
		}
	}
	m_videoPlayer.update();
	double diff = videoTimeMs - currentSongTimeMs;
	ofLog() << "video time: " << videoTimeMs << "  / expected: " << currentSongTimeMs << "   / diff: " << diff;
}

void VideoClipSource::loadVideo(std::string videoPath) {
	m_delayCompensationInProgress = false;
	m_videoPlayer.setSpeed(1.0);
	m_videoPlayer.load(videoPath);
	m_videoPlayer.setVolume(0.0);
	m_videoWidth = m_videoPlayer.getWidth();
	m_videoHeight = m_videoPlayer.getHeight();
}

void VideoClipSource::playVideo(float initTime) {
	float duration = m_videoPlayer.getDuration();
	float pct = initTime / duration;
	ofLogError() << "duration:" << duration << ", pct:" << pct;
	m_videoPlayer.play();
	if (pct > 0.0)
	{
		m_videoPlayer.setPosition(pct);
	}
	m_isPlaying = true;
}

// No need to take care of fbo.begin() and fbo.end() here.
// All within draw() is being rendered into fbo;
void VideoClipSource::draw(int targetWidth, int targetHeight) {
	// Fill FBO with our rects
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