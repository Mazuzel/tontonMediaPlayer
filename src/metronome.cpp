#include "metronome.h"

Metronome::Metronome():ofxSoundObject(OFX_SOUND_OBJECT_PROCESSOR) {
    setName("Metronome");

	m_ticksPerBeat = 24;
}

void Metronome::setMidiOuts(std::vector<std::shared_ptr<MidiOutput>>& midiOuts) {
	m_midiOuts = midiOuts;
}

Metronome::~Metronome() {

}

void Metronome::setNbIgnoredStartupsTicks(int nbIgnoredStartupTicks)
{
	if (nbIgnoredStartupTicks > 0)
	{
		m_tickCountStartThreshold = nbIgnoredStartupTicks;
	}
}

void Metronome::setSampleRate(unsigned int sampleRate)
{
    m_sampleRate = sampleRate;
}

const unsigned int Metronome::getTickCount() const
{
	return m_totalTickCount / m_ticksPerBeat;
}

const unsigned int Metronome::getCurrentSongPartIdx() const
{
	return m_currentSongPartIndex;
}

const bool Metronome::loopEndReached() const
{
	return m_loopEndReached;
}

void Metronome::setCurrentSongPartIdx(unsigned int newSongPartIdx)
{
	m_currentSongPartIndex = newSongPartIdx;
	m_totalTickCount = m_songEvents[m_currentSongPartIndex].tick;
	m_tickCountStartThreshold = m_totalTickCount + 4;
	m_loopEndReached = false;
}

double Metronome::getPlaybackPositionMs() const
{
	double msTime = 0.0;
	for (int i = 0; i < m_songEvents.size() - 1; i++)
	{
		if (m_totalTickCount >= m_songEvents[i + 1].tick)
		{
			// we already passed this whole part, we sum it
			unsigned int partTicksCount = m_songEvents[i + 1].tick - m_songEvents[i].tick;
			msTime += 1000.0 * partTicksCount / m_ticksPerBeat / m_songEvents[i].bpm * 60.0;
		}
		else
		{
			// we are in the current part
			msTime += 1000.0 * (m_totalTickCount - m_songEvents[i].tick) / m_ticksPerBeat / m_songEvents[i].bpm * 60.0;
			break;
		}
	}
	return msTime;
}

void Metronome::setNewSong(std::vector<songEvent> songEvents)
{
	m_songEvents.clear();
	for (auto event : songEvents) {
		m_songEvents.push_back(event);
        ofLog() << "event patches: " << event.patches.size();
	}
	m_totalTickCount = 0;
	m_currentSongPartIndex = 0;
	m_samples = 0;
	m_tickCountStartThreshold = 4;

	for (int i = 0; i < m_songEvents.size(); i++) {
		m_songEvents[i].tick *= m_ticksPerBeat;  // on adapte la valeur au nombre de coups réels transmis par pulsation
	}
}

void Metronome::setEnabled(bool enabled) {
	ofLog() << "metronome status enabled: " << enabled;
	m_enabled = enabled;
}

void Metronome::setLoopMode(bool loop)
{
	m_loop = loop;
}

void Metronome::tick() {
	for (auto midiOut: m_midiOuts)
	{
		if (midiOut->sendTicks && midiOut->isOpen())
		{
			midiOut->_midiOut << StartMidi() << 0xF8 << FinishMidi();
		}
	}

}

void Metronome::sendNextProgramChange() {
    for (auto midiOut: m_midiOuts)
    {
        int programNumber = -1;
        if (midiOut->_automaticMode)
        {
            for (auto patch : m_songEvents[m_currentSongPartIndex].patches)
            {
                if (patch.midiOutputIndex == midiOut->_deviceIndex)
                {
                    programNumber = patch.programNumber;
                }
            }
        }
        else
        {
            programNumber = midiOut->getManualPatchProgram();
        }

        if (programNumber >= 0)
        {
            ofLog() << "sending Pch " << programNumber << " to midi device " << midiOut->_deviceOsName << " (" << midiOut->_deviceName << ")";
            // midiOut._midiOut.sendControlChange(10, 0, 1);  // 0 = MSB = playlist (start at 1)  //(int channel, int control, int value);
            // midiOut._midiOut.sendControlChange(10, 32, 2);  // 32 = LSB = song (start at 1)
            midiOut->_midiOut.sendProgramChange(midiOut->defaultChannel, programNumber);
        }

    }

	m_samplesPerTick = round((m_sampleRate * 60.0f) / m_songEvents[m_currentSongPartIndex].bpm / m_ticksPerBeat);
}

bool Metronome::isSongEnded()
{
	if (!m_enabled)
	{
		return false;
	}
	return m_currentSongPartIndex == (m_songEvents.size() - 1);
}

void Metronome::correctTicksToPlaybackPosition(double realPlaybackPositionMs)
{
	// do not check at playback beginning: measurement lacks precison and overcorrects, causing lags
	if ((m_totalTickCount > 8 * m_ticksPerBeat) && m_totalTickCount % (m_ticksPerBeat * 8) == 0)
	{
		double metronomePositionMs = getPlaybackPositionMs();
		double timeLate = realPlaybackPositionMs - metronomePositionMs;
		m_ticksLate = timeLate * m_songEvents[m_currentSongPartIndex].bpm / 60000.0 * m_ticksPerBeat;
		ofLog() << "metronome is late " << timeLate << " ms, " << m_ticksLate << " subticks (running at " << m_ticksPerBeat << ")";
	}
}

void Metronome::process(ofSoundBuffer& input, ofSoundBuffer& output) {

	output = input;

	//if (m_totalTickCount == 0)  // premier coup
	//{
	//	tick();
	//	m_totalTickCount += 1;
	//}

	if (!m_enabled)
	{
		return;
	}

	for (size_t i = 0; i < output.getNumFrames(); i++)
	{
		if (++m_samples == m_samplesPerTick)
		{
			m_samples = 0;

			if ((m_currentSongPartIndex < m_songEvents.size() - 1) && (m_totalTickCount >= m_songEvents[m_currentSongPartIndex + 1].tick - 20))
			{
				m_currentSongPartIndex += 1;
				sendNextProgramChange();
				if (m_loop)
				{
					m_loopEndReached = true;
				}
			}

			if (m_ticksLate >= -1)
			{
				// metronome is accurate with playback or late, we can send a new tick
				m_totalTickCount += 1;
				if (m_totalTickCount > m_tickCountStartThreshold)
				{
					tick();
				}
			}
			else // correct only if two tick of advance measured (error of 1 could be noise and lead to excessive correction)
			{
                ofLog() << "-1 subtick to compensate sequencer in advance (subticks late=" << m_ticksLate << "), samples count=" << m_samples;
				// metronome is running in advance compared to playback
				// so now we skip one tick
				m_ticksLate += 1;
			}

		}
		else if (m_ticksLate > 1 && m_samples == int(m_samplesPerTick / 2))  // correct only if two tick late measured (error of 1 could be noise and lead to excessive correction)
		{
            ofLog() << "+1 subtick to compensate late sequencer (subticks late=" << m_ticksLate << "), samples count=" << m_samples;
			// metronome is running late compared to playback
			// so now we add one tick at half tick time
			m_totalTickCount += 1;
			tick();
			m_ticksLate -= 1;
		}
	}
}
