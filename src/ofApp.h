#pragma once

#include "ofMain.h"
#include "ofxMidi.h"
#include "ofxMidiClock.h"
#include "ofSoundStream.h"

#include "ofxSoundMixer.h"
#include "ofxSoundPlayerObject.h"

#include "ofxXmlSettings.h"

#include "metronome.h"

#include "list.h"
#include "midiOutput.h"
#include "shadersSource.h"
#include "song.h"
#include "videoClipSource.h"
#include "QuadSurface.h"
#include "Vec2.h"
#include "Vec3.h"

#define QUAD_CORNER_HWIDTH 12
#define TEXT_LIST_SPACING 15
#define VOLUME_MAX 2.0

enum MAIN_UI_ELEMENT {
    SETLIST,
    MIXER,
    MIDI_OUTPUTS
};

bool isMouseInRect(ofRectangle rect, int x, int y)
{
    if (x >= rect.x && x <= rect.x + rect.width && y >= rect.y && y <= rect.y + rect.height)
    {
        return true;
    }
    return false;
}

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

	void drawAnimatedLogo();

	void displayList(unsigned int x, unsigned int y, string title, vector<string> elements, unsigned int activeElement, unsigned int selectedElement, bool showIndex);

	void stopPlayback();
	void startPlayback();

	std::vector<songEvent> m_songEvents;
	shared_ptr<ofAppBaseWindow> mappingWindow = nullptr;

private:
    void loadSong();
	int openMidiOut();
	int openAudioOut();
	void loadHwConfig();
	void jumpToNextPart();
	void jumpToPreviousPart();
	unsigned int m_startingSongPart = 1;
	void drawMappingSetup();
    void drawPatches();
    void drawMixer();
	void drawHelp();
    void drawPlayer();
	void volumeUp();
	void volumeDown();
	double getCurrentSongTimeMs();
	void loadSongByIndex(unsigned int index);
    void saveMappingNodes();
    void loadMappingNodes();
	void loadPiMapperSurfaces();
    void loadSetlist();
    void changeSelectedUiElement(MAIN_UI_ELEMENT uiElement);
    void saveAudioMixerVolumes();

	// internal sound and midi handlers
	ofSoundStream soundStream;
	ofxSoundOutput output;
	ofxSoundMixer mixer;
	vector<unique_ptr<ofxSoundPlayerObject>> players;
	vector<string> playersNames;
	Metronome metronome;
	ofxMidiIn midiIn;
    std::vector<std::shared_ptr<MidiOutput>> _midiOuts;
	unsigned int m_sampleRate = 22050;
    int m_lastAudioMidiSyncPositionMs = 0;

	// internal video handlers
    bool m_enableVisuals = false;
	VideoClipSource m_videoClipSource;
	std::vector<QuadSurface> m_quadSurfaces;
	std::vector<ofFbo> m_fboSources;
	ofFbo m_fboMapping;
	bool m_videoLoaded;

	// internal shaders handlers
	ShadersSource m_shadersSource;

	// settings.xml
	unsigned int m_bufferSize = 128;
	std::vector<unsigned int> m_midiOutPorts;
	std::string m_songsRootDir = "songs/";  // path to directory containing songs
	std::string m_mappingConfigFileOverride = "";
    std::string m_requestedAudioOutDevice = "";
    ofSoundDevice::Api m_requestedAudioOutApi = ofSoundDevice::Api::DEFAULT;

	// setlist data and state
	std::vector<std::string> m_setlist;
	unsigned int m_currentSongIndex = 0;
	unsigned int m_songSelectorToolIdx = 0;

	// audio state
	bool m_isAudioOutOpened = false;
	bool m_isPlaying = false;

	// midi input
	bool m_enableMidiIn = false;

	// mapping setup state
	bool m_setupMappingMode = false;
	int m_quadMovedIdx = -1;
	int m_quadMovedVertexIdx = -1;

	// audio parameters
    std::string m_openedAudioDeviceName = "";
    ofSoundDevice::Api m_openedAudioDeviceApi;
	unsigned int m_selectedVolumeSetting = 0;
	bool m_autoPlayNext = false;

	// loop mode
	bool m_loop = false;

	// video parameters
	unsigned int m_videoStartDelayMs = 0;
	bool m_videoResync = true;

	// default video
	ofShader m_defaultShader;
	bool m_isDefaultShaderLoaded;

	// video monitoring
	float m_measuredVideoDelayMs = 0.0;
    
    // debounce
    float m_lastSongReloadTime;
    float m_lastSongChangeTime;

	// audio files
	std::vector<std::string> m_audioFilesIgnoreIfContains;

	// framerate
	unsigned int m_audioRefreshRate = 60;
	unsigned int m_videoRefreshRate = 24;
	float m_lastVideoRefreshTime = 0.0f;

	// app logo
	ofImage m_logo;
    
    // keys and controls
    bool m_keyShiftPressed = false;
    
    // UI elements
    MAIN_UI_ELEMENT m_mainUiElementSelected = MAIN_UI_ELEMENT::SETLIST;
    Tonton::Utils::ListView m_setlistView;
    ofColor m_colorFocused = ofColor(242, 182, 17);
    ofColor m_colorNotFocused = ofColor(26, 97, 138);
    std::string m_helper;
    unsigned int m_selectedMidiOutput = 0;
    
    // clickable areas
    ofRectangle m_areaStop {20, 430, 12, 12};
    ofRectangle m_areaPlay {43, 430, 12, 12};
    ofRectangle m_areaPreviousSongPart {62, 430, 12, 12};
    ofRectangle m_areaNextSongPart {84, 430, 12, 12};
    ofRectangle m_areaAutoPlay {725, 430, 12, 12};
    
    ofRectangle m_areaMixer {230, 35, 510, 210};
    ofRectangle m_areaSetlist {20, 35, 190, 380};
    ofRectangle m_areaPatches {230, 265, 510, 150};
    
    // max elements (no scrollbar implemented yet)
    unsigned int m_maxElementsMixer = 11;
    unsigned int m_maxElementsPatches = 7;
};
