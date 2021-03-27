#pragma once

#include <vector>

#include "SynchronBlockProcessor.h"
#include "FFT.h"
#include "CColorpalette.h"
#include "PlugInGUISettings.h"

class JadeSpectrogramAudioProcessorEditor;

const struct
{
	const std::string ID = "MinFreq";
	std::string name = "MinFreq";
	std::string unitName = "Hz";
	float minValue = log(1.f);
	float maxValue = log(10000.f);
	float defaultValue = log(1.f);
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

const struct
{
	const std::string ID = "MinColor";
	std::string name = "MinColor";
	std::string unitName = "";
	float minValue = g_minColorVal;
	float maxValue = g_maxColorVal;
	float defaultValue = g_minColorVal;
}paramDisplayMinColor;
const struct
{
	const std::string ID = "MaxColor";
	std::string name = "MaxColor";
	std::string unitName = "";
	float minValue = g_minColorVal;
	float maxValue = g_maxColorVal;
	float defaultValue =g_maxColorVal;
}paramDisplayMaxColor;


class SpectrogramParameter
{
public:
    SpectrogramParameter(){};
	int addParameter(std::vector < std::unique_ptr<RangedAudioParameter>>& paramVector);

public:
    std::atomic<float>* m_DisplayMinFreq;
    float m_DisplayMinFreqOld;
    std::atomic<float>* m_DisplayMaxFreq;
    float m_DisplayMaxFreqOld;
    std::atomic<float>* m_DisplayMinColor;
    float m_DisplayMinColorOld;
    std::atomic<float>* m_DisplayMaxColor;
    float m_DisplayMaxColorOld;
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
    void setPauseMode (bool mode){m_PauseMode = mode;};
    void setWindow (Spectrogram::Windows win){m_windowChoice = win;setWindowFkt();};
    
    size_t getnextpowerof2(float fftsize_ms);

    int getSpectrumSize(){return m_freqsize;};
    int getMemorySize(){return m_memsize_blocks;};
    int getMem(std::vector<std::vector<float >>& mem, int& pos);
    float getSamplerate(){return m_fs;};

private:
    CriticalSection m_protect;
    float m_fs;
    size_t m_channels;
    float m_feed_percent;
    int m_feed_samples;
    int m_feedblocks;
    float m_memsize_s;
    int m_memsize_blocks;
    size_t m_freqsize;
    size_t m_fftsize;
    ChannelMixMode m_mode;
    std::vector<std::vector<float >> m_mem;
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


    bool m_PauseMode;
};

class SpectrogramComponent : public Component, public Timer
{
public:
    SpectrogramComponent(AudioProcessorValueTreeState& vts, Spectrogram& spectrogram, JadeSpectrogramAudioProcessorEditor& editor);
    ~SpectrogramComponent(){stopTimer();};
	void paint(Graphics& g) override;
	void resized() override;
    void setScaleFactor(float newscale){m_scaleFactor = newscale;};
    void timerCallback() override;
    std::function<void()> somethingChanged;    
    void mouseMove (const MouseEvent& event);    
private:
    float m_scaleFactor;
    Spectrogram& m_spectrogram;
    std::vector<std::vector<float >> m_displaymem;
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

    Label m_DisplayMinColorLabel;
    Slider m_DisplayMinColorSlider;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> m_DisplayMinColorAttachment;

    Label m_DisplayMaxColorLabel;
    Slider m_DisplayMaxColorSlider;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> m_DisplayMaxColorAttachment;

    TextButton m_runModeButton;
    TextButton m_pauseButton;
    void pauseClicked();
    void runClicked();
    bool m_isPaused;
    bool m_isRunningDisplay;

    ComboBox m_colorScheme;
    ComboBox m_windowFktCombo;
    
    JadeSpectrogramAudioProcessorEditor& m_editor;
    ComboBox m_fftSizeCombo;
    bool m_hideFFTSizeCombobox;
    void changeFFTSize();
    Label m_FreqLabel;

};
