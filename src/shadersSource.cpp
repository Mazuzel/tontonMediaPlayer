#include "shadersSource.h"
#include "metronome.h"

void ShadersSource::setup(std::vector<songEvent> songEvents)
{
    m_currentSongPartIndex = 0;
    m_shaders.clear();
    m_events.clear();

    for (auto event : songEvents) {
        shaderEvent e;
        e.tick = event.tick;
        e.bpm = event.bpm;
        if (m_shaders.find(event.shader) != m_shaders.end()) {
            // shader already loaded
            e.shader = event.shader;
        }
        else
        {
            // try to load it
            try
            {
                ofShader shader;
                bool success = shader.load("shaders/default.vert", "shaders/" + event.shader + ".frag");
                if (success)
                {
                    m_shaders.insert({ event.shader, shader });
                    e.shader = event.shader;
                }
            }
            catch (const std::exception& e) {
                ofLog() << "could not load shader: " << event.shader << e.what();
            }
        }
        m_events.push_back(e);
    }
}

void ShadersSource::draw(int targetWidth, int targetHeight, int ticks, float time)
{
    while (m_currentSongPartIndex < m_events.size() && ticks >= m_events[m_currentSongPartIndex + 1].tick)
    {
        m_currentSongPartIndex += 1;
    }

    if (m_events[m_currentSongPartIndex].shader.length() > 0)
    {
        ofSetColor(255);
        m_shaders[m_events[m_currentSongPartIndex].shader].begin();
        m_shaders[m_events[m_currentSongPartIndex].shader].setUniform1f("time", time);
        m_shaders[m_events[m_currentSongPartIndex].shader].setUniform1f("bpm", m_events[m_currentSongPartIndex].bpm);
        m_shaders[m_events[m_currentSongPartIndex].shader].setUniform2f("resolution", targetWidth, targetHeight);
        ofDrawRectangle(0, 0, targetWidth, targetHeight);
        m_shaders[m_events[m_currentSongPartIndex].shader].end();
    }
}