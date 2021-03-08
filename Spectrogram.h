#pragma once

#include <vector>
#include <deque>

#include "SynchronBlockProcessor.h"
#include "FFT.h"
#include "CColorpalette.h"

const struct
{
	const std::string ID = "MinFreq";
	std::string name = "MinFreq";
	std::string unitName = "Hz";
	float minValue = log(30.f);
	float maxValue = log(10000.f);
	float defaultValue = log(30.f);
}paramDisplayMinFreq;
const struct
{
	const std::string ID = "MaxFreq";
	std::string name = "MaxFreq";
	std::string unitName = "Hz";
	float minValue = log(500.f);
	float maxValue = log(20000.f);
	float defaultValue = log(20000.f);
}paramDisplayMaxFreq;

class SpectrogramParameter
{
public:
	int addParameter(std::vector < std::unique_ptr<RangedAudioParameter>>& paramVector);

public:
    std::atomic<float>* m_DisplayMinFreq;
    float m_DisplayMinFreqOld;
    std::atomic<float>* m_DisplayMaxFreq;
    float m_DisplayMaxFreqOld;
};




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
    enum class Windows
    {
        Rect,
        Hann,
        Hamming,
        BlackmanHarris,
        FlatTop,
        HannPoisson
    };
    enum class FeedPercentage
    {
        perc100,
        perc50,
        perc25,
        perc10
    };
    Spectrogram();

    // void prepareToPlay();
    virtual int processSynchronBlock(std::vector <std::vector<float>>&, juce::MidiBuffer& midiMessages);
    void prepareParameter(std::unique_ptr<AudioProcessorValueTreeState>& vts);

    // setter
    void setSamplerate(float samplerate);
    void setchannels(size_t newchannels);
    void setFFTSize(size_t newFFTSize);
    void setclosestFFTSize_ms(float fftsize_ms);
    void setmemoryTime_s (float memsize_s);
    void setfeed_percent (FeedPercentage feed);
    size_t getnextpowerof2(float fftsize_ms);

    int getSpectrumSize(){return m_freqsize;};
    int getMemorySize(){return m_memsize_blocks;};
    int getMem(std::deque<std::vector<float >>& mem, int& pos);
    float getSamplerate(){return m_fs;};
private:
    CriticalSection m_protect;
    float m_fs;
    size_t m_channels;
    float m_feed_percent;
    float m_feed_samples;
    int m_feedblocks;
    float m_memsize_s;
    int m_memsize_blocks;
    size_t m_freqsize;
    size_t m_fftsize;
    ChannelMixMode m_mode;
    std::deque<std::vector<float >> m_mem;
    int m_newEntryCounter;
    int m_memCounter;

    std::vector<float> m_intime;
    std::vector<std::vector<float>> m_indatamem;
    int m_feedcounter;
    int m_inCounter;


    std::vector<std::vector<float>> m_power;
    std::vector<float> m_powerfinal;

    spectrum m_fft;
    Windows m_windowChoice;
    std::vector<float> m_window;

    void buildmem();
    void computePowerSpectrum(std::vector<float>& in, std::vector<float>& power);
    void setWindowFkt();
    SpectrogramParameter m_SpecParameter;
};

class SpectrogramComponent : public Component, public Timer
{
public:
    SpectrogramComponent(AudioProcessorValueTreeState& vts, Spectrogram& spectrogram);
    ~SpectrogramComponent(){stopTimer();};
	void paint(Graphics& g) override;
	void resized() override;
    void setScaleFactor(float newscale){m_scaleFactor = newscale;};
    void timerCallback() override;
    std::function<void()> somethingChanged;    
private:
    float m_scaleFactor;
    Spectrogram& m_spectrogram;

    Image m_internalImg;
    Image m_ColorbarImg;
    int m_internalWidth;
    int m_internalHeight;
    bool m_recomputeAll;


    float m_maxColorVal;
    float m_minColorVal;
    CColorPalette m_colorpalette;

    float m_maxDisplayFreq;
    float m_minDisplayFreq;

// Othe UI Elements
    AudioProcessorValueTreeState& m_vts; 

    Label m_DisplayMinFreqLabel;
    Slider m_DisplayMinFreqSlider;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> m_DisplayMinFreqAttachment;

    Label m_DisplayMaxFreqLabel;
    Slider m_DisplayMaxFreqSlider;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> m_DisplayMaxFreqAttachment;
    
 
};
