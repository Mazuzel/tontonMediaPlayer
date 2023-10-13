#include "ofApp.h"

#include "volumesDb.h"

#include <filesystem>
#include <utility>
namespace fs = std::filesystem;

//--------------------------------------------------------------
void ofApp::setup(){

	ofSetLogLevel(OF_LOG_VERBOSE);

	ofBackground(0);

	// print devices list
	m_midiOutDevices = midiOut.getOutPortList();
	m_audioDevices = soundStream.getDeviceList(ofSoundDevice::MS_WASAPI);

	// print devices selected by config
	loadHwConfig();
	ofLog() << "Midi out devices:";
	for (int idx = 0; idx < m_midiOutDevices.size(); idx++)
	{
		if (idx == m_midiOutputIdx)
		{
			ofLog() << "*" << m_midiOutDevices[idx] << "(selected)";
		}
		else
		{
			ofLog() << m_midiOutDevices[idx];
		}
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
	openAudioOut();

	m_fboSource.allocate(960, 540, GL_RGBA);
	m_fboMapping.allocate(1920, 1080, GL_RGBA);

	// chargement setlist
	ofDirectory dir;
	dir.listDir(m_songsRootDir);

	ofxXmlSettings settings;
	string filePath = "setlist.xml";
	if (settings.loadFile(filePath)) {
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

	metronome.setSampleRate(m_sampleRate);

	loadSong(); // chargement du premier morceau

	m_quadSurfaces.push_back(QuadSurface());

	m_isDefaultShaderLoaded = m_defaultShader.load("shaders/default.vert", "shaders/bad_tv.frag");
}

void ofApp::loadHwConfig() {
	ofxXmlSettings settings;
	string filePath = "settings.xml";
	if (settings.loadFile(filePath)) {
		settings.pushTag("settings");
		m_midiOutputIdx = settings.getValue("midi_out", 0);
		m_audioOutputIdx = settings.getValue("audio_out", 0);
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
	}
	else {
		ofLogError() << "settings.xml not found, using default hw config";
	}
}

int ofApp::openMidiOut() {
	m_isMidiOutOpened = true;
	midiOut.openPort(m_midiOutputIdx); // by number
	// set metronome controls
	metronome.setMidiOut(midiOut);
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

	std::vector<ofSoundDevice> devices = soundStream.getMatchingDevices("Elektron Model:Cycles", UINT_MAX, 2, ofSoundDevice::Api::MS_WASAPI);
	settings.setApi(ofSoundDevice::Api::MS_WASAPI);
	settings.setOutDevice(devices[0]);

	m_isAudioOutOpened = soundStream.setup(settings);
	if (m_isAudioOutOpened)
	{
		soundStream.setOutput(output);
	}
	
	return 0;
}

//--------------------------------------------------------------
void ofApp::update(){
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

	// update the sound playing system:
	ofSoundUpdate();

	if (m_isPlaying && players.size() > 0 && metronome.getTickCount() % 4 == 0)
	{
		double realPlaybackPositionMs = players[0]->getPositionMS();
		metronome.correctTicksToPlaybackPosition(realPlaybackPositionMs);
	}

	if (m_videoLoaded && m_isPlaying)
	{
		float currentSongTimeMs = getCurrentSongTimeMs();
		m_videoClipSource.update(currentSongTimeMs);
	}
}

//--------------------------------------------------------------
void ofApp::drawMapping(ofEventArgs& args){
	m_fboMapping.draw(0, 0, ofGetWidth(), ofGetHeight());
}

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
void ofApp::draw() {
	ofShowCursor();

	m_fboSource.begin();
	ofClear(0);
	ofSetColor(255);
	if (m_setupMappingMode)
	{
		// on dessine un arrière plan au cas où il n'y ait pas de vidéo
		ofBackground(128);
	}
	if (m_isPlaying)
	{
		if (m_videoLoaded)
		{
			m_videoClipSource.draw(m_fboSource.getWidth(), m_fboSource.getHeight());
		}
		m_shadersSource.draw(m_fboSource.getWidth(), m_fboSource.getHeight(), metronome.getTickCount(), metronome.getPlaybackPositionMs() / 1000.0);
	}
	else if (m_isDefaultShaderLoaded)
	{
		m_defaultShader.begin();
		m_defaultShader.setUniform1f("time", ofGetElapsedTimef());
		m_defaultShader.setUniform1f("bpm", 60.0);
		m_defaultShader.setUniform2f("resolution", m_fboSource.getWidth(), m_fboSource.getHeight());
		ofDrawRectangle(0, 0, m_fboSource.getWidth(), m_fboSource.getHeight());
		m_defaultShader.end();
	}
	m_fboSource.end();

	m_fboMapping.begin();
	ofClear(0, 0, 0, 255);
	ofSetColor(255);
	for (int i = 0; i < m_quadSurfaces.size(); i++)
	{
		m_quadSurfaces[i].draw(m_fboSource.getTexture());
	}

	if (m_setupMappingMode)
	{
		drawMappingSetup();
	}
	m_fboMapping.end();

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
		"f: fullscreen, m: mapping, v: sel. stem, n: vol. up, b: vol. down, s: store vols, Q: quit, p: launch next",
		20,
		580);
}

void ofApp::drawSequencerPage()
{
	ofSetColor(255);

	ofDrawBitmapString("Auto play (g)", 680, 15);
	ofSetColor(128);
	if (m_autoPlayNext)
	{
		ofSetColor(50, 255, 25);
	}
	ofDrawRectangle(660, 5, 10, 10);

	ofSetColor(255);

	ofDrawBitmapString("Mixer", 20, 50);
	for (int i = 0; i < players.size(); i++)
	{
		float volume = mixer.getConnectionVolume(i);
		int y = 50 + TEXT_LIST_SPACING * (i + 1);
		ofSetColor(180);
		if (i == m_selectedVolumeSetting)
		{
			ofSetColor(255);
		}
		ofDrawBitmapString(playersNames[i], 290, y + 15);
		ofSetColor(255);
		int volumeBars = round(volume * 20);
		int barWidth = 3;
		int barPeriod = 8;
		ofSetColor(255 * (1.5 - volume), 255 * volume, 100 * (1.5 + volume));
		ofDrawBitmapString(volume, 250, y + 15);
		for (int j = 0; j < volumeBars; j++)
		{
			int xBar = 20 + j * barPeriod;
			ofDrawRectangle(xBar, y, barWidth, TEXT_LIST_SPACING - 2);
		}
	}

	displayList(500, 50, "Setlist (up/down keys to change)", m_setlist, m_currentSongIndex, true);

	ofDrawBitmapString("Song", 20, 450);
	ofSetColor(150);
	ofDrawBitmapString("(<- | N | ->)", 60, 450);
	ofSetColor(50);
	ofDrawRectangle(15, 475, 770, 50);
	int songTicks = m_songEvents[m_songEvents.size() - 1].tick;
	for (int i = 0; i < m_songEvents.size()-1; i++)
	{
		int x = 20 + 760 * (int)m_songEvents[i].tick / songTicks;
		int w = 760 * ((int)m_songEvents[i + 1].tick - (int)m_songEvents[i].tick) / songTicks;

		ofSetColor(
			15 * (m_songEvents[i].program % 16),
			127 * (m_songEvents[i].program % 2),
			255 - 15 * (m_songEvents[i].program % 16)
		);
		ofDrawRectangle(x, 480, w, 40);
		ofSetColor(0, 0, 0);
		ofDrawBitmapString(m_songEvents[i].programName, x + 0.5*w - 10, 500);
		ofSetColor(255);
	}

	ofDrawRectangle(20 + 760 * metronome.getTickCount() / songTicks - 2, 480, 4, 40);
	ofDrawBitmapString(metronome.getTickCount() + 1, 20, 465);
}

void ofApp::displayList(unsigned int x, unsigned int y, string title, vector<string> elements, unsigned int selectedElement, bool showIndex)
{
	ofSetColor(255);
	ofDrawBitmapString(title, x, y);
	for (int i = 0; i < elements.size(); i++)
	{
		if (i == selectedElement){
			ofSetColor(255, 100, 100);
		}
		if (showIndex)
		{
			ofDrawBitmapString(to_string(i), x - 25, y + 20 + TEXT_LIST_SPACING * i);
		}
		ofDrawBitmapString(elements[i], x, y + 20 + TEXT_LIST_SPACING * i);
		if (i == selectedElement){
			ofSetColor(255);
		}
	}
}

//--------------------------------------------------------------
void ofApp::exit() {

	// stop external midi device
	midiOut << StartMidi() << 0xFC << FinishMidi(); // stop playback
	midiOut.sendProgramChange(10, 95);  // back to sync F16 program

	// clean up
	midiOut.closePort();

	// clean up
	if (m_enableMidiIn)
	{
		midiIn.closePort();
		midiIn.removeListener(this);
	}
}

//------------- Changing state --------------------------------

// TODO add lock system to avoid contradictory actions

void ofApp::stopPlayback()
{
	metronome.setEnabled(false);
	midiOut << StartMidi() << 0xFC << FinishMidi();  // stop

	for (int i = 0; i < players.size(); i++) {
		players[i]->stop();
	}
	mixer.setMasterVolume(0);

	m_isPlaying = false;
	ofSleepMillis(4);
}

void ofApp::loadSong()
{
	for (int i = 0; i < players.size(); i++) {
		players[i]->stop();
		players[i]->unload();
		players[i]->disconnect();
	}
	players.clear();
	playersNames.clear();

	m_videoClipSource.closeVideo();

	midiOut << StartMidi() << 0xFC << FinishMidi(); // stop playback

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
	if (settings.loadFile(filePath)) {
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

	// find suitable wav files
	vector<string> trackFilesToLoad;
	for (int i = 0; i < dir.size(); i++) {
		string trackName = fs::path(dir.getPath(i)).filename().string();

		if (!m_stemMode && trackName != "master.wav")
		{
			continue;
		}
		else if (m_stemMode && dir.size() > 1 && trackName == "master.wav")
		{
			// if stem mode we ignore master, excepted when there is only one master audio
			continue;
		}

		trackFilesToLoad.push_back(dir.getPath(i));
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
	midiOut << StartMidi() << 0xFA << FinishMidi();  // start
	midiOut << StartMidi() << 0xF8 << FinishMidi();  // tick
	midiOut << StartMidi() << 0xFC << FinishMidi();  // stop
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

	midiOut << StartMidi() << 0xFA << FinishMidi(); // start playback
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

void ofApp::volumeUp()
{
	if (m_selectedVolumeSetting >= players.size())
	{
		return;
	}

	float volume = mixer.getConnectionVolume(m_selectedVolumeSetting);
	volume = min(1.5, max(0.0, volume + 0.05));
	mixer.setConnectionVolume(m_selectedVolumeSetting, volume);
}

void ofApp::volumeDown()
{
	if (m_selectedVolumeSetting >= players.size())
	{
		return;
	}

	float volume = mixer.getConnectionVolume(m_selectedVolumeSetting);
	volume = min(1.5, max(0.05, volume - 0.05));
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
		stopPlayback();
		loadSong();
		break;
	case OF_KEY_UP:
		stopPlayback();
		if (m_currentSongIndex > 0)
		{
			m_currentSongIndex -= 1;
			loadSong();
		}
		break;
	case 'N':  // next song part
		jumpToNextPart();
		break;
	case OF_KEY_DOWN:
		stopPlayback();
		if (m_currentSongIndex < m_setlist.size() - 1)
		{
			m_currentSongIndex += 1;
			loadSong();
		}
		break;
	case 'p':
		stopPlayback();
		if (m_currentSongIndex < m_setlist.size() - 1)
		{
			m_currentSongIndex += 1;
			loadSong();
			startPlayback();
		}
		break;
	case '0':
		loadSongByIndex(0);
		startPlayback();
		break;
	case '1':
		loadSongByIndex(1);
		startPlayback();
		break;
	case '2':
		loadSongByIndex(2);
		startPlayback();
		break;
	case '3':
		loadSongByIndex(3);
		startPlayback();
		break;
	case '4':
		loadSongByIndex(4);
		startPlayback();
		break;
	case '5':
		loadSongByIndex(5);
		startPlayback();
		break;
	case '6':
		loadSongByIndex(6);
		startPlayback();
		break;
	case '7':
		loadSongByIndex(7);
		startPlayback();
		break;
	case '8':
		loadSongByIndex(8);
		startPlayback();
		break;
	case '9':
		loadSongByIndex(9);
		startPlayback();
		break;
	case 'a':
		loadSongByIndex(10);
		startPlayback();
		break;
	case 'z':
		loadSongByIndex(11);
		startPlayback();
		break;
	case 'e':
		loadSongByIndex(12);
		startPlayback();
		break;
	case 'r':
		loadSongByIndex(13);
		startPlayback();
		break;
	case 't':
		loadSongByIndex(14);
		startPlayback();
		break;
	case 'y':
		loadSongByIndex(15);
		startPlayback();
		break;
	case 'm':
		m_setupMappingMode = !m_setupMappingMode;
		break;
	case 'f':
		mappingWindow->toggleFullscreen();
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
	case 'g':
		m_autoPlayNext = !m_autoPlayNext;
		break;
	case 's':
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
