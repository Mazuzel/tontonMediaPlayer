#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	// ----------------------------------------
	// midi out (program changes)
	// print the available output ports to the console
	//midiOut.listOutPorts();

	// connect
	midiOut.openPort(1); // by number
	//midiOut.openPort("Elektron Model:Cycles"); // by name
	//midiOut.openVirtualPort("ofxMidiOut"); // open a virtual port



	// ----------------------------------------
	// midi in (clock)
	//midiIn.openPort(0);
	//midiIn.ignoreTypes(false, // sysex  <-- don't ignore timecode messages!
	//	false, // timing <-- don't ignore clock messages!
	//	true); // sensing
	// add ofApp as a listener
	//midiIn.addListener(this);

	// ----------------------------------------
	// sound stream
	soundStream.printDeviceList();

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

	soundStream.setup(settings);
	soundStream.setOutput(output);

	// set metronome controls
	metronome.setMidiOut(midiOut);

	//midiOut.sendProgramChange(10, 95);  // back to sync F16 program
	//midiOut << StartMidi() << 0xFA << FinishMidi();  // start
	//midiOut << StartMidi() << 0xF8 << FinishMidi();  // tick
	//midiOut << StartMidi() << 0xFC << FinishMidi();  // stop
	//midiOut << StartMidi() << 0xFA << FinishMidi();  // start
	//midiOut << StartMidi() << 0xF8 << FinishMidi();  // tick
	//midiOut << StartMidi() << 0xFC << FinishMidi();  // stop


	// Enable or disable audio for video sources globally
	ofx::piMapper::VideoSource::enableAudio = false;
	m_piMapper.registerFboSource(m_videoClipSource);

	m_piMapper.setup();
}

//--------------------------------------------------------------
void ofApp::setupGui() {
	ofLogNotice() << "Setup GUI" << endl;

	ofSetBackgroundColor(0);
	//ofShowCursor();
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
	//std::vector<ofParameterGroup*> pp = m_liveEffectSource.getParameters(); // TODO passer ça dans une fonction updateGui a créer
	//if (pp.size() != m_guiPanels.size() || (m_redrawGui && !m_guiRedrawn))
	//{
	//	if (m_redrawGui)
	//	{
	//		m_guiRedrawn = true;
	//	}

	//	m_guiPanels.clear();
	//	for (int i = 0; i < pp.size(); i++)
	//	{
	//		ofxPanel panel;
	//		m_guiPanels.push_back(panel);
	//		m_guiPanels[i].setup(*pp[i]);
	//	}
	//	m_redrawGui = false;
	//}

	//for (int i = 0; i < pp.size(); i++) {
	//	//m_guiPanels[i].setup(*pp[i]);
	//	m_guiPanels[i].setPosition(220 * i, 0);
	//	m_guiPanels[i].draw();
	//}
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
		float mixerMasterVolume = 1.0;


		if (key == 'f') {
			dir.listDir("songs/fuministe/audio");
			songEvent e0;
			e0.bpm = 150;
			e0.program = 32;
			e0.tick = 0;
			songEvents.push_back(e0);
			songEvent e1;
			e1.bpm = 150;
			e1.program = 33;
			e1.tick = 32;
			songEvents.push_back(e1);

			m_videoClipSource.loadVideo("songs/fuministe/clip/fuministe_live_clip_lq_mute.mp4");
		}
		else {
			dir.listDir("songs/pakela/audio");
			songEvent e0;
			e0.bpm = 150;
			e0.program = 52;
			e0.tick = 0;
			songEvents.push_back(e0);
			songEvent e1;
			e1.bpm = 150;
			e1.program = 53;
			e1.tick = 32;
			songEvents.push_back(e1);
			songEvent e2;
			e2.bpm = 150;
			e2.program = 54;
			e2.tick = 32 + 64 + 16;
			songEvents.push_back(e2);

			mixerMasterVolume = 0.5;

			m_videoClipSource.loadVideo("songs/pakela/clip/pakela_live_mute_lq.mp4");
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
