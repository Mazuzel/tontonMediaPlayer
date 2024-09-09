#include "ofApp.h"

#include <filesystem>
#include <tuple>
#include <utility>

#include "color.h"
#include "midiUtils.h"
#include "volumesDb.h"
#include "stringUtils.h"

namespace fs = std::filesystem;


//--------------------------------------------------------------
void ofApp::setup(){

	ofSetLogLevel(OF_LOG_VERBOSE);
	ofBackground(0);

	loadHwConfig();

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
	metronome.setMidiOuts(_midiOuts);
	metronome.setLoopMode(m_loop);

	openAudioOut();

	// chargement setlist
    loadSetlist();
    m_setlistView.setup(30, 50, 190, 380, "Setlist", m_setlist, 0, 0, false, m_colorFocused, m_colorNotFocused);
    changeSelectedUiElement(MAIN_UI_ELEMENT::SETLIST);

	metronome.setSampleRate(m_sampleRate);

	loadSong(); // chargement du premier morceau

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
    
    ofSetFrameRate(m_audioRefreshRate);

	// load logo
	m_logo = ofImage("TontonMediaPlayerLogo.png");
}

void ofApp::loadHwConfig() {
	ofxXmlSettings settings;
	string filePath = "settings.xml";
	if (settings.load(filePath)) {
		settings.pushTag("settings");
		m_midiInputIdx = settings.getValue("midi_in", 0);
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
			for (int i = ofSoundDevice::ALSA; i < ofSoundDevice::NUM_APIS; i++)
			{
                transform(value.begin(), value.end(), value.begin(), ::tolower);
                string apiStr = toString((ofSoundDevice::Api)i);
                transform(apiStr.begin(), apiStr.end(), apiStr.begin(), ::tolower);
				if (apiStr.find(value) != std::string::npos)
				{
					m_requestedAudioOutApi = (ofSoundDevice::Api)i;
                    ofLog() << "Requested audio out api: " << m_requestedAudioOutApi;
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
        if (settings.tagExists("show_video_preview"))
        {
            m_showVideoPreview = settings.getValue("show_video_preview", 0) == 1;
        }
        m_enableVisuals = settings.getValue("enable_visuals", 0) == 1;
	}
	else {
		ofLogError() << "settings.xml not found, using default hw config";
	}
}

//--------------------------------------------------------------
void ofApp::loadSetlist() {
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
}

int ofApp::openMidiOut() {
    _midiOuts.clear();

    // print midi out devices
    ofLog() << "[Midi out] Port / Device -----------------------------";
    map<int, string> midiOutDevices = getMidiOutDevices();
    for (auto itr = midiOutDevices.begin(); itr != midiOutDevices.end(); ++itr) {
        ofLog() << itr->first << " / " << itr->second;
    }
    ofLog() << "------------------------------------------------------";


    ofLog() << "--------------------Trying to open midi outputs-------------------------";
    ofxXmlSettings settings;
    string filePath = "settings.xml";
    if (settings.load(filePath)) {
        settings.pushTag("settings");
        if (settings.tagExists("midi_outputs"))
        {
            settings.pushTag("midi_outputs");
            int nbMidiOut = settings.getNumTags("output");
            vector<int> attributedDevices;  // list devices already resolved to dispatch properly virtual devices with identical names
            for (int i = 0; i < nbMidiOut; i++) {
                settings.pushTag("output", i);
                string name = settings.getValue("name", to_string(i));
                string deviceId = settings.getValue("device_id", "default");
                
                // resolve port
                int port = -1;
                string deviceOsName = "";
                for (auto itr = midiOutDevices.begin(); itr != midiOutDevices.end(); ++itr) {
                    if (itr->second.find(deviceId) != std::string::npos)
                    {
                        // found
                        unsigned int index = std::distance(midiOutDevices.begin(), itr);
                        if (std::count(attributedDevices.begin(), attributedDevices.end(), index)) {
                            continue;
                        }
                        attributedDevices.push_back(index);
                        port = itr->first;
                        deviceOsName = itr->second;
                        break;
                    }
                }
                if (port < 0) {
                    ofLogError() << "Midi output not found for " << name << ": " << deviceId;
                }
                else
                {
                    ofLog() << "Associating " << name << " with os device " << deviceOsName << " on port " << port;
                }

                auto midiOut = std::make_shared<MidiOutput>(port, name, i, deviceOsName);
                
                // add optional settings
                if (settings.tagExists("send_ticks")) {
                    midiOut->sendTicks = (settings.getValue("send_ticks", 0) == 1);
                }
                if (settings.tagExists("send_timecodes")) {
                    midiOut->sendTimecodes = (settings.getValue("send_timecodes", 0) == 1);
                }
                if (settings.tagExists("channel")) {
                    midiOut->defaultChannel = settings.getValue("channel", 1);
                }
                if (settings.tagExists("use_legacy_program")) {
                    midiOut->_useLegacyProgram = (settings.getValue("use_legacy_program", 0) == 1);
                }
                if (settings.tagExists("input_format")) {
                    string patchFormat = settings.getValue("input_format", "");
                    transform(patchFormat.begin(), patchFormat.end(), patchFormat.begin(), ::tolower);
                    if (patchFormat.find("patch_name") != string::npos)
                    {
                        midiOut->_patchFormat = PatchFormat::PATCH_NAME;
                        
                        // load patches
                        if (settings.tagExists("patches"))
                        {
                            settings.pushTag("patches");
                            
                            int nbPatches = settings.getNumTags("patch");
                            for (int i = 0; i < nbPatches; i++) {
                                settings.pushTag("patch", i);
                                string name = settings.getValue("name", "");
                                unsigned int programNumber = settings.getValue("program", 0);
                                midiOut->_patchesMap.insert({name, programNumber});
                                settings.popTag();  // patch i
                            }
                            
                            settings.popTag();  // patches
                        }
                    }
                    else if (patchFormat.find("elektron_pattern") != string::npos)
                    {
                        midiOut->_patchFormat = PatchFormat::ELEKTRON_PATTERN;
                    }
                }
                
                _midiOuts.push_back(midiOut);

                settings.popTag();
            }
            settings.popTag();
        }
    }
    ofLog() << "--------------------------------------------------------------------------";

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
        std::vector<ofSoundDevice> devices = soundStream.getMatchingDevices(m_requestedAudioOutDevice, UINT_MAX, 2, m_requestedAudioOutApi);
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
            
            // shorten name
            shortenString(m_openedAudioDeviceName, 25, 8, 3);
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
                m_setlistView.setActiveElement(m_currentSongIndex);
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

	if (m_isPlaying && players.size() > 0 && (players[0]->getPositionMS() - m_lastAudioMidiSyncPositionMs) > 1000)
	{
        // do not check at playback beginning: measurement lacks precison and overcorrects, causing lags
        m_lastAudioMidiSyncPositionMs = players[0]->getPositionMS();
		metronome.correctTicksToPlaybackPosition(m_lastAudioMidiSyncPositionMs);
	}

	// VIDEO UPDATE
    if (m_enableVisuals)
    {
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
}

void ofApp::changeSelectedUiElement(MAIN_UI_ELEMENT uiElement)
{
    m_setlistView.setFocus(uiElement == MAIN_UI_ELEMENT::SETLIST);
    m_mainUiElementSelected = uiElement;
    if (uiElement == MAIN_UI_ELEMENT::SETLIST)
    {
        m_helper = "Enter: Load song, >: Play song, <: Stop song, N: Go to next part";
    }
    else if (uiElement == MAIN_UI_ELEMENT::MIXER)
    {
        m_helper = "</>: Change volume";
    }
    else if (uiElement == MAIN_UI_ELEMENT::MIDI_OUTPUTS)
    {
        m_helper = "</>: Change patch manually";
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
    int centerX = 300;
    int centerY = 200;
    int baseWidth = 200;
    int baseHeight = 220;

	ofSetColor(200);
	m_logo.draw(centerX - baseWidth / 2, centerY - baseHeight / 2, baseHeight, baseHeight);

	int xOffset = 0;
	int yOffset = 0;
	ofSetColor(20, 30, 50);
	if (m_currentSongIndex != m_songSelectorToolIdx)
	{
		ofSetColor(180, 90, 25);
		xOffset = static_cast<int>(-3.2 * baseWidth / 220.0);
		yOffset = static_cast<int>( (m_songSelectorToolIdx - 0.4 * m_setlist.size()) * baseHeight / 250.0);
	}
	else if (m_isPlaying)
	{
		ofSetColor(80, 90, 250);
		int songTicks = m_songEvents[m_songEvents.size() - 1].tick;
		float progressOffset = (metronome.getTickCount() - 0.5 * songTicks) / songTicks;
		xOffset = static_cast<int>(progressOffset * 5.2 * baseWidth / 220.0);
        yOffset = static_cast<int>(5 * baseHeight / 250.0);
	}
    ofDrawCircle(centerX - 0.005 * baseWidth + xOffset, centerY - 0.22 * baseHeight + yOffset, 4);
	ofDrawCircle(centerX + 0.125 * baseWidth + xOffset, centerY - 0.22 * baseHeight + yOffset, 4);
	ofSetColor(255);
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofShowCursor();
	ofSetColor(255);

	// draw logo with eyes animation
//	if (!m_setupMappingMode)
//	{
//		drawAnimatedLogo();
//	}

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
    if (m_showVideoPreview)
    {
        m_fboSources[0].draw(20, 260, 300, 200);
    }
    
    std::stringstream strmAudioOut;
    strmAudioOut << "Audio out: " << m_openedAudioDeviceName << " (" << toString(m_openedAudioDeviceApi) << ")";
    ofDrawBitmapString(strmAudioOut.str(), 20, 15);
    
    std::stringstream strmFps;
    strmFps << round(ofGetFrameRate()) << " fps";
    ofDrawBitmapString(strmFps.str(), 460, 15);

	ofSetColor(255);
	if (m_setupMappingMode)
	{
		m_fboMapping.draw(0, 0, ofGetWidth(), ofGetHeight());
	}
	else
	{
        drawPatches();
		drawSequencerPage();
	}
	drawHelp();
}

void ofApp::drawHelp()
{
    if (m_helper.size() > 0)
    {
        ofSetColor(128);
        ofDrawBitmapString(m_helper, 10, ofGetHeight() - 5);
    }

}

void ofApp::drawPatches()
{
    ofSetColor(255);
    unsigned int baseX = 240;
    unsigned int baseY = 280;
    unsigned int width = 510;
    unsigned int height = 150;
    
    ofPath border;
    if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIDI_OUTPUTS)
    {
        border.setStrokeColor(m_colorFocused);
    }
    else
    {
        border.setStrokeColor(m_colorNotFocused);
    }
    border.setStrokeWidth(2);
    border.setFilled(false);
    border.moveTo(baseX - 10, baseY - 15);
    border.lineTo(baseX - 10 + width, baseY - 15);
    border.lineTo(baseX - 10 + width, baseY + height - 15);
    border.lineTo(baseX - 10, baseY + height - 15);
    border.lineTo(baseX - 10, baseY - 15);
    border.draw();
    
    ofDrawBitmapString("Midi outputs", baseX, baseY);
    
    int offsetY = 15;
    int row = 1;
    int R, G, B;
    for (auto midiOut : _midiOuts)
    {
        ofSetColor(255);
        if (!midiOut->isOpen())
        {
            ofSetColor(128);
        }
        if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIDI_OUTPUTS && m_selectedMidiOutput == midiOut->_deviceIndex)
        {
            ofSetColor(m_colorFocused);
        }
        ofDrawBitmapString(midiOut->_deviceName, baseX, baseY + offsetY + row * 15);
        
        if (midiOut->_patchFormat == PatchFormat::PATCH_NAME && !midiOut->_automaticMode)
        {
            string programName = midiOut->getManualPatchName();
            int programNumber = midiOut->getManualPatchProgram();
            float hue = 1313 * programNumber / 128.0 + 100.0 * midiOut->_deviceIndex;
            hue = fmod(hue, 360.0);
            tie(R, G, B) = Tonton::Utils::HSVtoRGB(hue, 50, 96);
            ofSetColor(R, G, B);
            ofDrawRectRounded(baseX + 78, baseY + offsetY + row * 15 - 10, 60, 13, 3.0);
            ofSetColor(0);
            ofDrawBitmapString(programName, baseX + 80, baseY + offsetY + row * 15);
        }
        else
        {
            for (auto patch : m_songEvents[metronome.getCurrentSongPartIdx()].patches)
            {
                if (midiOut->_deviceIndex == patch.midiOutputIndex)
                {
                    ofSetColor(30);
                    if (midiOut->_patchFormat == PatchFormat::PROGRAM_NUMBER)
                    {
                        float hue = 200;
                        hue = fmod(hue, 360.0);
                        tie(R, G, B) = Tonton::Utils::HSVtoRGB(hue, 50, 96);
                        ofSetColor(R, G, B);
                    }
                    else if (midiOut->_patchFormat == PatchFormat::ELEKTRON_PATTERN)
                    {
                        float hue = 500 * patch.programNumber / 128.0;
                        hue = fmod(hue, 360.0);
                        tie(R, G, B) = Tonton::Utils::HSVtoRGB(hue, 50, 96);
                        ofSetColor(R, G, B);
                    }
                    else if (midiOut->_patchFormat == PatchFormat::PATCH_NAME)
                    {
                        float hue = 1313 * patch.programNumber / 128.0 + 100.0 * midiOut->_deviceIndex;
                        hue = fmod(hue, 360.0);
                        tie(R, G, B) = Tonton::Utils::HSVtoRGB(hue, 50, 96);
                        ofSetColor(R, G, B);
                    }
                    ofDrawRectRounded(baseX + 78, baseY + offsetY + row * 15 - 10, 60, 13, 3.0);
                    ofSetColor(0);
                    ofDrawBitmapString(patch.name, baseX + 80, baseY + offsetY + row * 15);
                }
            }
        }

        // allow to switch manually between patches
        if (midiOut->_patchFormat == PatchFormat::PATCH_NAME)
        {
            ofSetColor(128);
            string mode = "patch";
            if (midiOut->_automaticMode)
            {
                mode = "auto";
            }
            ofDrawBitmapString(mode, baseX + 160, baseY + offsetY + row * 15);
        }
        
        // print os device name
        if (midiOut->_deviceOsName.size() > 0)
        {
            ofSetColor(128);
            ofDrawBitmapString(midiOut->_deviceOsName, baseX + 230, baseY + offsetY + row * 15);
        }

        row += 1;
    }
}

void ofApp::drawMixer()
{
    unsigned int baseX = 240;
    unsigned int baseY = 50;
    unsigned int width = 510;
    unsigned int height = 210;
    
    ofPath border;
    if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIXER)
    {
        border.setStrokeColor(m_colorFocused);
    }
    else
    {
        border.setStrokeColor(m_colorNotFocused);
    }
    border.setStrokeWidth(2);
    border.setFilled(false);
    border.moveTo(baseX - 10, baseY - 15);
    border.lineTo(baseX - 10 + width, baseY - 15);
    border.lineTo(baseX - 10 + width, baseY + height - 15);
    border.lineTo(baseX - 10, baseY + height - 15);
    border.lineTo(baseX - 10, baseY - 15);
    border.draw();
    
    ofDrawBitmapString("Mixer", baseX, baseY);
    for (int i = 0; i < players.size(); i++)
    {
        float volume = mixer.getConnectionVolume(i);
        int y = baseY + TEXT_LIST_SPACING * (i + 1);
        ofSetColor(128);
        if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIXER && i == m_selectedVolumeSetting)
        {
            ofSetColor(m_colorFocused);
        }
        ofDrawBitmapString(playersNames[i], baseX + 260, y + 15);
        ofSetColor(255);
        float volumeNorm = volume / VOLUME_MAX;
        int volumeBars = round(volumeNorm * 40);
        int barWidth = 2;
        int barPeriod = 5;
        int R, G, B;
        float hue = 250 - 250 * volumeNorm;
        tie(R, G, B) = Tonton::Utils::HSVtoRGB(hue, 50, 95);
        ofSetColor(R, G, B);
        ofDrawBitmapString(volume, baseX + 210, y + 15);
        for (int j = 0; j < volumeBars; j++)
        {
            int xBar = baseX + j * barPeriod;
            ofDrawRectangle(xBar, y, barWidth, TEXT_LIST_SPACING - 2);
        }
    }
}

void ofApp::drawPlayer()
{
    unsigned int baseY = 440;
    
    unsigned int timelinePosY = 10;
    unsigned int timelineHeight = 38;
    unsigned int timelineWidth = ofGetWidth() - 40;
    unsigned int timelinePosX = (ofGetWidth() - timelineWidth) / 2;
    
    ofDrawBitmapString("Player", 20, baseY);
    ofDrawBitmapString(metronome.getTickCount() + 1, 100, baseY);

    ofSetColor(50);
    if (m_mainUiElementSelected == MAIN_UI_ELEMENT::SETLIST) ofSetColor(194, 155, 83);
    else if (m_isPlaying) ofSetColor(48, 72, 140);
    ofDrawRectangle(timelinePosX, baseY + timelinePosY, timelineWidth, timelineHeight);

    float songTicks = static_cast<float>(m_songEvents[m_songEvents.size() - 1].tick);
    for (int i = 0; i < m_songEvents.size()-1; i++)
    {
        int x = timelinePosX + static_cast<int>((timelineWidth - 4) * m_songEvents[i].tick / songTicks);
        int nbTicks = m_songEvents[i + 1].tick - m_songEvents[i].tick;
        int w = round((timelineWidth - 4) * (m_songEvents[i + 1].tick - m_songEvents[i].tick + 0.5) / songTicks);
        
        if (w + x > timelinePosX + timelineWidth)
        {
            w = timelinePosX + timelineWidth - x;
        }

        float hue = ((2 * m_songEvents[i].program) % 16) * 20.0 + i * 60.0 / m_songEvents.size();
        hue = fmod(hue, 360.0);
        int R, G, B;
        tie(R, G, B) = Tonton::Utils::HSVtoRGB(hue, 50, 95);
        ofSetColor(R, G, B);
        ofDrawRectangle(x, baseY + timelinePosY + 2, w, timelineHeight - 4);
        if (nbTicks >= 8)  // draw part name only if part is big enough
        {
            ofSetColor(0, 0, 0);
            int nameSize = m_songEvents[i].name.size();
            string songPartDisplay = replaceSpacesWithNewline(m_songEvents[i].name);
            auto strSize = estimateStringSize(songPartDisplay);
            float xName = x + 0.5 * w - 0.5 * strSize.x;
            if (xName < x) xName = x;
            float yName = baseY + timelinePosY + 0.5 * timelineHeight - 0.5 * strSize.y + 6;
            if (yName < baseY + timelinePosY) yName = baseY + timelinePosY;
            ofDrawBitmapString(songPartDisplay, round(xName), round(yName));
        }
        
        // draw separation between song parts
        ofSetColor(50);
        ofDrawRectangle(x, baseY + timelinePosY + 2, 1, timelineHeight - 4);
    }
    ofSetColor(255);

    if (m_loop)
    {
        ofSetColor(200, 180, 120);
    }
    ofDrawRectangle(timelinePosX + 2 + (timelineWidth - 4) * metronome.getTickCount() / songTicks - 2, baseY + timelinePosY, 4, timelineHeight);  // player cursor
    ofSetColor(255);
}

void ofApp::drawSequencerPage()
{
	ofSetColor(255);

	ofDrawBitmapString("Auto play (w)", 640, 15);
	ofSetColor(128);
	if (m_autoPlayNext)
	{
		ofSetColor(50, 255, 25);
	}
	ofDrawRectangle(620, 5, 10, 10);

//	ofSetColor(255);
//
//	ofDrawBitmapString("Video resync (x)", 520, 15);
//	ofSetColor(128);
//	if (m_videoResync)
//	{
//		ofSetColor(50, 255, 25);
//	}
//	ofDrawRectangle(500, 5, 10, 10);

	ofSetColor(255);

    drawMixer();

    m_setlistView.draw();

    drawPlayer();
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
	for (auto midiOut: _midiOuts)
	{
		if (midiOut->isOpen())
		{
            if (midiOut->sendTimecodes) // for tonton stage mapper. TODO use standard start & stop messages
            {
                // send stop control message to channel 15
                midiOut->_midiOut.sendProgramChange(15, 2);
            }
            else
            {
                midiOut->_midiOut << StartMidi() << 0xFC << FinishMidi();  // stop
            }
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
    // then stop playback
	m_songSelectorToolIdx = m_currentSongIndex;
    m_setlistView.setSelectedElement(m_songSelectorToolIdx);
	for (int i = 0; i < players.size(); i++) {
		players[i]->stop();
		players[i]->unload();
		players[i]->disconnect();
	}
	players.clear();
	playersNames.clear();

	m_videoClipSource.closeVideo();

	for (auto midiOut: _midiOuts)
	{
		if (midiOut->isOpen())
		{
            if (midiOut->sendTimecodes) // for tonton stage mapper. TODO use standard start & stop messages
            {
                // send stop control message to channel 15
                midiOut->_midiOut.sendProgramChange(15, 2);
                // send program change to other apps via chanel 16
                midiOut->_midiOut.sendProgramChange(16, m_currentSongIndex);
            }
            else
            {
                midiOut->_midiOut << StartMidi() << 0xFC << FinishMidi(); // stop playback
            }
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
        vector<PatchEvent> defaultPatches;  // store previous patch values to redund them at every song part
		settings.pushTag("structure");
		settings.pushTag("songparts");
		int numberOfParts = settings.getNumTags("songpart");
		for (int i = 0; i < numberOfParts; i++) {
			settings.pushTag("songpart", i);
			songEvent e;
			e.bpm = settings.getValue("bpm", 0.0);
			e.programName = settings.getValue("program", "F16");
            e.program = getProgramNumberFromElektronPatternStr(e.programName);
			e.tick = settings.getValue("tick", 0);
			if (settings.tagExists("shader"))
			{
				e.shader = settings.getValue("shader", "");
			}
            e.name = settings.getValue("desc", "");
            
            bool patchesTag = settings.tagExists("patches");
            if (patchesTag) {
                settings.pushTag("patches");
            }

            for (auto midiOut : _midiOuts)
            {
                if (patchesTag && settings.tagExists(midiOut->_deviceName))
                {
                    PatchEvent patchEvent;
                    patchEvent.programNumber = 0;
                    patchEvent.midiOutputIndex = midiOut->_deviceIndex;
                    if (midiOut->_patchFormat == PatchFormat::PROGRAM_NUMBER)
                    {
                        patchEvent.programNumber = settings.getValue(midiOut->_deviceName, 0);
                        patchEvent.name = to_string(patchEvent.programNumber);
                    }
                    else if (midiOut->_patchFormat == PatchFormat::PATCH_NAME)
                    {
                        patchEvent.name = settings.getValue(midiOut->_deviceName, "");
                        if (midiOut->_patchesMap.count(patchEvent.name))
                        {
                            patchEvent.programNumber = midiOut->_patchesMap[patchEvent.name];
                        }
                    }
                    else if (midiOut->_patchFormat == PatchFormat::ELEKTRON_PATTERN)
                    {
                        patchEvent.name = settings.getValue(midiOut->_deviceName, "");
                        patchEvent.programNumber = getProgramNumberFromElektronPatternStr(patchEvent.name);
                    }
                    
                    e.patches.push_back(patchEvent);
                }
                else if (midiOut->_useLegacyProgram)
                {
                    // store default program value from song (legacy from 1st software versions with only 1 midi output)
                    PatchEvent patchEvent;
                    patchEvent.programNumber = e.program;
                    patchEvent.name = e.programName;
                    patchEvent.midiOutputIndex = midiOut->_deviceIndex;
                    e.patches.push_back(patchEvent);
                }
            }

            if (patchesTag) {
                settings.popTag();
            }
            
            // redund patches from previous iteration
            for (auto prevPatch : defaultPatches)
            {
                bool shouldRedund = true;
                for (auto patch : e.patches)
                {
                    if (prevPatch.midiOutputIndex == patch.midiOutputIndex)
                    {
                        shouldRedund = false;
                        break;
                    }
                }
                
                if (shouldRedund)
                {
                    e.patches.push_back(prevPatch);
                }
            }
            
            // reset default patches
            defaultPatches = e.patches;
            
			settings.popTag();
            m_songEvents.push_back(e);
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
	for (auto midiOut: _midiOuts)
	{
		if (midiOut->isOpen())
		{
            if (midiOut->sendTimecodes) // tonton stage mapper midi
            {
                // send start control message to channel 15
                midiOut->_midiOut.sendProgramChange(15, 1);
            }
            else  // standard midi
            {
                midiOut->_midiOut << StartMidi() << 0xFA << FinishMidi();  // start
                midiOut->_midiOut << StartMidi() << 0xF8 << FinishMidi();  // tick
                midiOut->_midiOut << StartMidi() << 0xFC << FinishMidi();  // stop
            }
		}
	}

	ofSleepMillis(2);

	// chain components
	mixer.connectTo(metronome).connectTo(output);

	unsigned int currentSongPartIdx = metronome.getCurrentSongPartIdx();
	double msTime = 0.0;
	for (int i = 1; i <= currentSongPartIdx; i++)
	{
		int ticks = m_songEvents[i].tick - m_songEvents[i - 1].tick;
		msTime += ticks * 1000.0 / m_songEvents[i - 1].bpm * 60.0;
	}
    
    m_lastAudioMidiSyncPositionMs = round(msTime);

	float videoStartTime = (msTime + m_videoStartDelayMs) / 1000.0;  // m_videoStartDelayMs is an offset for latency compensation
	m_videoClipSource.playVideo(videoStartTime);

	for (auto midiOut: _midiOuts)
	{
		if (midiOut->isOpen())
		{
			midiOut->_midiOut << StartMidi() << 0xFA << FinishMidi(); // start playback
		}
	}

    metronome.setEnabled(true);
	for (int i = 0; i < players.size(); i++) {
		players[i]->play();
		if (currentSongPartIdx > 0)
		{
			players[i]->setPositionMS(round(msTime), 0);
		}
		
	}

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
    
    for (auto midiOut : _midiOuts)
    {
        if (midiOut->sendTimecodes) // tonton stage mapper custom midi
        {
            // send tick command to channel 15 and tick value to channel 14
            midiOut->_midiOut.sendProgramChange(15, 3);
            unsigned int tickCount = metronome.getTickCount();
            unsigned short tickCountHb = static_cast<unsigned short>(tickCount / 128);
            unsigned short tickCountLb = tickCount - tickCountHb * 128;
            midiOut->_midiOut.sendProgramChange(14, tickCountLb);
            midiOut->_midiOut.sendProgramChange(14, tickCountHb);
        }
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
    
    for (auto midiOut : _midiOuts)
    {
        if (midiOut->sendTimecodes) // tonton stage mapper custom midi
        {
            // send tick command to channel 15 and tick value to channel 14
            midiOut->_midiOut.sendProgramChange(15, 3);
            unsigned int tickCount = metronome.getTickCount();
            unsigned short tickCountHb = static_cast<unsigned short>(tickCount / 128);
            unsigned short tickCountLb = tickCount - tickCountHb * 128;
            midiOut->_midiOut.sendProgramChange(14, tickCountLb);
            midiOut->_midiOut.sendProgramChange(14, tickCountHb);
        }
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
            if (m_keyShiftPressed)
            {
                changeSelectedUiElement(MAIN_UI_ELEMENT::MIXER);
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::SETLIST)
            {
                startPlayback();
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIXER)
            {
                volumeUp();
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIDI_OUTPUTS)
            {
                if (_midiOuts[m_selectedMidiOutput]->_patchFormat == PatchFormat::PATCH_NAME)
                {
//                    if (_midiOuts[m_selectedMidiOutput]->_automaticMode)
//                    {
//                        _midiOuts[m_selectedMidiOutput]->_automaticMode = false;
//                        _midiOuts[m_selectedMidiOutput]->_manualPatchSelection = 0;
//                    }
//                    else
//                    {
//                        int newManualPatch = _midiOuts[m_selectedMidiOutput]->_manualPatchSelection + 1;
//                        if (newManualPatch >= _midiOuts[m_selectedMidiOutput]->_patchesMap.size())
//                        {
//                            _midiOuts[m_selectedMidiOutput]->_automaticMode = true;
//                            _midiOuts[m_selectedMidiOutput]->_manualPatchSelection = 0;
//                        }
//                        else
//                        {
//                            _midiOuts[m_selectedMidiOutput]->_manualPatchSelection = newManualPatch;
//                            _midiOuts[m_selectedMidiOutput]->_automaticMode = false;
//                        }
//                    }
                    _midiOuts[m_selectedMidiOutput]->incrementManualPatchSelection(1);
                    metronome.sendNextProgramChange();
                }
            }
            break;
        case OF_KEY_LEFT:
            if (m_keyShiftPressed)
            {
                changeSelectedUiElement(MAIN_UI_ELEMENT::SETLIST);
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::SETLIST)
            {
                if (!m_isPlaying && (ofGetElapsedTimef() - m_lastSongReloadTime < 1.0))
                    break;
                stopPlayback();
                loadSong();
                m_lastSongReloadTime = ofGetElapsedTimef();
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIXER)
            {
                volumeDown();
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIDI_OUTPUTS)
            {
                if (_midiOuts[m_selectedMidiOutput]->_patchFormat == PatchFormat::PATCH_NAME)
                {
//                    if (_midiOuts[m_selectedMidiOutput]->_automaticMode)
//                    {
//                        _midiOuts[m_selectedMidiOutput]->_automaticMode = false;
//                        _midiOuts[m_selectedMidiOutput]->_manualPatchSelection = _midiOuts[m_selectedMidiOutput]->_patchesMap.size() - 1;
//                    }
//                    else
//                    {
//                        int newManualPatch = _midiOuts[m_selectedMidiOutput]->_manualPatchSelection - 1;
//                        if (newManualPatch < 0)
//                        {
//                            _midiOuts[m_selectedMidiOutput]->_automaticMode = true;
//                            _midiOuts[m_selectedMidiOutput]->_manualPatchSelection = 0;
//                        }
//                        else
//                        {
//                            _midiOuts[m_selectedMidiOutput]->_manualPatchSelection = newManualPatch;
//                            _midiOuts[m_selectedMidiOutput]->_automaticMode = false;
//                        }
//                    }
                    _midiOuts[m_selectedMidiOutput]->incrementManualPatchSelection(-1);
                    metronome.sendNextProgramChange();
                }
            }
            break;
        case OF_KEY_UP:
            if (m_keyShiftPressed && m_mainUiElementSelected == MAIN_UI_ELEMENT::MIDI_OUTPUTS)
            {
                changeSelectedUiElement(MAIN_UI_ELEMENT::MIXER);
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::SETLIST)
            {
                if (m_songSelectorToolIdx > 0){
                    m_songSelectorToolIdx -= 1;
                    m_setlistView.setSelectedElement(m_songSelectorToolIdx);
                }
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIXER)
            {
                m_selectedVolumeSetting = (m_selectedVolumeSetting - 1) % players.size();
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIDI_OUTPUTS)
            {
                m_selectedMidiOutput = (m_selectedMidiOutput - 1) % _midiOuts.size();
            }
            break;
        case OF_KEY_RETURN:
            if (ofGetElapsedTimef() - m_lastSongChangeTime < 0.1)
                break;
            stopPlayback();
            m_currentSongIndex = m_songSelectorToolIdx;
            m_setlistView.setActiveElement(m_currentSongIndex);
            loadSong();
            m_lastSongChangeTime = ofGetElapsedTimef();
            break;
        case 'N':  // next song part
            jumpToNextPart();
            break;
        case OF_KEY_DOWN:
            if (m_keyShiftPressed && m_mainUiElementSelected == MAIN_UI_ELEMENT::MIXER)
            {
                changeSelectedUiElement(MAIN_UI_ELEMENT::MIDI_OUTPUTS);
                break;
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::SETLIST)
            {
                if (m_songSelectorToolIdx < m_setlist.size() - 1) {
                    m_songSelectorToolIdx += 1;
                    m_setlistView.setSelectedElement(m_songSelectorToolIdx);
                }
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIXER)
            {
                m_selectedVolumeSetting = (m_selectedVolumeSetting + 1) % players.size();
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIDI_OUTPUTS)
            {
                m_selectedMidiOutput = (m_selectedMidiOutput + 1) % _midiOuts.size();
            }
            break;
        case 'p':
            if (ofGetElapsedTimef() - m_lastSongChangeTime < 0.1)
                break;
            stopPlayback();
            if (m_currentSongIndex < m_setlist.size() - 1)
            {
                m_currentSongIndex += 1;
                m_setlistView.setActiveElement(m_currentSongIndex);
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
        case 'w':
            m_autoPlayNext = !m_autoPlayNext;
            break;
//        case 'x':
//            m_videoResync = !m_videoResync;
//            break;
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
        case 'h':
            m_helper = "f:fullscreen, m:mapping, s:store vols, Q:quit, p:launch next, l:loop";
            break;
        case OF_KEY_SHIFT:  // needs to be checked after every upper case letter check
            m_keyShiftPressed = true;
    }
}

//--------------------------------------------------------------
void ofApp::newMidiMessage(ofxMidiMessage& message) {

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    switch (key) {
        case OF_KEY_SHIFT:
            m_keyShiftPressed = false;
            break;
    }
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
