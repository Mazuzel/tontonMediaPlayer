#pragma once

#include "ofMain.h"
#include "ofxSoundObject.h"
#include "ofxMidi.h"

#include "midiOutput.h"

#include "song.h"


class Metronome : public ofxSoundObject {
public:
	Metronome();
	virtual ~Metronome();

	void setMidiOuts(std::vector<std::shared_ptr<MidiOutput>>& midiOuts);

	void setNewSong(std::vector<songEvent> songEvents);

	void process(ofSoundBuffer& input, ofSoundBuffer& output);

	std::vector<std::shared_ptr<MidiOutput>> m_midiOuts;

	void sendNextProgramChange();

	void setEnabled(bool enabled);

	bool isSongEnded();

	void setLoopMode(bool loop);
	const bool loopEndReached() const;

	const unsigned int getTickCount() const;
	double getPlaybackPositionMs() const;

	const unsigned int getCurrentSongPartIdx() const;
	void setCurrentSongPartIdx(unsigned int newSongPartIdx);

	void setNbIgnoredStartupsTicks(int nbIgnoredStartupTicks);
	void correctTicksToPlaybackPosition(double realPlaybackPositionMs);
	void setSampleRate(unsigned int sampleRate);

private:

	void tick();

	bool m_loop = false;
	bool m_loopEndReached = false;

	bool m_enabled = false;

	int m_samplesPerTick;
	int m_ticksPerBeat;
	int m_samples = 0;
	std::vector<songEvent> m_songEvents;
	long m_totalTickCount;
	int m_currentSongPartIndex;
	int m_tickCountStartThreshold;
	int m_ticksLate = 0;

	unsigned int m_sampleRate;
};
