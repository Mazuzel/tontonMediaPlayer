#include "ofApp.h"

#include <filesystem>
#include <tuple>
#include <utility>

#include "color.h"
#include "midiUtils.h"
#include "volumesDb.h"

namespace fs = std::filesystem;


//--------------------------------------------------------------
void ofApp::setup(){

	ofSetLogLevel(OF_LOG_VERBOSE);
	ofBackground(0);

	loadHwConfig();
	
	{  // print midi out devices
		ofLog() << "--------------------Midi out-------------------------";
		map<int, string> midiOutDevices = getMidiOutDevices();
		for (auto itr = midiOutDevices.begin(); itr != midiOutDevices.end(); ++itr) {
			ofLog() << itr->second << " (port " << itr->first << ")";
		}
		ofLog() << "------------------------------------------------------";
	}

	// ----------------------------------------
	// midi in (clock)
	if (m_enableMidiIn)
	{
		midiIn.listInPorts();
		midiIn.openPort(0);
		midiIn.ignoreTypes(true, // sysex  <-- ignore timecode messages!
			true, // timing <-- ignore clock messages!
			true); // sensing
		// add ofApp as a listener
		midiIn.addListener(this);
	}

	// ----------------------------------------
	openMidiOut();
	// set metronome controls
	metronome.setMidiOuts(midiOuts);
	metronome.setLoopMode(m_loop);

	ofLog() << "init midi out";
	openAudioOut();
	ofLog() << "init audio out";

	// chargement setlist
	ofDirectory dir;
	dir.listDir(m_songsRootDir);

	ofxXmlSettings settings;
	string filePath = "setlist.xml";
	if (settings.load(filePath)) {
		settings.pushTag("setlist");
		int numberOfSongs = settings.getNumTags("song");
		for (int i = 0; i < numberOfSongs; i++) {
			settings.pushTag("song", i);
			std::string songName = settings.getValue("name", "");
			// TODO vérifications
			m_setlist.push_back(songName);
			settings.popTag();
		}
	}
	else {
		ofLogError() << "setlist.xml not found, creating a default setlist";
		for (int i = 0; i < dir.size(); i++) {
			m_setlist.push_back(dir.getName(i));
		}
	}
	ofLog() << "init setlist";

	metronome.setSampleRate(m_sampleRate);

	loadSong(); // chargement du premier morceau
	ofLog() << "init first song";

	m_quadSurfaces.push_back(QuadSurface());
	if (m_mappingConfigFileOverride.size() > 0)
	{
		loadPiMapperSurfaces();
	}
	else
	{
		loadMappingNodes();
	}

	for (uint32_t i = 0; i < m_quadSurfaces.size(); i++)
	{
		ofFbo fbo;
		fbo.allocate(960, 540, GL_RGBA);
		m_fboSources.push_back(fbo);
	}
	m_fboMapping.allocate(1920, 1080, GL_RGBA);

	m_isDefaultShaderLoaded = m_defaultShader.load("shaders/default.vert", "shaders/bad_tv.frag");
	ofLog() << "init default shader";
    
    ofSetFrameRate(m_audioRefreshRate);

	// load logo
	m_logo = ofImage("TontonMediaPlayerLogo.png");
}

void ofApp::loadHwConfig() {
	ofxXmlSettings settings;
	string filePath = "settings.xml";
	if (settings.load(filePath)) {
		settings.pushTag("settings");
		if (settings.tagExists("midi_out_ports"))
		{
			settings.pushTag("midi_out_ports");
			int nbMidiOut = settings.getNumTags("midi_out");
			for (int i = 0; i < nbMidiOut; i++) {
				settings.pushTag("midi_out", i);
				unsigned int midiPort = settings.getValue("port", 0);
				m_midiOutPorts.push_back(midiPort);
				ofLog() << "Midi output port settings: " << midiPort;
				settings.popTag();
			}
			settings.popTag();
		}

		m_midiInputIdx = settings.getValue("midi_input", 0);
		m_bufferSize = settings.getValue("buffer_size", 128);
		if (settings.tagExists("songs_root_dir"))
		{
			m_songsRootDir = settings.getValue("songs_root_dir", "songs/");
		}
		m_videoStartDelayMs = settings.getValue("video_start_delay_ms", 0);
		if (settings.tagExists("video_speed_change_delay_ms"))
		{
			m_videoClipSource.setSpeedChangeDelay(settings.getValue("video_speed_change_delay_ms", 30.0));
		}
		if (settings.tagExists("nb_ignored_startup_ticks"))
		{
			metronome.setNbIgnoredStartupsTicks(settings.getValue("nb_ignored_startup_ticks", 4));
		}
		if (settings.tagExists("sample_rate"))
		{
			m_sampleRate = settings.getValue("sample_rate", 22050);
		}
        if (settings.tagExists("requested_audio_out_device"))
        {
            m_requestedAudioOutDevice = settings.getValue("requested_audio_out_device", "");
			ofLog() << "Requested audio out device value from settings: " << m_requestedAudioOutDevice;
        }
        if (settings.tagExists("requested_audio_out_api"))
        {
            string value = settings.getValue("requested_audio_out_api", "");
			ofLog() << "Requested audio out API value from settings: " << value;
			for (int i = ofSoundDevice::ALSA; i < ofSoundDevice::NUM_APIS; i++)
			{
				if (value == to_string((ofSoundDevice::Api)i))
				{
					m_requestedAudioOutApi = (ofSoundDevice::Api)i;
				}
			}
        }
		if (settings.tagExists("load_pi_mapper_config"))
		{
			string value = settings.getValue("load_pi_mapper_config", "");
			if (value.size() > 0)
			{
				m_mappingConfigFileOverride = value;
			}
		}
		if (settings.tagExists("ignore_audio_files"))
		{
			settings.pushTag("ignore_audio_files");
			int nbIgnored = settings.getNumTags("containing");
			for (int i = 0; i < nbIgnored; i++)
			{
				settings.pushTag("containing", i);
				std::string value = settings.getValue("value", "");
				if (value.size() > 0)
				{
					transform(value.begin(), value.end(), value.begin(), ::tolower);
					m_audioFilesIgnoreIfContains.push_back(value);
				}
				settings.popTag();
			}
			settings.popTag();
			for (auto str : m_audioFilesIgnoreIfContains)
			{
				ofLog() << "files will be ignored if they contain this : " << str;
			}
			
		}
	}
	else {
		ofLogError() << "settings.xml not found, using default hw config";
	}
}

int ofApp::openMidiOut() {

	for (auto midiPort: m_midiOutPorts)
	{
		ofxMidiOut midiOut;
		midiOut.openPort(midiPort);
		if (!midiOut.isOpen())
		{
			ofLogError() << "Could not open midi out on port " << midiPort;
		}
		else
		{
			midiOuts.push_back(midiOut);
			ofLog() << "Midi out successfully opened on port " << midiPort;
		}
	}
	return 0;
}

int ofApp::openAudioOut()
{
	ofSoundStreamSettings settings;
	settings.setOutListener(this);
	settings.sampleRate = m_sampleRate;
	settings.numOutputChannels = 2;
    settings.numInputChannels = 0;
	settings.bufferSize = m_bufferSize;
	settings.numBuffers = 1;

	ofLog() << "--------------------Audio out-------------------------";
	soundStream.printDeviceList();

	ofLog() << "----     Connecting...";

    if (m_requestedAudioOutDevice.size() > 0)
    {
        std::vector<ofSoundDevice> devices = soundStream.getMatchingDevices(m_requestedAudioOutDevice, UINT_MAX, 2, ofSoundDevice::MS_WASAPI);
        if (devices.size() == 0)
        {
            ofLogError() << "Audio out device not found: " << m_requestedAudioOutDevice;
            ofLog() << "Trying to connect to default audio device";
        }
        else
        {
            settings.setApi(m_requestedAudioOutApi);
            settings.setOutDevice(devices[0]);
        }
    }

	m_isAudioOutOpened = soundStream.setup(settings);
	if (m_isAudioOutOpened)
	{
		soundStream.setOutput(output);
        if (soundStream.getSoundStream() != nullptr)
        {
			string fullDevName = soundStream.getSoundStream()->getOutDevice().name;
			int idx0 = fullDevName.find("(");
			int idx1 = fullDevName.find(")");
			if (idx1 > idx0)
				m_openedAudioDeviceName = fullDevName.substr(idx0 + 1, idx1 - idx0 - 1);
			else
				m_openedAudioDeviceName = fullDevName;
            m_openedAudioDeviceApi = soundStream.getSoundStream()->getOutDevice().api;
        }
	}

	ofLog() << "------------------------------------------------------";
	
	return 0;
}

//--------------------------------------------------------------
void ofApp::update(){
	// AUDIO UPDATE
	if (metronome.isSongEnded())
	{
		stopPlayback();

		if (m_autoPlayNext)
		{
			if (m_currentSongIndex < m_setlist.size() - 1)
			{
				m_currentSongIndex += 1;
				loadSong();
				startPlayback();
			}
		}
	}

	if (metronome.loopEndReached())
	{
		jumpToPreviousPart();
	}

	// update the sound playing system:
	ofSoundUpdate();

	if (m_isPlaying && players.size() > 0 && metronome.getTickCount() % 4 == 0)
	{
		double realPlaybackPositionMs = players[0]->getPositionMS();
		metronome.correctTicksToPlaybackPosition(realPlaybackPositionMs);
	}

	// VIDEO UPDATE
	auto time = ofGetElapsedTimef();
	if ((time - m_lastVideoRefreshTime) < (1.0 / m_videoRefreshRate))
	{
		// no video update
		return;
	}
	m_lastVideoRefreshTime = time;

	if (m_videoLoaded && m_isPlaying)
	{
		float currentSongTimeMs = getCurrentSongTimeMs();
		m_videoClipSource.update(m_videoResync, currentSongTimeMs, m_measuredVideoDelayMs);
	}
    
    // drawing into fbo
	for (uint32_t i = 0; i < m_fboSources.size(); i++)
	{
		m_fboSources[i].begin();
		ofClear(0);
		ofSetColor(255);
		if (m_setupMappingMode)
		{
			// on dessine un arrière plan au cas où il n'y ait pas de vidéo
			ofBackground(50);
		}
		if (m_isPlaying)
		{
			if (m_videoLoaded)
			{
				m_videoClipSource.draw(m_fboSources[i].getWidth(), m_fboSources[i].getHeight());
			}
			m_shadersSource.draw(m_fboSources[i].getWidth(), m_fboSources[i].getHeight(), metronome.getTickCount(), metronome.getPlaybackPositionMs() / 1000.0, i);
		}
		else if (m_isDefaultShaderLoaded)
		{
			m_defaultShader.begin();
			m_defaultShader.setUniform1f("time", ofGetElapsedTimef());
			m_defaultShader.setUniform1f("bpm", 60.0);
			m_defaultShader.setUniform2f("resolution", m_fboSources[i].getWidth(), m_fboSources[i].getHeight());
			ofDrawRectangle(0, 0, m_fboSources[i].getWidth(), m_fboSources[i].getHeight());
			m_defaultShader.end();
		}
		m_fboSources[i].end();
	}
}

//--------------------------------------------------------------
void ofApp::drawMapping(ofEventArgs& args){
	m_fboMapping.draw(0, 0, ofGetWidth(), ofGetHeight());
}

//--------------------------------------------------------------
void ofApp::drawMappingSetup()
{
	for (int i = 0; i < m_quadSurfaces.size(); i++)
	{
		vector<Vec3> vertices = m_quadSurfaces[i].getVertices();
		for (int j = 0; j < 4; j++)
		{
			ofSetColor(255, 50, 150);
			if (i == m_quadMovedIdx && j == m_quadMovedVertexIdx)
			{
				ofSetColor(10, 250, 50);
			}
			ofDrawRectangle(
				vertices[j].x - QUAD_CORNER_HWIDTH,
				vertices[j].y - QUAD_CORNER_HWIDTH,
				2 * QUAD_CORNER_HWIDTH,
				2 * QUAD_CORNER_HWIDTH);
		}
	}

	ofSetColor(255);
}

//--------------------------------------------------------------
void ofApp::drawAnimatedLogo()
{
	ofSetColor(200);
	m_logo.draw(320, 150, 220, 250);

	int xOffset = 0;
	int yOffset = 0;
	ofSetColor(20, 30, 50);
	if (m_currentSongIndex != m_songSelectorToolIdx)
	{
		ofSetColor(180, 90, 25);
		xOffset = 4;
		yOffset = m_songSelectorToolIdx - 5;
	}
	else if (m_isPlaying)
	{
		ofSetColor(80, 90, 250);
		int songTicks = m_songEvents[m_songEvents.size() - 1].tick;
		float progressOffset = (metronome.getTickCount() - 0.5 * songTicks) / songTicks;
		xOffset = int(progressOffset * 5);
		yOffset = int(xOffset / 2.0);
		if (yOffset < 0)
		{
			yOffset = -yOffset;
		}
		yOffset += 3;
	}
	ofDrawCircle(417 + xOffset, 220 + yOffset, 4);
	ofDrawCircle(445 + xOffset, 220 + yOffset, 4);
	ofSetColor(255);
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofShowCursor();
	ofSetColor(255);

	// draw logo with eyes animation
	if (!m_setupMappingMode)
	{
		drawAnimatedLogo();
	}

	m_fboMapping.begin();
	ofClear(0, 0, 0, 255);
	ofSetColor(255);
	for (int i = 0; i < m_quadSurfaces.size(); i++)
	{
		m_quadSurfaces[i].draw(m_fboSources[i].getTexture());
	}

	if (m_setupMappingMode)
	{
		drawMappingSetup();
	}
	m_fboMapping.end();
    
    ofSetColor(255);
    
    // mini-view of the video window
    m_fboSources[0].draw(20, 260, 300, 200);
    
    std::stringstream strmAudioOut;
    strmAudioOut << "Audio out: " << m_openedAudioDeviceName << "(" << toString(m_openedAudioDeviceApi) << ")";
    ofDrawBitmapString(strmAudioOut.str(), 20, 15);
    
    std::stringstream strmFps;
    strmFps << round(ofGetFrameRate()) << " fps";
    ofDrawBitmapString(strmFps.str(), 420, 15);

	ofSetColor(255);
	if (m_setupMappingMode)
	{
		m_fboMapping.draw(0, 0, ofGetWidth(), ofGetHeight());
	}
	else
	{
		drawSequencerPage();
	}
	drawHelp();
}

void ofApp::drawHelp()
{
	ofSetColor(150);
	ofDrawBitmapString(
		"f:fullscreen, m:mapping, s:store vols, Q:quit, p:launch next, l:loop",
		10,
		580);
}

void ofApp::drawSequencerPage()
{
	ofSetColor(255);

	ofDrawBitmapString("Auto play (w)", 680, 15);
	ofSetColor(128);
	if (m_autoPlayNext)
	{
		ofSetColor(50, 255, 25);
	}
	ofDrawRectangle(660, 5, 10, 10);

	ofSetColor(255);

	ofDrawBitmapString("Video resync (x)", 520, 15);
	ofSetColor(128);
	if (m_videoResync)
	{
		ofSetColor(50, 255, 25);
	}
	ofDrawRectangle(500, 5, 10, 10);

	ofSetColor(255);

	ofDrawBitmapString("Mixer (v: scroll, b: increase, n: decrease", 20, 50);
	for (int i = 0; i < players.size(); i++)
	{
		float volume = mixer.getConnectionVolume(i);
		int y = 50 + TEXT_LIST_SPACING * (i + 1);
		ofSetColor(180);
		if (i == m_selectedVolumeSetting)
		{
			ofSetColor(255);
		}
		ofDrawBitmapString(playersNames[i], 280, y + 15);
		ofSetColor(255);
		float volumeNorm = volume / VOLUME_MAX;
		int volumeBars = round(volumeNorm * 40);
		int barWidth = 2;
		int barPeriod = 5;
		ofSetColor(255 * volumeNorm, 128, 255 * (1.0 - volumeNorm));
		ofDrawBitmapString(volume, 230, y + 15);
		for (int j = 0; j < volumeBars; j++)
		{
			int xBar = 20 + j * barPeriod;
			ofDrawRectangle(xBar, y, barWidth, TEXT_LIST_SPACING - 2);
		}
	}

	displayList(550, 50, "Setlist (up/down/return)", m_setlist, m_currentSongIndex, m_songSelectorToolIdx, true);

    int ySeq = 480;
    ofDrawBitmapString("Measured video delay (ms):", 500, ySeq);
    ofDrawBitmapString(m_measuredVideoDelayMs, 710, ySeq);
    
	ofDrawBitmapString("Song", 20, ySeq);
	ofSetColor(150);
	ofDrawBitmapString("(<- | N | ->)", 60, ySeq);
	ofSetColor(50);
	ofDrawRectangle(15, ySeq + 25, 770, 60);

	int songTicks = m_songEvents[m_songEvents.size() - 1].tick;
	for (int i = 0; i < m_songEvents.size()-1; i++)
	{
		int x = 20 + static_cast<int>(760 * m_songEvents[i].tick / songTicks);
		int w = static_cast<int>(760 * (m_songEvents[i + 1].tick - m_songEvents[i].tick + 1) / songTicks);

		float hue = ((2 * m_songEvents[i].program) % 16) * 300.0 / 16.0 + i * 60.0 / m_songEvents.size();
		int R, G, B;
		tie(R, G, B) = Tonton::Utils::HSVtoRGB(hue, 50, 95);
		ofSetColor(R, G, B);
		ofDrawRectangle(x, ySeq + 30, w, 50);
		ofSetColor(0, 0, 0);
		ofDrawBitmapString(m_songEvents[i].programName, x + 0.5 * w - 10, ySeq + 60);
	}
	ofSetColor(255);

	if (m_loop)
	{
		ofSetColor(200, 180, 120);
	}
	ofDrawRectangle(20 + 760 * metronome.getTickCount() / songTicks - 2, ySeq + 30, 6, 50);  // player cursor
	ofSetColor(255);

	ofDrawBitmapString(metronome.getTickCount() + 1, 20, ySeq + 15);
}

void ofApp::displayList(unsigned int x, unsigned int y, string title, vector<string> elements, unsigned int activeElement, unsigned int selectedElement, bool showIndex)
{
	ofSetColor(255);
	ofDrawBitmapString(title, x - 20, y);
	for (int i = 0; i < elements.size(); i++)
	{
		if (i == activeElement){
			ofSetColor(255, 100, 100);
		}
		else if (i == selectedElement) {
			ofSetColor(200, 130, 50);
		}
		if (showIndex)
		{
			ofDrawBitmapString(to_string(i), x - 25, y + 20 + TEXT_LIST_SPACING * i);
		}
		ofDrawBitmapString(elements[i], x, y + 20 + TEXT_LIST_SPACING * i);
		ofSetColor(255);
	}
}

//--------------------------------------------------------------
void ofApp::exit() {

	// stop external midi device
	for (auto midiOut: midiOuts)
	{
		if (midiOut.isOpen())
		{
			midiOut << StartMidi() << 0xFC << FinishMidi(); // stop playback
			midiOut.sendProgramChange(10, 95);  // back to sync F16 program

			// clean up
			midiOut.closePort();
		}
	}

	// clean up
	if (m_enableMidiIn)
	{
		midiIn.closePort();
		midiIn.removeListener(this);
	}
}

//------------- Changing state --------------------------------

void ofApp::stopPlayback()
{
    mixer.setMasterVolume(0);
	metronome.setEnabled(false);
	for (auto midiOut: midiOuts)
	{
		if (midiOut.isOpen())
		{
			midiOut << StartMidi() << 0xFC << FinishMidi();  // stop
		}
	}


	for (int i = 0; i < players.size(); i++) {
		players[i]->stop();
	}

	m_isPlaying = false;
	ofSleepMillis(4);
}

void ofApp::loadSong()
{
	m_songSelectorToolIdx = m_currentSongIndex;
	for (int i = 0; i < players.size(); i++) {
		players[i]->stop();
		players[i]->unload();
		players[i]->disconnect();
	}
	players.clear();
	playersNames.clear();

	m_videoClipSource.closeVideo();

	for (auto midiOut: midiOuts)
	{
		if (midiOut.isOpen())
		{
			midiOut << StartMidi() << 0xFC << FinishMidi(); // stop playback
		}
	}

	// load audio
	ofDirectory dir;
	dir.allowExt("wav");
	dir.allowExt("flac");

	string songName = m_setlist[m_currentSongIndex];

	dir.listDir(m_songsRootDir + songName + "/export/audio");

	m_videoLoaded = false;
	if (ofFile(m_songsRootDir + songName + "/export/clip/clip.mp4").exists())
	{
		m_videoClipSource.loadVideo(m_songsRootDir + songName + "/export/clip/clip.mp4");
		m_videoLoaded = true;
	}

	m_songEvents.clear();

	ofxXmlSettings settings;
	string filePath = m_songsRootDir + songName + "/export/structure.xml";
	if (settings.load(filePath)) {
		settings.pushTag("structure");
		settings.pushTag("songparts");
		int numberOfParts = settings.getNumTags("songpart");
		for (int i = 0; i < numberOfParts; i++) {
			settings.pushTag("songpart", i);
			songEvent e;
			e.bpm = settings.getValue("bpm", 0.0);
			e.programName = settings.getValue("program", "F16");
			char bankName = e.programName[0];
			int bankOffset = 0;
			switch (bankName) {
			case 'A':
				break;
			case 'B':
				bankOffset = 16;
				break;
			case 'C':
				bankOffset = 32;
				break;
			case 'D':
				bankOffset = 48;
				break;
			case 'E':
				bankOffset = 64;
				break;
			case 'F':
				bankOffset = 80;
				break;
			}

			std::stringstream strm;
			strm << e.programName.substr(1, 2);
			int num = std::stoi(strm.str());
			
			e.program = bankOffset + num - 1;
			e.tick = settings.getValue("tick", 0);
			if (settings.tagExists("shader"))
			{
				e.shader = settings.getValue("shader", "");
			}
			m_songEvents.push_back(e);
			settings.popTag();
		}
	}
	else {
		ofLogError() << "Impossible de charger " + filePath;
		return;
	}

	// find suitable audio files
	vector<string> trackFilesToLoad;
	for (int i = 0; i < dir.size(); i++) {
		string trackName = fs::path(dir.getPath(i)).filename().string();
		bool ignoreFile = false;

		string lowerTrackName = trackName;
		transform(lowerTrackName.begin(), lowerTrackName.end(), lowerTrackName.begin(), ::tolower);
		for (auto ignoreString : m_audioFilesIgnoreIfContains)
		{
			if (lowerTrackName.find(ignoreString) != string::npos)
			{
				ofLog() << "File " + trackName + " ignored";
				ignoreFile = true;
				continue;
			}
		}
		if (!ignoreFile)	trackFilesToLoad.push_back(dir.getPath(i));
	}

	// create players
	players.resize(trackFilesToLoad.size());
	m_selectedVolumeSetting = 0;
	for (int i = 0; i < trackFilesToLoad.size(); i++)
	{
		string trackName = fs::path(trackFilesToLoad[i]).filename().string();
		playersNames.push_back(trackName);
		players[i] = make_unique<ofxSoundPlayerObject>();
		players[i]->setLoop(false);
		players[i]->load(ofToDataPath(trackFilesToLoad[i]));
	}

	for (int i = 0; i < players.size(); i++) {
		players[i]->connectTo(mixer);
	}

	// load volumes from database
	try
	{
		vector<pair<string, float>> storedVolumes;
		for (int i = 0; i < playersNames.size(); i++)
		{
			storedVolumes.push_back(make_pair(playersNames[i], 1.0));
		}
		VolumesDb::getStoredSongVolumes(songName, storedVolumes);

		for (int i = 0; i < storedVolumes.size(); i++)
		{
			string stem = storedVolumes[i].first;
			float volume = storedVolumes[i].second;
			for (int j = 0; j < playersNames.size(); j++)
			{
				if (stem == playersNames[j] && j < players.size())
				{
					mixer.setConnectionVolume(j, volume);
				}
			}
		}
	}
	catch (const std::exception& e) {
		ofLog() << "could not load song volumes, an error occured: " << e.what();
	}

	// load shaders
	m_shadersSource.setup(m_songEvents);

	// configure output device and metronome
	metronome.setNewSong(m_songEvents);
	metronome.sendNextProgramChange();  // envoi du premier pch
}

double ofApp::getCurrentSongTimeMs()
{
	return metronome.getPlaybackPositionMs();
}

void ofApp::startPlayback()
{
	// force midi device to go to the first pattern
	ofSleepMillis(2);
	for (auto midiOut: midiOuts)
	{
		if (midiOut.isOpen())
		{
			midiOut << StartMidi() << 0xFA << FinishMidi();  // start
			midiOut << StartMidi() << 0xF8 << FinishMidi();  // tick
			midiOut << StartMidi() << 0xFC << FinishMidi();  // stop
		}
	}

	ofSleepMillis(2);

	// chain components
	mixer.connectTo(metronome).connectTo(output);

	unsigned int currentSongPartIdx = metronome.getCurrentSongPartIdx();
	int msTime = 0.0;
	for (int i = 1; i <= currentSongPartIdx; i++)
	{
		int ticks = m_songEvents[i].tick - m_songEvents[i - 1].tick;
		msTime += ticks * 1000.0 / m_songEvents[i - 1].bpm * 60.0;
	}

	float videoStartTime = (msTime + m_videoStartDelayMs) / 1000.0;  // m_videoStartDelayMs is an offset for latency compensation
	m_videoClipSource.playVideo(videoStartTime);

	for (auto midiOut: midiOuts)
	{
		if (midiOut.isOpen())
		{
			midiOut << StartMidi() << 0xFA << FinishMidi(); // start playback
		}
	}

	for (int i = 0; i < players.size(); i++) {
		players[i]->play();
		if (currentSongPartIdx > 0)
		{
			players[i]->setPositionMS(msTime, 0);
		}
		
	}
	metronome.setEnabled(true);

	mixer.setMasterVolume(1.0); // TODO config

	m_isPlaying = true;
}

void ofApp::jumpToNextPart()
{
	bool playingBeforeAction = m_isPlaying;

	if (playingBeforeAction)
	{
		stopPlayback();
	}

	unsigned int currentSongPartIdx = metronome.getCurrentSongPartIdx();
	if (currentSongPartIdx < m_songEvents.size() - 1)
	{
		metronome.setCurrentSongPartIdx(currentSongPartIdx + 1);
		metronome.sendNextProgramChange();
	}

	if (playingBeforeAction)
	{
		startPlayback();
	}
}

void ofApp::jumpToPreviousPart()
{
	bool playingBeforeAction = m_isPlaying;

	if (playingBeforeAction)
	{
		stopPlayback();
	}

	unsigned int currentSongPartIdx = metronome.getCurrentSongPartIdx();
	if (currentSongPartIdx > 0)
	{
		metronome.setCurrentSongPartIdx(currentSongPartIdx - 1);
		metronome.sendNextProgramChange();
	}

	if (playingBeforeAction)
	{
		startPlayback();
	}
}

void ofApp::volumeUp()
{
	if (m_selectedVolumeSetting >= players.size())
	{
		return;
	}

	float volume = mixer.getConnectionVolume(m_selectedVolumeSetting);
	// stick volume to closest step value (to avoid offset due to min value in volumeDown func)
	volume = round(volume * 20) / 20.0;

	volume = min(VOLUME_MAX, volume + 0.05);
	mixer.setConnectionVolume(m_selectedVolumeSetting, volume);
}

void ofApp::volumeDown()
{
	if (m_selectedVolumeSetting >= players.size())
	{
		return;
	}

	float volume = mixer.getConnectionVolume(m_selectedVolumeSetting);
	volume = max(0.01, volume - 0.05);  // avoid null volumes, it stops stem playback
	mixer.setConnectionVolume(m_selectedVolumeSetting, volume);
}

void ofApp::loadSongByIndex(unsigned int index)
{
	if (index >= m_setlist.size())
	{
		return;
	}
	m_currentSongIndex = index;
	loadSong();
}

void ofApp::saveMappingNodes()
{
    ofxXmlSettings settings;

    settings.addTag("surfaces");
    settings.pushTag("surfaces");
    for (int i = 0; i < m_quadSurfaces.size(); i++)
    {
        settings.addTag("surface");
        settings.pushTag("surface", i);
        vector<Vec3> vertices = m_quadSurfaces[i].getVertices();
        for (int j = 0; j < vertices.size(); j++)
        {
            settings.addTag("vertex");
            settings.pushTag("vertex", j);
            settings.addValue("x", int(vertices[j].x));
            settings.addValue("y", int(vertices[j].y));
            settings.popTag();
        }
        settings.popTag();
    }
    settings.popTag();
    settings.save("mapping.xml");
}

void ofApp::loadPiMapperSurfaces()
{
	ofxXmlSettings settings;
	if (!settings.load(m_mappingConfigFileOverride))
	{
		ofLogError() << "Failed to load ofx::piMapper file (xml)";
		return;
	}
	settings.pushTag("surfaces");
	int numberOfSurfaces = settings.getNumTags("surface");
	m_quadSurfaces.clear();
	for (int i = 0; i < numberOfSurfaces; i++)
	{
		m_quadSurfaces.push_back(QuadSurface());
		settings.pushTag("surface", i);
		settings.pushTag("vertices");
		int numberOfVertices = settings.getNumTags("vertex");
		if (numberOfVertices != 4)
		{
			ofLogError() << "unexpected surface in mapping.xml, with nb vertices = " << numberOfVertices;
			break;
		}
		vector<Vec3> vertices = m_quadSurfaces[i].getVertices();
		for (int j = 0; j < numberOfVertices; j++)
		{
			settings.pushTag("vertex", j);
			vertices[j].x = settings.getValue("x", 0);
			vertices[j].y = settings.getValue("y", 0);
			settings.popTag();
		}
		m_quadSurfaces[i].setVertices(vertices);
		settings.popTag();
		settings.popTag();
	}
}

void ofApp::loadMappingNodes()
{
    ofxXmlSettings settings;
    if (!settings.load("mapping.xml"))
    {
        ofLogError() << "Failed to load mapping file (mapping.xml)";
        return;
    }
    settings.pushTag("surfaces");
    int numberOfSurfaces = settings.getNumTags("surface");
	m_quadSurfaces.clear();
    for (int i = 0; i < numberOfSurfaces; i++)
    {
		m_quadSurfaces.push_back(QuadSurface());
        settings.pushTag("surface", i);
        int numberOfVertices = settings.getNumTags("vertex");
        if (numberOfVertices != 4)
        {
            ofLogError() << "unexpected surface in mapping.xml, with nb vertices = " << numberOfVertices;
            break;
        }
        vector<Vec3> vertices = m_quadSurfaces[i].getVertices();
        for (int j = 0; j < numberOfVertices; j++)
        {
            settings.pushTag("vertex", j);
            vertices[j].x = settings.getValue("x", 0);
            vertices[j].y = settings.getValue("y", 0);
            settings.popTag();
        }
        m_quadSurfaces[i].setVertices(vertices);
        settings.popTag();
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key) {
	case 'Q':
		OF_EXIT_APP(0);
		break;
	case OF_KEY_RIGHT:
		startPlayback();
		break;
	case OF_KEY_LEFT:
        if (!m_isPlaying && (ofGetElapsedTimef() - m_lastSongReloadTime < 1.0))
            break;
		stopPlayback();
		loadSong();
        m_lastSongReloadTime = ofGetElapsedTimef();
		break;
	case OF_KEY_UP:
		if (m_songSelectorToolIdx > 0){
			m_songSelectorToolIdx -= 1;
		}
		break;
	case OF_KEY_RETURN:
		if (ofGetElapsedTimef() - m_lastSongChangeTime < 0.1)
			break;
		stopPlayback();
		m_currentSongIndex = m_songSelectorToolIdx;
		loadSong();
		m_lastSongChangeTime = ofGetElapsedTimef();
		break;
	case 'N':  // next song part
		jumpToNextPart();
		break;
	case OF_KEY_DOWN:
		if (m_songSelectorToolIdx < m_setlist.size() - 1) {
			m_songSelectorToolIdx += 1;
		}
		break;
	case 'p':
        if (ofGetElapsedTimef() - m_lastSongChangeTime < 0.1)
            break;
		stopPlayback();
		if (m_currentSongIndex < m_setlist.size() - 1)
		{
			m_currentSongIndex += 1;
			loadSong();
			startPlayback();
		}
        m_lastSongChangeTime = ofGetElapsedTimef();
		break;
	case 'm':
        if (m_setupMappingMode)
        {
            saveMappingNodes();
        }
		m_setupMappingMode = !m_setupMappingMode;
		break;
	case 'f':
		if (mappingWindow != nullptr)
		{
			mappingWindow->toggleFullscreen();
		}
		break;
	case 'v':
		m_selectedVolumeSetting = (m_selectedVolumeSetting + 1) % players.size();
		break;
	case 'n':
		volumeUp();
		break;
	case 'b':
		volumeDown();
		break;
	case 'w':
		m_autoPlayNext = !m_autoPlayNext;
		break;
	case 'x':
		m_videoResync = !m_videoResync;
		break;
	case 's':
	{
		// store volumes
		vector<pair<string, float>> volumes;
		for (int i = 0; i < players.size(); i++)
		{
			float volume = mixer.getConnectionVolume(i);
			string stem = playersNames[i];
			volumes.push_back(make_pair(stem, volume));
		}
		VolumesDb::setStoredSongVolumes(m_setlist[m_currentSongIndex], volumes);
		break;
	}
	case 'l':
		m_loop = !m_loop;
		metronome.setLoopMode(m_loop);
		break;
	}
}

//--------------------------------------------------------------
void ofApp::newMidiMessage(ofxMidiMessage& message) {

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

	if (m_setupMappingMode && m_quadMovedIdx >= 0 && m_quadMovedVertexIdx >= 0)
	{
		int xCoordInMappingFbo = x * m_fboMapping.getWidth() / ofGetWidth();
		int yCoordInMappingFbo = y * m_fboMapping.getHeight() / ofGetHeight();

		xCoordInMappingFbo = min(xCoordInMappingFbo, int(m_fboMapping.getWidth()));
		xCoordInMappingFbo = max(xCoordInMappingFbo, 0);

		yCoordInMappingFbo = min(yCoordInMappingFbo, int(m_fboMapping.getHeight()));
		yCoordInMappingFbo = max(yCoordInMappingFbo, 0);

		vector<Vec3> vertices = m_quadSurfaces[m_quadMovedIdx].getVertices();
		vertices[m_quadMovedVertexIdx].x = xCoordInMappingFbo;
		vertices[m_quadMovedVertexIdx].y = yCoordInMappingFbo;
		m_quadSurfaces[m_quadMovedIdx].setVertices(vertices);
	}
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	if (m_setupMappingMode)
	{
		for (int i = 0; i < m_quadSurfaces.size(); i++)
		{
			vector<Vec3> vertices = m_quadSurfaces[i].getVertices();
			for (int j = 0; j < 4; j++)
			{
				int xInScreen = vertices[j].x * ofGetWidth() / m_fboMapping.getWidth();
				int yInScreen = vertices[j].y * ofGetHeight() / m_fboMapping.getHeight();
				if (abs(x - xInScreen) < QUAD_CORNER_HWIDTH && abs(y - yInScreen) < QUAD_CORNER_HWIDTH)
				{
					m_quadMovedIdx = i;
					m_quadMovedVertexIdx = j;
				}
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	m_quadMovedIdx = -1;
	m_quadMovedVertexIdx = -1;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
