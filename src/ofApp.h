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

private:
	ofxMidiOut midiOut;
	ofxMidiIn midiIn;

	// SOUND STREAM
	ofSoundStream soundStream;
	ofxSoundOutput output;

	ofxSoundMixer mixer;
	vector<unique_ptr<ofxSoundPlayerObject>> players;

	Metronome metronome;


	ofParameterGroup parameters;
	ofParameter<float> radius;
	ofParameter<ofColor> color;
	ofxPanel gui;
	std::vector<ofxPanel> m_guiPanels;

	// pour régler le bug de gui (paramètres non éditables), on redessine la gui une nouvelle fois
	bool m_redrawGui = false;
	bool m_guiRedrawn = false;
	
	ofxPiMapper m_piMapper;

	VideoClipSource m_videoClipSource;
	
};
