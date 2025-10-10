#include "ofApp.h"

#include <filesystem>
#include <tuple>
#include <utility>

#include "color.h"
#include "midiUtils.h"
#include "volumesDb.h"
#include "stringUtils.h"

namespace fs = std::filesystem;

namespace {
    bool isMouseInRect(ofRectangle rect, int x, int y)
    {
        if (x >= rect.x && x <= rect.x + rect.width && y >= rect.y && y <= rect.y + rect.height)
        {
            return true;
        }
        return false;
    }
} // unnamed namespace

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
    m_setlistView.setup("Setlist", m_setlist, 0, 0, false, m_colorFocused, m_colorNotFocused);
    changeSelectedUiElement(MAIN_UI_ELEMENT::SETLIST);

	metronome.setSampleRate(m_sampleRate);

	loadSong(); // chargement du premier morceau

	m_quadSurfaces.push_back(QuadSurface());
	loadMappingNodes();

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
	// m_logo = ofImage("TontonMediaPlayerLogo.png");
    
    initializeLayout();
}

void ofApp::initializeLayout()
{
    // clickable areas
    m_areaStop = {20, 430, 12, 12};
    m_areaPlay = {43, 430, 12, 12};
    m_areaPreviousSongPart = {62, 430, 12, 12};
    m_areaNextSongPart = {84, 430, 12, 12};
    m_areaAutoPlay = {725, 430, 12, 12};
    m_setlistArea = {30, 50, 190, 380};
    
    m_areaMixer = {230, 35, 510, 14 * TEXT_LIST_SPACING};
    m_areaSetlist = {30, 50, 190, 25 * TEXT_LIST_SPACING};
    m_areaPatches = {230, 265, 510, 10 * TEXT_LIST_SPACING};
    m_areaPatches.y = m_areaMixer.y + m_areaMixer.height + TEXT_LIST_SPACING;
    m_areaFreeVersionPanel = {0, 0, 760, 60};
    
    m_areaMuteBackings = {698, 37, 38, 15};
    
    // add space for free license panel
    if (m_testVersion)
    {
        unsigned int lostLines = ceil(m_areaFreeVersionPanel.height / TEXT_LIST_SPACING);
        
        unsigned int lostLinesMixer = round(0.6f * lostLines);
        unsigned int lostLinesPatches = lostLines - lostLinesMixer;
        
        m_areaMixer.y += m_areaFreeVersionPanel.height;
        m_areaMixer.height -= lostLinesMixer * TEXT_LIST_SPACING;
        
        m_areaPatches.y = m_areaMixer.y + m_areaMixer.height + TEXT_LIST_SPACING;
        m_areaPatches.height -= lostLinesPatches * TEXT_LIST_SPACING;
        
        m_areaMuteBackings.y += m_areaFreeVersionPanel.height;
        
        m_areaSetlist.y += m_areaFreeVersionPanel.height;
        m_areaSetlist.height -= m_areaFreeVersionPanel.height;
    }
    
    // setlist
    m_setlistView.setCoordinates(m_setlistArea.x, m_areaSetlist.y, m_setlistArea.width, m_areaSetlist.height);
    
    // patches
    m_patchesNbElementsPerPage = m_areaPatches.height / TEXT_LIST_SPACING - 3;
    m_patchesNbElementsPerPage = min<unsigned int>(m_patchesNbElementsPerPage, _midiOuts.size());
    
    // mixer
    m_mixerNbElementsPerPage = m_areaMixer.height / TEXT_LIST_SPACING - 3;
    m_mixerNbElementsPerPage = min<unsigned int>(m_mixerNbElementsPerPage, players.size());
}

void ofApp::setPatchesPageOffset()
{
    if (m_selectedMidiOutput >= m_patchesPageOffset + m_patchesNbElementsPerPage - 2)
    {
        m_patchesPageOffset = min<int>(_midiOuts.size() - m_patchesNbElementsPerPage, m_selectedMidiOutput - m_patchesNbElementsPerPage + 2);
    }
    else if (m_selectedMidiOutput < m_patchesPageOffset + 1)
    {
        m_patchesPageOffset = max<int>(0, m_selectedMidiOutput - 1);
    }
}

void ofApp::setMixerPageOffset()
{
    if (m_selectedVolumeSetting >= m_mixerPageOffset + m_mixerNbElementsPerPage - 2)
    {
        m_mixerPageOffset = min<int>(players.size() - m_mixerNbElementsPerPage, m_selectedVolumeSetting - m_mixerNbElementsPerPage + 2);
    }
    else if (m_selectedVolumeSetting < m_mixerPageOffset + 1)
    {
        m_mixerPageOffset = max<int>(0, m_selectedVolumeSetting - 1);
    }
}

void ofApp::loadHwConfig() {
	ofxXmlSettings settings;
	string filePath = "settings.xml";
	if (settings.load(filePath)) {
		settings.pushTag("settings");
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
        
        if (settings.tagExists("auto_play_delay_seconds"))
        {
            m_autoPlayDelaySeconds = settings.getValue("auto_play_delay_seconds", 0);
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

        m_enableVisuals = settings.getValue("enable_visuals", 1) == 1;
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
            // TODO v�rifications
            shortenString(songName, 20, -1, 0);
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

                string deviceShortenName = deviceOsName;
                // shortenString(deviceShortenName, TEXT_LEN_MIDI_OUTPUT_DEVICE, -1, -1);
                auto midiOut = std::make_shared<MidiOutput>(port, name, i, deviceOsName, deviceShortenName);
                
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
            m_isWarningStateAudioOut = true;
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
        else
        {
            m_isWarningStateAudioOut = true;
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
                if (m_autoPlayDelaySeconds > 0)
                {
                    sleep(m_autoPlayDelaySeconds);
                }
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
                // on dessine un arri�re plan au cas o� il n'y ait pas de vid�o
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
        m_helper = "";
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
    
    std::stringstream strmAudioOut;
    strmAudioOut << "Audio out: " << m_openedAudioDeviceName << " (" << toString(m_openedAudioDeviceApi) << ")";
    int textAudioOutY = 15;
    if (m_testVersion)
    {
        textAudioOutY += m_areaFreeVersionPanel.height;
    }
    unsigned int audioOutStrXOffset = 0;
    if (m_isWarningStateAudioOut)
    {
        drawWarningSign(20, textAudioOutY);
        audioOutStrXOffset = 15;
    }
    ofDrawBitmapString(strmAudioOut.str(), 20 + audioOutStrXOffset, textAudioOutY);
    
//    std::stringstream strmFps;
//    strmFps << round(ofGetFrameRate()) << " fps";
//    ofDrawBitmapString(strmFps.str(), 460, 15);

	ofSetColor(255);
	if (m_setupMappingMode)
	{
		m_fboMapping.draw(0, 0, ofGetWidth(), ofGetHeight());
	}
	else
	{
        drawPatches();
		drawSequencerPage();
        drawLicenseInfo();
	}
	drawHelp();
}

void ofApp::drawLicenseInfo()
{
    if (m_testVersion)
    {
        ofSetColor(m_colorFocused);
        ofDrawRectangle(m_areaFreeVersionPanel.x, m_areaFreeVersionPanel.y + 4, m_areaFreeVersionPanel.width, m_areaFreeVersionPanel.height - 8);
        ofSetColor(240);
        ofDrawBitmapString("Test version - Buy a license to support us and remove this panel", m_areaFreeVersionPanel.x + 20, m_areaFreeVersionPanel.y + m_areaFreeVersionPanel.height / 2);
    }
}

void ofApp::drawHelp()
{
    if (m_helper.size() > 0)
    {
        ofSetColor(128);
        ofDrawBitmapString(m_helper, 10, ofGetHeight() - 5);
    }

}

void ofApp::drawWarningSign(unsigned int x, unsigned int y)
{
    ofSetColor(m_colorWarning);
    ofDrawRectRounded(x, y - 11, 9, 13, 3.0);
    ofSetColor(255);
    ofDrawRectangle(x + 3, y - 10, 3, 6);
    ofDrawRectangle(x + 3, y - 1, 3, 2);
}

void ofApp::drawPatches()
{
    unsigned int baseX = m_areaPatches.x;
    unsigned int baseY = m_areaPatches.y;
    unsigned int width = m_areaPatches.width;
    unsigned int height = m_areaPatches.height;

    ofSetColor(255);
    
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
    border.moveTo(baseX, baseY);
    border.lineTo(baseX + width, baseY);
    border.lineTo(baseX + width, baseY + height);
    border.lineTo(baseX, baseY + height);
    border.lineTo(baseX, baseY);
    border.draw();
    
    if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIDI_OUTPUTS)
    {
        ofSetColor(m_colorFocused);
    }
    else
    {
        ofSetColor(m_colorNotFocused);
    }
    ofDrawRectangle(baseX, baseY, width, 20);

    
    baseX += 10;
    baseY += TEXT_LIST_SPACING;
    
    if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIDI_OUTPUTS)
    {
        ofSetColor(10);
    }
    else
    {
        ofSetColor(255);
    }
    ofDrawBitmapString("Midi outputs", baseX, baseY);
    ofSetColor(255);
    
    unsigned int idxMax = m_patchesPageOffset + m_patchesNbElementsPerPage;
    if (idxMax > _midiOuts.size())
    {
        idxMax = _midiOuts.size();
    }
        
    int offsetY = 15;
    int row = 1;
    int R, G, B;
    for (int i = m_patchesPageOffset; i < idxMax; i++)
    {
        ofSetColor(255);
        
        unsigned int iPage = i - m_patchesPageOffset;
        
        if (iPage == (m_patchesNbElementsPerPage - 1) && m_patchesPageOffset + m_patchesNbElementsPerPage < _midiOuts.size())
        {
            ofSetColor(128);
            ofDrawBitmapString("...", baseX + 0, baseY - 5 + TEXT_LIST_SPACING + TEXT_LIST_SPACING * (m_patchesNbElementsPerPage));
            continue;
        }
        else if (iPage == 0 && m_patchesPageOffset > 0)
        {
            ofSetColor(128);
            ofDrawBitmapString("...", baseX + 0, baseY - 5 + TEXT_LIST_SPACING + TEXT_LIST_SPACING * (0 + 1));
            continue;
        }
        row = iPage;
        
        auto midiOut = _midiOuts[i];
        
        bool isWarning = false;
        if (!midiOut->isOpen())
        {
            isWarning = true;
            drawWarningSign(baseX, baseY + offsetY + (row + 1) * TEXT_LIST_SPACING);

            ofSetColor(128);
        }
        if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIDI_OUTPUTS && m_selectedMidiOutput == midiOut->_deviceIndex)
        {
            ofSetColor(m_colorFocused);
        }
        unsigned int deviceNameXOffset = 0;
        if (isWarning)
        {
            deviceNameXOffset = 15;
            // TODO cut length
        }
        ofDrawBitmapString(midiOut->_deviceName, baseX + deviceNameXOffset, baseY + offsetY + (row + 1) * 15);
        
        if (midiOut->_patchFormat == PatchFormat::PATCH_NAME && !midiOut->_automaticMode)
        {
            string programName = midiOut->getManualPatchName();
            ofSetColor(m_colorFocused);
            if (isWarning)
            {
                ofSetColor(128);
            }
            ofDrawRectRounded(baseX + 100, baseY + offsetY + (row + 1) * 15 - 10, 80, 13, 3.0);
            ofSetColor(0);
            ofDrawBitmapString(programName, baseX + 104, baseY + offsetY + (row + 1) * 15);
        }
        else
        {
            for (auto patch : m_songEvents[metronome.getCurrentSongPartIdx()].patches)
            {
                if (midiOut->_deviceIndex == patch.midiOutputIndex)
                {
                    ofSetColor(m_colorNotFocused);
                    if (isWarning)
                    {
                        ofSetColor(128);
                    }
                    ofDrawRectRounded(baseX + 100, baseY + offsetY + (row + 1) * 15 - 10, 80, 13, 3.0);
                    ofSetColor(0);
                    ofDrawBitmapString(patch.name, baseX + 104, baseY + offsetY + (row + 1) * 15);
                }
            }
        }

        // allow to switch manually between patches
        //if (midiOut->_patchFormat == PatchFormat::PATCH_NAME)
        //{
        //    ofSetColor(128);
        //    string mode = "patch";
        //    if (midiOut->_automaticMode)
        //    {
        //        mode = "auto";
        //    }
        //    ofDrawBitmapString(mode, baseX + 172, baseY + offsetY + row * 15);
        //}
        
        // print os device name
        if (midiOut->_deviceOsName.size() > 0)
        {
            ofSetColor(128);
            ofDrawBitmapString(midiOut->_shortName, baseX + 240, baseY + offsetY + (row + 1) * 15);
        }

        //row += 1;
    }
}

void ofApp::drawMixerLine(int i, int x, int y, int w, int h)
{
    unsigned int iPage = i - m_mixerPageOffset;
    
    if (iPage == (m_mixerNbElementsPerPage - 1) && m_mixerPageOffset + m_mixerNbElementsPerPage < players.size())
    {
        ofSetColor(128);
        ofDrawBitmapString("...", x + 0, y - 5 + TEXT_LIST_SPACING + TEXT_LIST_SPACING * (m_mixerNbElementsPerPage));
        return;
    }
    else if (iPage == 0 && m_mixerPageOffset > 0)
    {
        ofSetColor(128);
        ofDrawBitmapString("...", x + 0, y - 5 + TEXT_LIST_SPACING + TEXT_LIST_SPACING * (0 + 1));
        return;
    }
    
    float volume = mixer.getConnectionVolume(i);
    int lineY = y + TEXT_LIST_SPACING * (iPage + 1);
    ofSetColor(128);
    if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIXER && i == m_selectedVolumeSetting)
    {
        ofSetColor(m_colorFocused);
    }
    ofDrawBitmapString(playersNames[i].second, x + 260, lineY + 15);

    float volumeNorm = volume / VOLUME_MAX;
    int volumeBars = round(volumeNorm * 40);
    int barWidth = 2;
    int barPeriod = 5;
    
    // volume bars placeholders
    ofSetColor(30);
    for (int j = 0; j < 40; j++)
    {
        int xBar = x + j * barPeriod + 2;
        ofDrawRectangle(xBar, lineY + 4, barWidth, TEXT_LIST_SPACING - 4);
    }

    ofSetColor(m_colorNotFocused);
    if (m_muteBackings)
    {
        ofSetColor(100);
    }
    else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIXER && i == m_selectedVolumeSetting)
    {
        ofSetColor(m_colorFocused);
    }
    ofDrawBitmapString(volume, x + 210, lineY + 15);
    for (int j = 0; j < volumeBars; j++)
    {
        int xBar = x + j * barPeriod + 2;
        ofDrawRectangle(xBar, lineY + 4, barWidth, TEXT_LIST_SPACING - 4);
    }
}

void ofApp::drawMixer()
{
    unsigned int baseX = m_areaMixer.x;
    unsigned int baseY = m_areaMixer.y;
    unsigned int width = m_areaMixer.width;
    unsigned int height = m_areaMixer.height;
    
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
    border.moveTo(baseX, baseY);
    border.lineTo(baseX + width, baseY);
    border.lineTo(baseX + width, baseY + height);
    border.lineTo(baseX, baseY + height);
    border.lineTo(baseX, baseY);
    border.draw();
    
    if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIXER)
    {
        ofSetColor(m_colorFocused);
    }
    else
    {
        ofSetColor(m_colorNotFocused);
    }
    ofDrawRectangle(baseX, baseY, width, 20);
    
    baseX += 10;
    baseY += TEXT_LIST_SPACING;
    
    if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIXER)
    {
        ofSetColor(10);
    }
    else
    {
        ofSetColor(255);
    }
    ofDrawBitmapString("Backing tracks", baseX, baseY);
    
    ofSetColor(255);
        
    unsigned int idxMax = m_mixerPageOffset + m_mixerNbElementsPerPage;
    if (idxMax > players.size())
    {
        idxMax = players.size();
    }
    
    for (int i = m_mixerPageOffset; i < idxMax; i++)
    {
        drawMixerLine(i, baseX, baseY, m_areaMixer.width, m_areaMixer.height);
    }
}

void ofApp::drawPlayer()
{
    unsigned int baseY = 430;
    
    unsigned int timelinePosY = 20;
    unsigned int timelineHeight = 38;
    unsigned int timelineWidth = ofGetWidth() - 40;
    unsigned int timelinePosX = (ofGetWidth() - timelineWidth) / 2;
    
    int buttonsYOffset = 0;
    
    // stop button
    if (m_isPlaying)
    {
        ofSetColor(m_colorNotFocused);
    }
    else
    {
        ofSetColor(m_colorFocused);
    }
    ofDrawRectangle(m_areaStop.x, m_areaStop.y + buttonsYOffset, m_areaStop.width, m_areaStop.height);

    // play button
    if (m_isPlaying)
    {
        ofSetColor(m_colorFocused);
    }
    else
    {
        ofSetColor(m_colorNotFocused);
    }
    ofDrawTriangle(
                   m_areaPlay.x, m_areaPlay.y + buttonsYOffset - 1,
                   m_areaPlay.x, m_areaPlay.y + buttonsYOffset + m_areaPlay.height + 1,
                   m_areaPlay.x + m_areaPlay.width, m_areaPlay.y + buttonsYOffset + (int)(0.5f * m_areaPlay.height)
    );
    
    // previous button
    ofSetColor(m_colorNotFocused);
    ofDrawTriangle(
                   m_areaPreviousSongPart.x + m_areaPreviousSongPart.width,
                   m_areaPreviousSongPart.y + buttonsYOffset,
                   m_areaPreviousSongPart.x + m_areaPreviousSongPart.width,
                   m_areaPreviousSongPart.y + buttonsYOffset + m_areaPreviousSongPart.height,
                   m_areaPreviousSongPart.x,
                   m_areaPreviousSongPart.y + buttonsYOffset + (int)(0.5f * m_areaPreviousSongPart.height)
    );
    ofDrawRectangle(
                    m_areaPreviousSongPart.x,
                    m_areaPreviousSongPart.y + buttonsYOffset,
                    (int)(0.25f * m_areaPreviousSongPart.width),
                    m_areaPreviousSongPart.height
    );
    
    // next button
    ofSetColor(m_colorNotFocused);
    ofDrawTriangle(
                   m_areaNextSongPart.x,
                   m_areaNextSongPart.y + buttonsYOffset,
                   m_areaNextSongPart.x,
                   m_areaNextSongPart.y + buttonsYOffset + m_areaNextSongPart.height,
                   m_areaNextSongPart.x + m_areaNextSongPart.width,
                   m_areaNextSongPart.y + buttonsYOffset + (int)(0.5f * m_areaNextSongPart.height)
    );
    ofDrawRectangle(
                    m_areaNextSongPart.x + (int)(0.75f * m_areaNextSongPart.width),
                    m_areaNextSongPart.y + buttonsYOffset,
                    (int)(0.25f * m_areaNextSongPart.width),
                    m_areaNextSongPart.height
    );
    
    // ticks
    ofSetColor(255);
    if (m_isPlaying)
    {
        ofDrawBitmapString(metronome.getTickCount() + 1, 104, baseY + buttonsYOffset + 10);
    }
    
    // mute backings
    ofSetColor(128);
    if (m_muteBackings)
    {
        ofSetColor(m_colorWarning);
        ofDrawRectangle(m_areaMuteBackings.x, m_areaMuteBackings.y, m_areaMuteBackings.width, m_areaMuteBackings.height);
        ofSetColor(255);
    }
    ofDrawBitmapString("mute", m_areaMuteBackings.x + 4, m_areaMuteBackings.y + 10);
    
    // auto play
//    if (m_autoPlayNext)
//    {
//        ofSetColor(200, 180, 30);
//    }
//    else
//    {
//        ofSetColor(50);
//    }
//    unsigned int radius = 7;
//    unsigned int autoPlayX = ofGetWidth() - 25;
//    unsigned int autoPlayY = baseY + radius;
//    ofDrawCircle(autoPlayX, autoPlayY, radius);
//    ofSetColor(0);
//    ofDrawCircle(autoPlayX, autoPlayY, radius - 2);
//    ofDrawRectangle(autoPlayX + radius - 3, autoPlayY, 5, 5);
//    if (m_autoPlayNext)
//    {
//        ofSetColor(200, 180, 30);
//    }
//    else
//    {
//        ofSetColor(50);
//    }
//    ofDrawTriangle(autoPlayX + radius - 4, autoPlayY - 3, autoPlayX + radius + 2, autoPlayY - 3, autoPlayX + radius, autoPlayY + 3);
    if (m_autoPlayNext)
    {
        ofSetColor(200, 180, 30);
    }
    else
    {
        ofSetColor(50);
    }
    unsigned int autoPlayX = ofGetWidth() - 35;
    unsigned int autoPlayY = baseY + 4;
    ofDrawRectangle(autoPlayX, autoPlayY, 8, 4);
    ofDrawTriangle(autoPlayX + 8, autoPlayY - 4, autoPlayX + 8, autoPlayY + 8, autoPlayX + 14, autoPlayY + 2);
    
    
    
    ofSetColor(50);
    // if (m_isPlaying) ofSetColor(m_colorFocused);
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

        ofSetColor(m_songEvents[i].color);
        if (metronome.getCurrentSongPartIdx() == i && m_isPlaying)
        {
            ofSetColor(m_colorFocused);
        }
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

//	ofDrawBitmapString("Auto play (w)", 640, 15);
//	ofSetColor(128);
//	if (m_autoPlayNext)
//	{
//		ofSetColor(50, 255, 25);
//	}
//	ofDrawRectangle(620, 5, 10, 10);

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
    dir.allowExt("mp3");

	string songName = m_setlist[m_currentSongIndex];

	dir.listDir(m_songsRootDir + songName + "/audio");

	m_videoLoaded = false;
	if (ofFile(m_songsRootDir + songName + "/clip/clip.mp4").exists())
	{
		m_videoClipSource.loadVideo(m_songsRootDir + songName + "/clip/clip.mp4");
		m_videoLoaded = true;
	}

	m_songEvents.clear();

	ofxXmlSettings settings;
	string filePath = m_songsRootDir + songName + "/structure.xml";
	if (settings.load(filePath)) {
        vector<PatchEvent> defaultPatches;  // store previous patch values to redund them at every song part
		settings.pushTag("structure");
		settings.pushTag("songparts");
		int numberOfParts = settings.getNumTags("songpart");
        uint32_t nextTick = 0;
        bool tickLenDesc = false;
        bool tickValDesc = false;
        // TODO check if tick mode is consistent
        
		for (int i = 0; i < numberOfParts; i++) {
			settings.pushTag("songpart", i);
			songEvent e;
			e.bpm = settings.getValue("bpm", 120);
			e.programName = settings.getValue("program", "F16");
            e.program = getProgramNumberFromElektronPatternStr(e.programName);
            if (settings.tagExists("tick_len", 0))
            {
                e.tick = nextTick;
                nextTick += settings.getValue("tick_len", 0);
            }
            else if (settings.tagExists("length", 0))
            {
                e.tick = nextTick;
                nextTick += settings.getValue("length", 0);
            }
            else
            {
                e.tick = settings.getValue("tick", 0);
            }
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
                    shortenString(patchEvent.name, TEXT_LEN_PATCH_NAME, -1, 0);
                    
                    e.patches.push_back(patchEvent);
                }
                else if (midiOut->_useLegacyProgram)
                {
                    // store default program value from song (legacy from 1st software versions with only 1 midi output)
                    PatchEvent patchEvent;
                    patchEvent.programNumber = e.program;
                    patchEvent.name = e.programName;
                    patchEvent.midiOutputIndex = midiOut->_deviceIndex;
                    shortenString(patchEvent.name, TEXT_LEN_PATCH_NAME, -1, 0);
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
            
            // set part color
            e.color = m_colorNotFocused;

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
        string shortTrackName = trackName;
        shortenString(shortTrackName, TEXT_LEN_MIXER_ENTRY, 0, 0);
        playersNames.push_back(std::make_pair(trackName, shortTrackName));
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
			storedVolumes.push_back(make_pair(playersNames[i].first, 1.0));
		}
		VolumesDb::getStoredSongVolumes(songName, storedVolumes);

		for (int i = 0; i < storedVolumes.size(); i++)
		{
			string stem = storedVolumes[i].first;
			float volume = storedVolumes[i].second;
			for (int j = 0; j < playersNames.size(); j++)
			{
				if (stem == playersNames[j].first && j < players.size())
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
    
    // initialize layout (update mixer list view)
    initializeLayout();
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
    
    saveAudioMixerVolumes();
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
    
    saveAudioMixerVolumes();
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

void ofApp::saveAudioMixerVolumes()
{
    vector<pair<string, float>> volumes;
    for (int i = 0; i < players.size(); i++)
    {
        float volume = mixer.getConnectionVolume(i);
        string stem = playersNames[i].first;
        volumes.push_back(make_pair(stem, volume));
    }
    VolumesDb::setStoredSongVolumes(m_setlist[m_currentSongIndex], volumes);
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
                    
                    if (m_songSelectorToolIdx != m_currentSongIndex)
                    {
                        m_helper = "Enter: Load song";
                    }
                    else
                    {
                        m_helper = "";
                    }
                }
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIXER)
            {
                m_selectedVolumeSetting = max<int>(0, m_selectedVolumeSetting - 1);
                setMixerPageOffset();
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIDI_OUTPUTS)
            {
                m_selectedMidiOutput = max<int>(0, m_selectedMidiOutput - 1);
                setPatchesPageOffset();
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
            
            if (m_mainUiElementSelected == MAIN_UI_ELEMENT::SETLIST)
            {
                m_helper = "";
            }
            break;
        case 'N':  // next song part
            jumpToNextPart();
            break;
        case 's':
            changeSelectedUiElement(MAIN_UI_ELEMENT::SETLIST);
            break;
        case 'b':
            changeSelectedUiElement(MAIN_UI_ELEMENT::MIXER);
            break;
        case 'm':
            changeSelectedUiElement(MAIN_UI_ELEMENT::MIDI_OUTPUTS);
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
                    
                    if (m_songSelectorToolIdx != m_currentSongIndex)
                    {
                        m_helper = "Enter: Load song";
                    }
                    else
                    {
                        m_helper = "";
                    }
                }
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIXER)
            {
                m_selectedVolumeSetting = min<unsigned int>(players.size() -1, m_selectedVolumeSetting + 1);
                setMixerPageOffset();
            }
            else if (m_mainUiElementSelected == MAIN_UI_ELEMENT::MIDI_OUTPUTS)
            {
                m_selectedMidiOutput = min<unsigned int>(_midiOuts.size() - 1, m_selectedMidiOutput + 1);
                setPatchesPageOffset();
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
        case 'v':
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
//        case 'w':
//            m_autoPlayNext = !m_autoPlayNext;
//            break;
//        case 'x':
//            m_videoResync = !m_videoResync;
//            break;
//        case 's':
//        {
//            // store volumes
//            vector<pair<string, float>> volumes;
//            for (int i = 0; i < players.size(); i++)
//            {
//                float volume = mixer.getConnectionVolume(i);
//                string stem = playersNames[i].first;
//                volumes.push_back(make_pair(stem, volume));
//            }
//            VolumesDb::setStoredSongVolumes(m_setlist[m_currentSongIndex], volumes);
//            break;
//        }
        case 'l':
            m_loop = !m_loop;
            metronome.setLoopMode(m_loop);
            break;
        case 'h':
            m_helper = "f:fullscreen (video), v:video mapping, Q:quit, l:loop (experimental)";
            break;
        case 't':
        {
            m_testVersion = !m_testVersion;
            initializeLayout();
            break;
        }
        case OF_KEY_SHIFT:  // needs to be checked after every upper case letter check
            m_keyShiftPressed = true;
            break;
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
    else
    {
        if (isMouseInRect(m_areaPlay, x, y))
        {
            startPlayback();
        }
        else if (isMouseInRect(m_areaStop, x, y))
        {
            stopPlayback();
            loadSong();
        }
        else if (isMouseInRect(m_areaPreviousSongPart, x, y))
        {
            jumpToPreviousPart();
        }
        else if (isMouseInRect(m_areaNextSongPart, x, y))
        {
            jumpToNextPart();
        }
        else if (isMouseInRect(m_areaAutoPlay, x, y))
        {
            m_autoPlayNext = !m_autoPlayNext;
        }
        else if (isMouseInRect(m_areaMuteBackings, x, y))
        {
            m_muteBackings = !m_muteBackings;
        }
        else if (isMouseInRect(m_areaMixer, x, y))
        {
            changeSelectedUiElement(MAIN_UI_ELEMENT::MIXER);
        }
        else if (isMouseInRect(m_areaSetlist, x, y))
        {
            changeSelectedUiElement(MAIN_UI_ELEMENT::SETLIST);
        }
        else if (isMouseInRect(m_areaPatches, x, y))
        {
            changeSelectedUiElement(MAIN_UI_ELEMENT::MIDI_OUTPUTS);
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
