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

void Metronome::setNewSong(std::vector<songEvent> songEvents)
{
	m_songEvents.clear();
	for (auto event : songEvents) {
		m_songEvents.push_back(event);
	}
	m_totalTickCount = 0;
	m_currentSongPartIndex = 0;
	m_samples = 0;

	for (int i = 0; i < m_songEvents.size(); i++) {
		m_songEvents[i].tick *= m_ticksPerBeat;  // on adapte la valeur au nombre de coups r�els transmis par pulsation
		m_songEvents[i].tick -= 12;  // heuristique
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

			if (m_totalTickCount % m_ticksPerBeat == 0) {
				ofLog() << "tick #" << int(m_totalTickCount / m_ticksPerBeat) + 1 << ", song part #" << m_currentSongPartIndex << ", song events -> " << m_songEvents.size();
			}

			if ((m_currentSongPartIndex < m_songEvents.size() - 1) && (m_totalTickCount == m_songEvents[m_currentSongPartIndex + 1].tick))
			{
				m_currentSongPartIndex += 1;
				sendNextProgramChange();
			}

			if (m_totalTickCount > 4) {
				tick();
			}
		}
	}
}