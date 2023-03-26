#pragma once

#include "ofMain.h"
#include "ofxMidi.h"
#include "ofxMidiClock.h"
#include "ofSoundStream.h"

#include "ofxSoundMixer.h"
#include "ofxSoundPlayerObject.h"

#include "ofxGui.h"

#include "metronome.h"

#include "ofxPiMapper.h"

#include "videoClipSource.h"

class ofApp : public ofBaseApp, public ofxMidiListener {

public:
	void setup();
	void setupGui();
	void update();
	void draw();
	void drawGui(ofEventArgs& args);
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

	void drawSetupPage();
	void drawSequencerPage();

	// gui
	ofxPanel m_gui;
	//ofxToggle m_buttonSetMidiOutDevice;
	//ofxIntField m_fieldSelectedMidiOutDevice;


	ofParameterGroup parameters;  // A collection of parameters with events to notify if a parameter changed
	
	ofxButton m_buttonConnect;
	ofxInputField<int> m_midiOutDeviceInputField;

	std::vector<ofxToggle> m_audioDeviceSelectors;

	// acces au menu de setup audio/midi
	ofxButton m_buttonSettings;
	void setupButtonPressed(const void* sender);

	// setup : choix audio/midi in/out
	ofParameter<int> m_selectedMidiOutDevice;
	ofParameter<int> m_selectedAudioOutputDevice;
	ofxPanel m_settingsGui;

	// setup : validation
	ofxButton m_buttonValidateSettings;
	void validateSettingsButtonPressed(const void* sender);

	// exit button
	ofxButton m_buttonExit;
	void exitButtonPressed(const void* sender);

	void midiOutTogglePressed(const void* sender, bool& pressed);
	void audioOutTogglePressed(const void* sender, bool& pressed);

private:
	int openMidiOut();
	int openAudioOut();

	ofxMidiOut midiOut;
	ofxMidiIn midiIn;

	// SOUND STREAM
	ofSoundStream soundStream;
	ofxSoundOutput output;

	ofxSoundMixer mixer;
	vector<unique_ptr<ofxSoundPlayerObject>> players;

	Metronome metronome;


	
	
	ofxPiMapper m_piMapper;

	VideoClipSource m_videoClipSource;

	std::vector<std::string> m_setlist;

	std::vector<std::string> m_midiOutDevices;
	std::vector<ofSoundDevice> m_audioDevices;

	bool m_isMidiOutOpened = false;
	bool m_isAudioOutOpened = false;


	// state
	bool m_isPlaying = false;
	bool m_isSetupPageOpened = false;
	
};
