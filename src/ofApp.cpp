#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

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

	// wait for the user to press enter to confirm


	// ----------------------------------------
	// midi in (clock)
	//midiIn.openPort(0);
	//midiIn.ignoreTypes(false, // sysex  <-- don't ignore timecode messages!
	//	false, // timing <-- don't ignore clock messages!
	//	true); // sensing
	// add ofApp as a listener
	//midiIn.addListener(this);

	// ----------------------------------------
	openMidiOut();
	openAudioOut();

	// Enable or disable audio for video sources globally
	ofx::piMapper::VideoSource::enableAudio = false;
	m_piMapper.registerFboSource(m_videoClipSource);

	m_piMapper.setup();

	// chargement setlist
	ofDirectory dir;
	dir.listDir("songs");

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

	loadSong(); // chargement du premier morceau
}

void ofApp::loadHwConfig() {
	ofxXmlSettings settings;
	string filePath = "settings.xml";
	if (settings.loadFile(filePath)) {
		settings.pushTag("settings");
		m_midiOutputIdx = settings.getValue("midi_out", 0);
		m_audioOutputIdx = settings.getValue("audio_out", 0);
	}
	else {
		ofLogError() << "settings.xml not found, using default hw config";
	}
}


void ofApp::exitButtonPressed(const void* sender) {
	OF_EXIT_APP(0);
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
	settings.sampleRate = 44100;
	settings.numOutputChannels = 2;
	settings.numInputChannels = 0;
	settings.bufferSize = 256;
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
	}

	// update the sound playing system:
	ofSoundUpdate();

	m_piMapper.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
	m_piMapper.draw();
}

//--------------------------------------------------------------
void ofApp::drawGui(ofEventArgs& args) {
	ofShowCursor();

	m_buttonExit.draw();

	drawSequencerPage();
}

void ofApp::drawSequencerPage()
{
	ofDrawBitmapString("Mixer", 20, 50);

	displayList(500, 50, "Setlist", m_setlist, m_currentSongIndex);

	ofDrawBitmapString("Song", 20, 450);
	ofSetColor(50);
	ofDrawRectangle(15, 475, 770, 50);
	for (int i = 0; i < m_songEvents.size()-1; i++)
	{
		int x = 20 + (int)m_songEvents[i].tick;
		int w = (int)m_songEvents[i+1].tick - (int)m_songEvents[i].tick;
		int pc = (m_songEvents[i].program % 16) * 64;
		ofSetColor((64 + pc) % 256, (32 + pc) % 256, (255 - pc) % 256);
		ofDrawRectangle(x, 480, w, 40);
		ofSetColor(255);
	}
}

void ofApp::displayList(unsigned int x, unsigned int y, string title, vector<string> elements, unsigned int selectedElement)
{
	ofDrawBitmapString(title, x, y);
	for (int i = 0; i < elements.size(); i++)
	{
		if (i == selectedElement){
			ofSetColor(255, 100, 100);
		}
		ofDrawBitmapString(elements[i], x, y + 20 + 15 * i);
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
	midiIn.closePort();
	midiIn.removeListener(this);
}

void ofApp::stopPlayback()
{
	metronome.setEnabled(false);
	midiOut << StartMidi() << 0xFC << FinishMidi();  // stop

	for (int i = 0; i < players.size(); i++) {
		players[i]->stop();
		/*players[i]->unload();
		players[i]->disconnect();*/
	}
	//players.clear();
	mixer.setMasterVolume(0);

	m_videoClipSource.closeVideo();
}

void ofApp::loadSong()
{
	for (int i = 0; i < players.size(); i++) {
		players[i]->stop();
		players[i]->unload();
		players[i]->disconnect();
	}
	players.clear();

	midiOut << StartMidi() << 0xFC << FinishMidi(); // stop playback

	// load audio
	ofDirectory dir;
	dir.allowExt("wav");

	float mixerMasterVolume = 0.5;  // TODO config

	string songName = m_setlist[m_currentSongIndex];

	dir.listDir("songs/" + songName + "/audio");

	m_videoClipSource.loadVideo("songs/" + songName + "/clip/clip.mp4");

	m_songEvents.clear();

	ofxXmlSettings settings;
	string filePath = "songs/" + songName + "/structure.xml";
	if (settings.loadFile(filePath)) {
		settings.pushTag("structure");
		settings.pushTag("songparts");
		int numberOfParts = settings.getNumTags("songpart");
		for (int i = 0; i < numberOfParts; i++) {
			settings.pushTag("songpart", i);
			songEvent e;
			e.bpm = settings.getValue("bpm", 0.0);
			e.program = settings.getValue("program", 0);
			e.tick = settings.getValue("tick", 0);
			cout << e.bpm << " " << e.program << " " << e.tick << endl;
			m_songEvents.push_back(e);
			settings.popTag();
		}
	}
	else {
		ofLogError() << "Impossible de charger " + filePath;
		return;
	}

	players.resize(dir.size());

	for (int i = 0; i < dir.size(); i++) {
		cout << dir.getPath(i) << endl;
		players[i] = make_unique<ofxSoundPlayerObject>();
		players[i]->setLoop(true);
		players[i]->load(ofToDataPath(dir.getPath(i)));
	}

	// start playing
	metronome.setNewSong(m_songEvents);
	metronome.sendNextProgramChange();  // envoi du premier pch

}

void ofApp::startPlayback()
{
	// force midi device to go to the first pattern
	ofSleepMillis(2);
	midiOut << StartMidi() << 0xFA << FinishMidi();  // start
	midiOut << StartMidi() << 0xF8 << FinishMidi();  // tick
	midiOut << StartMidi() << 0xFC << FinishMidi();  // stop
	ofSleepMillis(2);

	metronome.setEnabled(true);

	for (int i = 0; i < players.size(); i++) {
		players[i]->connectTo(mixer);
	}

	// chain components
	mixer.connectTo(metronome).connectTo(output);

	for (int i = 0; i < players.size(); i++) {
		players[i]->setVolume(1);
		players[i]->play();
	}

	m_videoClipSource.playVideo();

	mixer.setMasterVolume(1.0); // TODO config

	midiOut << StartMidi() << 0xFA << FinishMidi(); // start playback
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if (m_projectionWindowFocus)  // TODO fonction activation/desactivation
	{
		m_piMapper.keyPressed(key);
	}
	
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
		if (m_currentSongIndex > 0)
		{
			m_currentSongIndex -= 1;
			loadSong();
		}
		break;
	case OF_KEY_DOWN:
		if (m_currentSongIndex < m_setlist.size() - 1)
		{
			m_currentSongIndex += 1;
			loadSong();
		}
		break;
	}
}

//--------------------------------------------------------------
void ofApp::newMidiMessage(ofxMidiMessage& message) {

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
	m_piMapper.keyReleased(key);
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	m_piMapper.mouseDragged(x, y, button);
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	m_piMapper.mousePressed(x, y, button);
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	m_piMapper.mouseReleased(x, y, button);
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
