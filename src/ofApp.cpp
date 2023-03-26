#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	// ----------------------------------------
	// midi out (program changes)
	// print the available output ports to the console
	//midiOut.listOutPorts();
	m_midiOutDevices = midiOut.getOutPortList();
	m_selectedMidiOutDevice.set("midi out", 0, 0, max(0, (int)m_midiOutDevices.size()-1));

	ofLog() << "audio device list";
	soundStream.printDeviceList();
	ofLog() << "audio device list done";
	m_audioDevices = soundStream.getDeviceList(ofSoundDevice::MS_WASAPI);
	m_selectedAudioOutputDevice.set("audio out", 0, 0, max(0, (int)m_audioDevices.size() - 1));

	m_settingsGui.setup();
	m_settingsGui.add(m_selectedMidiOutDevice);
	m_settingsGui.add(m_selectedAudioOutputDevice);
	m_settingsGui.setPosition(600, 100);


	//m_selectedMidiOutDevice.set("midi out", 0, m_midiOutDevices.size(), 1);



	// ----------------------------------------
	// midi in (clock)
	//midiIn.openPort(0);
	//midiIn.ignoreTypes(false, // sysex  <-- don't ignore timecode messages!
	//	false, // timing <-- don't ignore clock messages!
	//	true); // sensing
	// add ofApp as a listener
	//midiIn.addListener(this);

	// ----------------------------------------
	//openMidiOut();
	//openAudioOut();

	// Enable or disable audio for video sources globally
	ofx::piMapper::VideoSource::enableAudio = false;
	m_piMapper.registerFboSource(m_videoClipSource);

	m_piMapper.setup();

	ofDirectory dir;
	dir.listDir("songs");
	for (int i = 0; i < dir.size(); i++) {
		m_setlist.push_back(dir.getName(i));
	}
	//m_setlist = { "pakela", "fuministe" };
}

void ofApp::setupButtonPressed(const void* sender) {
	if (!m_isSetupPageOpened && !m_isPlaying)
	{
		m_isSetupPageOpened = true;
	}
	else if (m_isSetupPageOpened)
	{
		m_isSetupPageOpened = false;
	}
}

void ofApp::validateSettingsButtonPressed(const void* sender)
{
	// ouvrir les ports audio et midi

	m_isSetupPageOpened = false;
}

void ofApp::exitButtonPressed(const void* sender) {
	OF_EXIT_APP(0);
}

void ofApp::midiOutTogglePressed(const void* sender, bool& pressed) {
	if (pressed)
	{
		openMidiOut();
	}
	m_isMidiOutOpened = pressed;
}

void ofApp::audioOutTogglePressed(const void* sender, bool& pressed) {

}

int ofApp::openMidiOut() {
	m_isMidiOutOpened = true; // midiOut.openPort(1); // by number
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

	/*std::vector<ofSoundDevice> devices = soundStream.getMatchingDevices("Elektron Model:Cycles", UINT_MAX, 2, ofSoundDevice::Api::MS_WASAPI);
	settings.setApi(ofSoundDevice::Api::MS_WASAPI);
	settings.setOutDevice(devices[0]);*/

	m_isAudioOutOpened = soundStream.setup(settings);
	if (m_isAudioOutOpened)
	{
		soundStream.setOutput(output);
	}
	
	return 0;
}

//--------------------------------------------------------------
void ofApp::setupGui() {
	ofLogNotice() << "Setup GUI" << endl;

	ofSetBackgroundColor(0);
	//ofShowCursor();

 
	// sound stream
	//m_audioDevices = soundStream.getDeviceList();
	//soundStream.printDeviceList();
	//for (auto device : m_audioDevices)
	//{
	//	ofxToggle deviceToggle;
	//	deviceToggle.setup(device.name, false);
	//	deviceToggle.addListener(this, &ofApp::audioOutTogglePressed);
	//	m_audioDeviceSelectors.push_back(deviceToggle);
	//	ofLog() << device.name;
	//}

	//m_gui.setup();
	//if (m_midiOutDevices.size() > 0)
	//{
	//	m_gui.add(m_fieldSelectedMidiOutDevice.setup("midi out", 1, 0, 3 + m_midiOutDevices.size()));
	//	m_gui.add(m_buttonSetMidiOutDevice.setup("open", false));
	//	m_buttonSetMidiOutDevice.addListener(this, &ofApp::midiOutTogglePressed);
	//}

	//for (auto audioDeviceToggle : m_audioDeviceSelectors)
	//{
	//	m_gui.add(audioDeviceToggle.setup(audioDeviceToggle.getName(), false));
	//}

	/*parameters.setName("params");
	parameters.add(m_selectedMidiOutDevice.set("midi out", 1, 0, 10));
	m_gui.setup(parameters);*/

	/*m_buttonConnect.setup("connect");
	m_buttonConnect.setFillColor(ofColor(255, 10, 20));
	m_buttonConnect.setPosition(200, 10);

	m_midiOutDeviceInputField.setup("midi out", 2);
	m_midiOutDeviceInputField.setPosition(200, 50);*/

	m_buttonSettings.setup("setup")->setPosition(10, 5);
	m_buttonSettings.addListener(this, &ofApp::setupButtonPressed);

	m_buttonExit.setup("exit");
	m_buttonExit.setPosition(740, 5);
	m_buttonValidateSettings.setTextColor(ofColor(200, 10, 10));
	m_buttonExit.addListener(this, &ofApp::exitButtonPressed);

	m_buttonValidateSettings.setup("validate");
	m_buttonValidateSettings.setPosition(300, 10);
	m_buttonValidateSettings.setTextColor(ofColor(10, 200, 10));
	m_buttonValidateSettings.addListener(this, &ofApp::validateSettingsButtonPressed);
}

//--------------------------------------------------------------
void ofApp::update(){
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

	//m_gui.draw();


	//m_midiOutDeviceInputField.draw();
	//m_buttonConnect.draw();

	m_buttonSettings.draw();
	m_buttonExit.draw();


	if (m_isSetupPageOpened)
	{
		drawSetupPage();
	}
	else
	{
		drawSequencerPage();
	}
}

void ofApp::drawSetupPage()
{
	ofDrawBitmapString(("MIDI out devices"), 20, 60);
	for (int i = 0; i < m_midiOutDevices.size(); i++)
	{
		if (i == m_selectedMidiOutDevice.get())
		{
			ofSetColor(255, 100, 100);
		}
		ofDrawBitmapString(to_string(i) + ": " + m_midiOutDevices[i], 20, 80 + 15 * i);
		if (i == m_selectedMidiOutDevice.get())
		{
			ofSetColor(255);
		}
	}

	ofDrawBitmapString(("audio out devices"), 20, 300);
	for (int i = 0; i < m_audioDevices.size(); i++)
	{
		if (i == m_selectedAudioOutputDevice.get())
		{
			ofSetColor(255, 100, 100);
		}
		ofDrawBitmapString(to_string(i) + ": " + m_audioDevices[i].name, 20, 320 + 15 * i);
		if (i == m_selectedAudioOutputDevice.get())
		{
			ofSetColor(255);
		}
	}

	m_settingsGui.draw();

	m_buttonValidateSettings.draw();
}

void ofApp::drawSequencerPage()
{
	ofDrawBitmapString(("-------------Sequencer---------------"), 20, 50);
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


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	m_piMapper.keyPressed(key);

	if (key == 'Q') {
		OF_EXIT_APP(0);
	}

	if (key == 's') // stop
	{
		//std::vector<songEvent> songEvents;
		//songEvent e0;
		//e0.bpm = 120;
		//e0.program = 95;
		//e0.tick = 0;
		//songEvents.push_back(e0);
		//metronome.setNewSong(songEvents);
		//metronome.sendNextProgramChange();  // envoi du premier pch

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

		//midiOut << StartMidi() << 0xFC << FinishMidi();  // stop
		//midiOut.sendProgramChange(10, 95);  // back to sync F16 program
		//midiOut << StartMidi() << 0xFA << FinishMidi();  // start
		//midiOut << StartMidi() << 0xF8 << FinishMidi();  // tick
		//midiOut << StartMidi() << 0xFC << FinishMidi();  // stop
	}

	if (key == 'f' || key == 'p') {
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

		std::vector<songEvent> songEvents;
		float mixerMasterVolume = 0.5;

		string songName = "pakela";

		if (key == 'f') {
			songName = "fuministe";
			mixerMasterVolume = 0.9;
		}


		dir.listDir("songs/" + songName + "/audio");

		m_videoClipSource.loadVideo("songs/" + songName + "/clip/clip.mp4");

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
				songEvents.push_back(e);
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
		metronome.setNewSong(songEvents);
		metronome.sendNextProgramChange();  // envoi du premier pch

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

		mixer.setMasterVolume(mixerMasterVolume);

		midiOut << StartMidi() << 0xFA << FinishMidi(); // start playback
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
