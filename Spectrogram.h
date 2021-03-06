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
    void setchannels(size_t newchannels);
    void setFFTSize(size_t newFFTSize);
    void setclosestFFTSize_ms(float fftsize_ms);
    void setmemoryTime_s (float memsize_s);
    void setfeed_percent (float feed);
    size_t getnextpowerof2(float fftsize_ms);

private:
    float m_fs;
    size_t m_channels;
    float m_feed_percent;
    float m_feed_samples;
    float m_memsize_s;
    int m_memsize_blocks;
    size_t m_freqsize;
    size_t m_fftsize;
    ChannelMixMode m_mode;
    std::queue<std::vector<float >> m_mem;

    std::vector<float> m_intime;
    std::vector<std::vector<float>> m_power;
    std::vector<float> m_powerfinal;

    spectrum m_fft;


    void buildmem();


};
