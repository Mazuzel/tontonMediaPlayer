#include "videoClipSource.h"

void VideoClipSource::setup() {
	m_videoPlayer.setVolume(0);

	m_videoWidth = 800;
	m_videoHeight = 600;

	// Allocate our FBO source, decide how big it should be
	m_isPlaying = false;
}

void VideoClipSource::setSpeedChangeDelay(float speedChangeDelay)
{
	if (speedChangeDelay > 0)
	{
		m_speedChangeDelayMs = speedChangeDelay;
	}
}

void VideoClipSource::closeVideo() {
	m_videoPlayer.close();
	m_isPlaying = false;
}

void VideoClipSource::update(float currentSongTimeMs) {
	float pct = m_videoPlayer.getPosition();
	float duration = m_videoPlayer.getDuration();
	float videoTimeMs = 1000.0 * pct * duration;
	if (currentSongTimeMs >= m_NextPlaybackTimeSpeedCheck)
	{
		float delayMs = videoTimeMs - currentSongTimeMs;

		if (delayMs > -25 + m_speedChangeDelayMs && delayMs < 25 + m_speedChangeDelayMs)
		{
			if (m_videoPlayer.getSpeed() != 1.0)
			{
				m_videoPlayer.setSpeed(1.0);
			}
			ofLog() << "sync ok: " << delayMs << " cd=" << m_speedChangeDelayMs;
			m_NextPlaybackTimeSpeedCheck = currentSongTimeMs + 30000.0;  // longer check delay
		}
		else
		{
			float delayCaped = min(100.0, max(-100.0, double(delayMs)));
			delayCaped -= m_speedChangeDelayMs;
			float newSpeed = 1.0 - delayCaped / 2900.0; // heuristic to provide slight overshoot and try to achieve better value when stopping
			if (newSpeed > 1.0 && newSpeed < 1.01)
			{
				newSpeed = 1.01;  // else if does not seem to have any effect...
			}
			else if (newSpeed < 1.0 && newSpeed > 0.99)
			{
				newSpeed = 0.99;
			}

			m_nextTheoreticalPlaybackTime = videoTimeMs + newSpeed * 3000.0;

			m_videoPlayer.setSpeed(newSpeed);
			ofLog() << "sync not ok: " << delayMs << " (speed:" << newSpeed << ")" << " cd=" << m_speedChangeDelayMs;
			m_NextPlaybackTimeSpeedCheck = currentSongTimeMs + 3000.0;
		}
		//double diff = videoTimeMs - currentSongTimeMs;
		//ofLog() << "video time: " << videoTimeMs << "  / expected: " << currentSongTimeMs << "   / diff: " << diff;
	}

	m_videoPlayer.update();
}

void VideoClipSource::loadVideo(std::string videoPath) {
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
	m_NextPlaybackTimeSpeedCheck = initTime;
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