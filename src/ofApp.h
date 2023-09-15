#pragma once

#include "ofMain.h"
#include "ofxMidi.h"
#include "ofxMidiClock.h"
#include "ofSoundStream.h"

#include "ofxSoundMixer.h"
#include "ofxSoundPlayerObject.h"

#include "ofxXmlSettings.h"

#include "metronome.h"

#include "videoClipSource.h"
#include "QuadSurface.h"
#include "Vec2.h"
#include "Vec3.h"

#define QUAD_CORNER_HWIDTH 12
#define TEXT_LIST_SPACING 15

class ofApp : public ofBaseApp, public ofxMidiListener {

public:
	void setup();
	void update();
	void draw();
	void drawMapping(ofEventArgs& args);
	void exit();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	// ofxMidiListener callback
	void newMidiMessage(ofxMidiMessage& eventArgs);

	void drawSequencerPage();

	void displayList(unsigned int x, unsigned int y, string title, vector<string> elements, unsigned int selectedElement);

	void loadSong();
	void stopPlayback();
	void startPlayback();

	std::vector<songEvent> m_songEvents;
	shared_ptr<ofAppBaseWindow> mappingWindow;

private:
	int openMidiOut();
	int openAudioOut();
	void loadHwConfig();
	void jumpToNextPart();
	unsigned int m_startingSongPart = 1;
	void drawMappingSetup();
	void drawHelp();
	void volumeUp();
	void volumeDown();

	// internal sound and midi handlers
	ofSoundStream soundStream;
	ofxSoundOutput output;
	ofxSoundMixer mixer;
	vector<unique_ptr<ofxSoundPlayerObject>> players;
	vector<string> playersNames;
	Metronome metronome;
	std::vector<std::string> m_midiOutDevices;
	std::vector<ofSoundDevice> m_audioDevices;
	ofxMidiOut midiOut;
	ofxMidiIn midiIn;

	// internal video handlers
	VideoClipSource m_videoClipSource;
	std::vector<QuadSurface> m_quadSurfaces;
	ofFbo m_fboSource;
	ofFbo m_fboMapping;
	bool m_videoLoaded;

	// settings.xml
	unsigned int m_bufferSize = 128;
	unsigned int m_midiInputIdx = 0;
	unsigned int m_midiOutputIdx = 0;
	unsigned int m_audioOutputIdx = 0;
	std::string m_songsRootDir = "songs/";  // path to directory containing songs

	// setlist data and state
	std::vector<std::string> m_setlist;
	unsigned int m_currentSongIndex = 0;

	// audio state
	bool m_isMidiOutOpened = false;
	bool m_isAudioOutOpened = false;
	bool m_isPlaying = false;

	// mapping setup state
	bool m_setupMappingMode = false;
	int m_quadMovedIdx = -1;
	int m_quadMovedVertexIdx = -1;

	// audio parameters
	unsigned int m_selectedVolumeSetting = 0;
	bool m_stemMode = true;  // play stems separately instead of master if we find them

	// video parameters
	unsigned int m_videoStartDelayMs = 0;
};
