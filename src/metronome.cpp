#include "metronome.h"

Metronome::Metronome():ofxSoundObject(OFX_SOUND_OBJECT_PROCESSOR) {
    setName("Metronome");

	m_ticksPerBeat = 24;
}

void Metronome::setMidiOut(ofxMidiOut& midiOut) {
	m_midiOut = midiOut;
}

Metronome::~Metronome() {

}

const unsigned int Metronome::getTickCount() const
{
	return m_totalTickCount / m_ticksPerBeat;
}

const unsigned int Metronome::getCurrentSongPartIdx() const
{
	return m_currentSongPartIndex;
}

void Metronome::setCurrentSongPartIdx(unsigned int newSongPartIdx)
{
	m_currentSongPartIndex = newSongPartIdx;
	m_totalTickCount = m_songEvents[m_currentSongPartIndex].tick + 12;
	m_tickCountStartThreshold = m_totalTickCount + 4;
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
	}
	m_totalTickCount = 0;
	m_currentSongPartIndex = 0;
	m_samples = 0;
	m_tickCountStartThreshold = 4;

	for (int i = 0; i < m_songEvents.size(); i++) {
		m_songEvents[i].tick *= m_ticksPerBeat;  // on adapte la valeur au nombre de coups réels transmis par pulsation
		m_songEvents[i].tick -= 12;  // heuristique // TODO la mettre dans la fonction d'update, pas ici !
	}
}

void Metronome::setEnabled(bool enabled) {
	ofLog() << "metronome status enabled: " << enabled;
	m_enabled = enabled;
}

void Metronome::tick() {
	m_midiOut << StartMidi() << 0xF8 << FinishMidi();
}

void Metronome::sendNextProgramChange() {
	ofLog() << "sending PCh " << m_songEvents[m_currentSongPartIndex].program << " to external midi";
	m_midiOut.sendProgramChange(10, m_songEvents[m_currentSongPartIndex].program);
	m_samplesPerTick = (44100 * 60.0f) / m_songEvents[m_currentSongPartIndex].bpm / m_ticksPerBeat;
}

bool Metronome::isSongEnded()
{
	if (!m_enabled)
	{
		return false;
	}
	return m_currentSongPartIndex == (m_songEvents.size() - 1);
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

	for (size_t i = 0; i < output.getNumFrames(); ++i)
	{
		if (++m_samples == m_samplesPerTick)
		{
			m_samples = 0;

			m_totalTickCount += 1;

			/*if (m_totalTickCount % m_ticksPerBeat == 0) {
				ofLog() << "tick #" << int(m_totalTickCount / m_ticksPerBeat) + 1 << ", song part #" << m_currentSongPartIndex << ", song events -> " << m_songEvents.size();
			}*/

			if ((m_currentSongPartIndex < m_songEvents.size() - 1) && (m_totalTickCount == m_songEvents[m_currentSongPartIndex + 1].tick))
			{
				m_currentSongPartIndex += 1;
				sendNextProgramChange();
			}

			if (m_totalTickCount > m_tickCountStartThreshold)
			{
				// heuristique pour retarder le métronome et le caler sur l'audio
				// TODO paramètre xml
				tick();
			}
		}
	}
}