#pragma once

#include "ofMain.h"
#include "ofxSoundObject.h"
#include "ofxMidi.h"

struct songEvent {
	long tick; // duration of the part, in terms of tick count
	int program;
	string programName;
	float bpm;
	string shader;
};

class Metronome : public ofxSoundObject {
public:
	Metronome();
	virtual ~Metronome();

	void setMidiOut(ofxMidiOut& midiOut);

	void setNewSong(std::vector<songEvent> songEvents);

	void process(ofSoundBuffer& input, ofSoundBuffer& output);

	ofxMidiOut m_midiOut;

	void sendNextProgramChange();

	void setEnabled(bool enabled);

	bool isSongEnded();

	const unsigned int getTickCount() const;
	double getPlaybackPositionMs() const;

	const unsigned int getCurrentSongPartIdx() const;
	void setCurrentSongPartIdx(unsigned int newSongPartIdx);

	void setNbIgnoredStartupsTicks(int nbIgnoredStartupTicks);

private:

	void tick();

	bool m_enabled = false;

	int m_samplesPerTick;
	int m_ticksPerBeat;
	int m_samples = 0;

	std::vector<songEvent> m_songEvents;
	long m_totalTickCount;
	int m_currentSongPartIndex;
	int m_tickCountStartThreshold;

};