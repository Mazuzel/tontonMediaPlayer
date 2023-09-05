#pragma once

#include "ofMain.h"
#include "ofxMidi.h"
#include "ofxMidiClock.h"
#include "ofSoundStream.h"

#include "ofxSoundMixer.h"
#include "ofxSoundPlayerObject.h"

#include "ofxGui.h"
#include "ofxXmlSettings.h"

#include "metronome.h"

#include "videoClipSource.h"
#include "QuadSurface.h"
#include "Vec2.h"
#include "Vec3.h"

#define QUAD_CORNER_HWIDTH 8
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

	// gui
	ofxPanel m_gui;
	//ofxToggle m_buttonSetMidiOutDevice;
	//ofxIntField m_fieldSelectedMidiOutDevice;


	ofParameterGroup parameters;  // A collection of parameters with events to notify if a parameter changed
	
	ofxButton m_buttonConnect;

	unsigned int m_currentSongIndex = 0;

	void loadSong();
	void stopPlayback();
	void startPlayback();

	// exit button
	ofxButton m_buttonExit;
	void exitButtonPressed(const void* sender);

	bool m_projectionWindowFocus = false;
	std::vector<songEvent> m_songEvents;

	shared_ptr<ofAppBaseWindow> mappingWindow;

private:
	int openMidiOut();
	int openAudioOut();
	void loadHwConfig();
	void jumpToNextPart();
	unsigned int m_startingSongPart = 1;
	void drawMappingSetup();
	void drawSequencerBackground();
	void drawHelp();
	void volumeUp();
	void volumeDown();

	ofxMidiOut midiOut;
	ofxMidiIn midiIn;

	// SOUND STREAM
	ofSoundStream soundStream;
	ofxSoundOutput output;

	ofxSoundMixer mixer;
	vector<unique_ptr<ofxSoundPlayerObject>> players;
	vector<string> playersNames;

	Metronome metronome;

	unsigned int m_midiOutputIdx = 0;
	unsigned int m_audioOutputIdx = 0;
	
	VideoClipSource m_videoClipSource;

	std::vector<std::string> m_setlist;

	std::vector<std::string> m_midiOutDevices;
	std::vector<ofSoundDevice> m_audioDevices;

	bool m_isMidiOutOpened = false;
	bool m_isAudioOutOpened = false;

	std::vector<QuadSurface> m_quadSurfaces;
	bool m_setupMappingMode = false;
	int m_quadMovedIdx = -1;
	int m_quadMovedVertexIdx = -1;

	ofFbo m_fboSource;
	ofFbo m_fboMapping;

	// state
	bool m_isPlaying = false;

	unsigned int m_selectedVolumeSetting = 0;
	
};
