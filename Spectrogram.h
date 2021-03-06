#pragma once

#include <vector>
#include <queue>

#include "SynchronBlockProcessor.h"
#include "FFT.h"

class Spectrogram : public SynchronBlockProcessor
{
public:
    enum class ChannelMixMode
    {
        AbsMean,
        Max,
        Min,
        Left,
        Right
    };
    Spectrogram();

    // void prepareToPlay();
    virtual int processSynchronBlock(std::vector <std::vector<float>>&, juce::MidiBuffer& midiMessages);

    // setter
    void setSamplerate(float samplerate);
    void setchannels(int newchannels);
    void setFFTSize(int newFFTSize);
    void setclosestFFTSize_ms(float fftsize_ms);
    void setmemoryTime_s (float memsize_s);
    void setfeed_percent (float feed);
    int getnextpowerof2(float fftsize_ms);

private:
    float m_fs;
    int m_channels;
    float m_feed_percent;
    float m_feed_samples;
    float m_memsize_s;
    int m_memsize_blocks;
    int m_freqsize;
    int m_fftsize;
    ChannelMixMode m_mode;
    std::queue<std::vector<float >> m_mem;

    std::vector<float> m_intime;
    std::vector<std::vector<float>> m_power;
    std::vector<float> m_powerfinal;

    spectrum m_fft;


    void buildmem();


};
