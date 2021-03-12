
#ifndef _USE_MATH_DEFINES
	#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <stdlib.h>    
#include <time.h>    
#include "JadeLookAndFeel.h"
#include "Spectrogram.h"
#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif
Spectrogram::Spectrogram()
:SynchronBlockProcessor(), m_fs(48000.0),m_channels(2), m_feed_percent (100.0), m_feed_samples(1024), m_memsize_s(1.0),
m_fftsize(1024),m_feedcounter(0),m_inCounter(0),m_feedblocks(1),m_newEntryCounter(0),
m_memCounter(0),m_fft(1024), m_PauseMode(false)
{
    m_mode = Spectrogram::ChannelMixMode::AbsMean;
    m_windowChoice = Spectrogram::Windows::Hann;
    buildmem();
}
void Spectrogram::prepareParameter(std::unique_ptr<AudioProcessorValueTreeState>& vts)
{
    m_SpecParameter.m_DisplayMinFreq = vts->getRawParameterValue(paramDisplayMinFreq.ID);
	m_SpecParameter.m_DisplayMinFreqOld = paramDisplayMinFreq.defaultValue;
    m_SpecParameter.m_DisplayMaxFreq = vts->getRawParameterValue(paramDisplayMaxFreq.ID);
	m_SpecParameter.m_DisplayMaxFreqOld = paramDisplayMaxFreq.defaultValue;
    m_SpecParameter.m_DisplayMinColor = vts->getRawParameterValue(paramDisplayMinColor.ID);
	m_SpecParameter.m_DisplayMinColorOld = paramDisplayMinColor.defaultValue;
    m_SpecParameter.m_DisplayMaxColor = vts->getRawParameterValue(paramDisplayMaxColor.ID);
	m_SpecParameter.m_DisplayMaxColorOld = paramDisplayMaxColor.defaultValue;
}
float g_minValForLogSpectrogram(0.00000000001f);
int Spectrogram::processSynchronBlock(std::vector <std::vector<float>>& data, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    for (size_t kk = 0; kk < m_fftsize ; ++kk) // copy in block to internal mem
    {
        for (size_t cc = 0 ; cc < m_channels ; ++cc)
        {
            m_indatamem[cc][m_inCounter] = data[cc][kk];
        }
        m_inCounter++;
    }

    for (int bb = 0; bb < m_feedblocks; bb++)
    {
        for (size_t cc = 0 ; cc < m_channels ; ++cc)
        {
            for (size_t kk = 0; kk < m_fftsize ; ++kk)
                m_intime[kk] = m_indatamem[cc][kk+m_feed_samples*bb];
        
            // FFT
            computePowerSpectrum(m_intime,m_power[cc]);
        }

        // Hier Auswahl

        // start with a simple solution
        for (size_t kk = 0; kk < m_freqsize ; ++kk)
        {
            switch (m_mode)
            {
                case Spectrogram::ChannelMixMode::AbsMean:
                    m_powerfinal.at(kk) = 0.0;
                    for (size_t cc = 0 ; cc < m_channels ; ++cc)
                    {
                        m_powerfinal.at(kk) += m_power[cc][kk];
                    }
                    m_powerfinal.at(kk) /= m_channels;
                
                    break;
                case Spectrogram::ChannelMixMode::Max:
                    m_powerfinal.at(kk) = 0.0;
                    for (size_t cc = 0 ; cc < m_channels ; ++cc)
                    {
                        if (m_power[cc][kk] > m_powerfinal.at(kk) )
                            m_powerfinal.at(kk) = m_power[cc][kk];
                    }
                    break;
                case Spectrogram::ChannelMixMode::Min:
                    m_powerfinal.at(kk) = 1000000.0;
                    for (size_t cc = 0 ; cc < m_channels ; ++cc)
                    {
                        if (m_power[cc][kk] < m_powerfinal.at(kk) )
                            m_powerfinal.at(kk) = m_power[cc][kk];
                    }
                    break;
                case Spectrogram::ChannelMixMode::Left:
                    m_powerfinal.at(kk) = m_power[0][kk];
                    
                    break;
                case Spectrogram::ChannelMixMode::Right:
                    if (m_channels>0)
                        m_powerfinal.at(kk) = m_power[1][kk];
                    
                    break;
            }
            m_powerfinal.at(kk) = 10.0*log10(m_powerfinal.at(kk) + g_minValForLogSpectrogram);
        }
        // save into mem
        m_protect.enter();
        if (!m_PauseMode)
        {
            m_newEntryCounter++;
            m_mem.at(m_memCounter) = m_powerfinal;
            m_memCounter++;
            if (m_memCounter == m_memsize_blocks)
                m_memCounter = 0;
        }
        m_protect.exit();
    }
    if (m_inCounter == 2*m_fftsize) // copy data to the beginning
    {
        m_inCounter = m_fftsize;
        for (size_t kk = 0; kk < m_fftsize ; ++kk)
        {
            for (size_t cc = 0 ; cc < m_channels ; ++cc)
            {
                m_indatamem[cc][kk] = m_indatamem[cc][kk+m_fftsize];
            }
        }
    }


    return 0;
}

void Spectrogram::computePowerSpectrum(std::vector<float>& in, std::vector<float>& power)
{
    // FFT
    for (size_t kk = 0; kk < m_fftsize ; ++kk)
        in[kk] *= m_window[kk];

    float *invec = in.data();
    m_fft.power(invec, power);  
}

// setter
void Spectrogram::setSamplerate(float samplerate)
{
    m_fs = samplerate;
    buildmem();
}
void Spectrogram::setchannels(size_t newchannels)
{
    m_channels = newchannels;
    buildmem();
}


void Spectrogram::setFFTSize(size_t newFFTSize)
{
    m_fftsize = newFFTSize;
    setDesiredBlockSizeSamples(m_fftsize);
    buildmem();
    setWindowFkt();

}
size_t Spectrogram::getnextpowerof2(float fftsize_ms)
{
    float firstguessFFTSize = (fftsize_ms*0.001*m_fs);
    int nextpowerof2 = int(log(firstguessFFTSize)/log(2.f))+1;
    return size_t(pow(2.f,nextpowerof2));
}
void Spectrogram::setclosestFFTSize_ms(float fftsize_ms)
{
    m_fftsize = getnextpowerof2(fftsize_ms);
    setDesiredBlockSizeSamples(m_fftsize);    
    buildmem();
    setWindowFkt();
}
void Spectrogram::setmemoryTime_s (float memsize_s)
{
    m_memsize_s = memsize_s;
    buildmem();
}
void Spectrogram::setfeed_percent (FeedPercentage feed)
{
    switch (feed)
    {
        case Spectrogram::FeedPercentage::perc100:
            m_feed_percent = 100.0;    
            m_feedblocks = 1;
            break;
        case Spectrogram::FeedPercentage::perc50:
            m_feed_percent = 50.0;    
            m_feedblocks = 2;
            break;
        case Spectrogram::FeedPercentage::perc25:
            m_feed_percent = 25.0;  
            m_feedblocks = 4;  
            break;
        case Spectrogram::FeedPercentage::perc10:
            m_feed_percent = 10.0; 
            m_feedblocks = 10;   
            break;
    }
    buildmem();
}

void Spectrogram::buildmem()
{
    m_fft.setFFTSize(m_fftsize);
    m_feed_samples = int(m_feed_percent*0.01*m_fftsize+0.5);
    m_memsize_blocks = int(m_memsize_s*m_fs/m_feed_samples + 0.5);
    m_freqsize = m_fftsize/2+1;
    m_mem.resize(m_memsize_blocks);
    for (auto kk = 0 ; kk< m_memsize_blocks; kk++)
    {
        m_mem.at(kk).resize(m_freqsize);
        std::fill(m_mem.at(kk).begin(),m_mem.at(kk).end(),-120.0); // fill with -120 dB = silence
    }
    m_intime.resize(m_fftsize);
    m_powerfinal.resize(m_freqsize);
    m_power.resize(m_channels);
    m_indatamem.resize(m_channels);
    for (size_t cc = 0 ; cc < m_channels ; ++cc)
    {
        m_power.at(cc).resize(m_freqsize);
        m_indatamem.at(cc).resize(2*m_fftsize);
        std::fill(m_indatamem.at(cc).begin(),m_indatamem.at(cc).end(),0.0);
    }
    m_inCounter = m_fftsize;
    m_newEntryCounter = 0;
}
void Spectrogram::setWindowFkt()
{
    m_window.resize(m_fftsize);

    for (size_t kk = 0; kk < m_fftsize ; kk++)
    {
        float a0;
        float a1;
        float a2;
        float a3;
        float a4;
        switch (m_windowChoice)
        {
            case Spectrogram::Windows::Rect:
                m_window[kk] = 1.f;
            break;
            case Spectrogram::Windows::Hann:
                m_window[kk] = 0.5f*(1.f-cos(2.0*M_PI*kk/m_fftsize));
                break;
            case Spectrogram::Windows::Hamming:
                m_window[kk] = 25.0/46.0-(1.0-25.0/46.0)*cos(2.0*M_PI*kk/m_fftsize);
                break;
            case Spectrogram::Windows::BlackmanHarris:
                a0 = 0.35875;
                a1 = 0.48829;
                a2 = 0.14128;
                a3 = 0.01168;
                m_window[kk] = a0 - a1*cos(2.0*M_PI*kk/m_fftsize) + a2*cos(4.0*M_PI*kk/m_fftsize) - a3*cos(6.0*M_PI*kk/m_fftsize);
                break;
            case Spectrogram::Windows::FlatTop:
                a0 = 0.21557895;
                a1 = 0.41663158;
                a2 = 0.277263158;
                a3 = 0.083578947;
                a4 = 0.006947368;
                m_window[kk] = a0 - a1*cos(2.0*M_PI*kk/m_fftsize) + a2*cos(4.0*M_PI*kk/m_fftsize) 
                             - a3*cos(6.0*M_PI*kk/m_fftsize) + a4*cos(8.0*M_PI*kk/m_fftsize);
                break;
            case Spectrogram::Windows::HannPoisson:
                float alpha = 2.0;
                m_window[kk] = 0.5f*(1.f-cos(2.0*M_PI*kk/m_fftsize))
                                *exp(-alpha*fabs(m_fftsize-2*kk)/m_fftsize);
                break;
        }
    }
}

int Spectrogram::getMem(std::vector<std::vector<float >>& mem, int& pos)
{
    if (mem.size() != m_mem.size())
        return -1;
    
    for (size_t kk = 0; kk < mem.size(); kk++)
        std::copy(m_mem.at(kk).begin(),m_mem.at(kk).end(),mem.at(kk).begin());

    int newVals = m_newEntryCounter;
    m_newEntryCounter = 0;
    pos = m_memCounter;
    return newVals;
}

SpectrogramComponent::SpectrogramComponent(AudioProcessorValueTreeState& vts, Spectrogram& spectrogram)
:m_scaleFactor(1.f),m_spectrogram(spectrogram),m_vts(vts),
m_internalImg(Image::RGB,1,1,true),m_internalWidth(1),
m_internalHeight(1), m_recomputeAll(true),m_maxColorVal(g_maxColorVal),m_minColorVal(g_minColorVal),
m_colorpalette(256,6),m_maxDisplayFreq(20000.f),m_minDisplayFreq(0.f),somethingChanged(nullptr),
m_isPaused(false),m_isRunning(true)
{
    startTimer(40) ;
    srand (time(NULL));
    m_colorpalette.setValueRange(m_minColorVal,m_maxColorVal);

// UI Elements
	m_DisplayMinFreqLabel.setText("Freq.", NotificationType::dontSendNotification);
	m_DisplayMinFreqLabel.setJustificationType(Justification::centred);
	//m_DisplayMinFreqLabel.attachToComponent (&m_DisplayMinFreqSlider, false);
	//addAndMakeVisible(m_DisplayMinFreqLabel);
	m_DisplayMinFreqSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
	m_DisplayMinFreqAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(m_vts, paramDisplayMinFreq.ID, m_DisplayMinFreqSlider);
	addAndMakeVisible(m_DisplayMinFreqSlider);
	m_DisplayMinFreqSlider.onValueChange = [this]() {if (somethingChanged != nullptr) somethingChanged(); };

	m_DisplayMaxFreqLabel.setText("Freq.", NotificationType::dontSendNotification);
	m_DisplayMaxFreqLabel.setJustificationType(Justification::centred);
	//m_DisplayMaxFreqLabel.attachToComponent (&m_DisplayMaxFreqSlider, false);
	//addAndMakeVisible(m_DisplayMaxFreqLabel);
	m_DisplayMaxFreqSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
	m_DisplayMaxFreqAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(m_vts, paramDisplayMaxFreq.ID, m_DisplayMaxFreqSlider);
	addAndMakeVisible(m_DisplayMaxFreqSlider);
	m_DisplayMaxFreqSlider.onValueChange = [this]() {if (somethingChanged != nullptr) somethingChanged(); };

	m_DisplayMinColorLabel.setText("Color.", NotificationType::dontSendNotification);
	m_DisplayMinColorLabel.setJustificationType(Justification::centred);
	//m_DisplayMinColorLabel.attachToComponent (&m_DisplayMinColorSlider, false);
	//addAndMakeVisible(m_DisplayMinColorLabel);
	m_DisplayMinColorSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
	m_DisplayMinColorAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(m_vts, paramDisplayMinColor.ID, m_DisplayMinColorSlider);
	addAndMakeVisible(m_DisplayMinColorSlider);
	m_DisplayMinColorSlider.onValueChange = [this]() {if (somethingChanged != nullptr) somethingChanged(); };

	m_DisplayMaxColorLabel.setText("Color.", NotificationType::dontSendNotification);
	m_DisplayMaxColorLabel.setJustificationType(Justification::centred);
	//m_DisplayMaxColorLabel.attachToComponent (&m_DisplayMaxColorSlider, false);
	//addAndMakeVisible(m_DisplayMaxColorLabel);
	m_DisplayMaxColorSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
	m_DisplayMaxColorAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(m_vts, paramDisplayMaxColor.ID, m_DisplayMaxColorSlider);
	addAndMakeVisible(m_DisplayMaxColorSlider);
	m_DisplayMaxColorSlider.onValueChange = [this]() {if (somethingChanged != nullptr) somethingChanged(); };

    m_pauseButton.setButtonText("Pause");
    m_pauseButton.setToggleState(false,NotificationType::dontSendNotification);
    m_pauseButton.onClick = [this](){pauseClicked();};
    addAndMakeVisible(m_pauseButton);

    m_runModeButton.setButtonText("Fix");
    m_runModeButton.setToggleState(false,NotificationType::dontSendNotification);
    m_runModeButton.onClick = [this](){runClicked();};
    addAndMakeVisible(m_runModeButton);

    m_colorScheme.addItem("Mono",1);
    m_colorScheme.addItem("BW",2);
    m_colorScheme.addItem("Hot",3);
    m_colorScheme.addItem("Rainbow",4);
    m_colorScheme.addItem("Viridis",5);
    m_colorScheme.addItem("Plasma",6);
    m_colorScheme.addItem("Jade",7);
    m_colorScheme.setSelectedItemIndex(6,NotificationType::dontSendNotification);
    m_colorScheme.setColour(juce::ComboBox::ColourIds::backgroundColourId,JadeTeal);
    m_colorScheme.onChange = [this](){m_colorpalette.setColorSceme(m_colorScheme.getSelectedItemIndex());};
    addAndMakeVisible(m_colorScheme);
    m_windowFktCombo.addItem("Rectangular",1);
    m_windowFktCombo.addItem("Hann",2);
    m_windowFktCombo.addItem("Hamming",3);
    m_windowFktCombo.addItem("BlackmanHarris",4);
    m_windowFktCombo.addItem("FlatTop",5);
    m_windowFktCombo.addItem("HannPoisson",6);
    m_windowFktCombo.setColour(juce::ComboBox::ColourIds::backgroundColourId,JadeTeal);
    m_windowFktCombo.onChange = [this](){m_spectrogram.setWindow(static_cast<Spectrogram::Windows> (m_windowFktCombo.getSelectedItemIndex()));};
    m_windowFktCombo.setSelectedItemIndex(1,NotificationType::dontSendNotification);
    addAndMakeVisible(m_windowFktCombo);




    m_FreqLabel.setText("Analysis",juce::NotificationType::dontSendNotification);
    m_FreqLabel.setJustificationType(juce::Justification::centred);
    m_FreqLabel.setColour(Label::ColourIds::outlineColourId,JadeTeal);
    m_FreqLabel.setColour(Label::ColourIds::textColourId,juce::Colours::white);
    m_FreqLabel.setColour(Label::ColourIds::backgroundColourId,JadeTeal);
    addAndMakeVisible(m_FreqLabel);
}

void SpectrogramComponent::paint(Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).darker(0.2));

    int w = getWidth();
    int h = getHeight();

    float fs = m_spectrogram.getSamplerate();
    
    m_minDisplayFreq = exp(m_DisplayMinFreqSlider.getValue());
    m_maxDisplayFreq = exp(m_DisplayMaxFreqSlider.getValue());

    if (m_minDisplayFreq >= fs*0.5)
        m_minDisplayFreq = 0.9*fs*0.5;
    if (m_maxDisplayFreq >= fs*0.5)
        m_maxDisplayFreq = fs*0.5;

    if (m_minDisplayFreq >= m_maxDisplayFreq)
    {
        m_minDisplayFreq = 0.9*m_maxDisplayFreq;
        m_DisplayMinFreqSlider.setValue(log(m_minDisplayFreq));
    }

    int displayStartPixel = int(2.0*m_minDisplayFreq/fs *m_internalHeight+0.5);
    int displayEndPixel = int(2.0*m_maxDisplayFreq/fs *m_internalHeight + 0.5);
    int heightInterval = int(2.0*m_maxDisplayFreq/fs *m_internalHeight -2.0*m_minDisplayFreq/fs *m_internalHeight+0.5);

    int hStart = m_internalHeight - displayEndPixel;

    int wStartPic = m_scaleFactor*(g_SliderWidth + g_FreqMeter);

    g.drawImage(m_internalImg,wStartPic,0,0.8*w,int(float(h)-m_scaleFactor*g_menuHeight+0.5),
                0,hStart,m_internalWidth,heightInterval);
    
    // Add frequency scale
    int nrOfYTicks = 11;
    float RangePerTick = float(m_maxDisplayFreq - m_minDisplayFreq)/(nrOfYTicks-1);
    int TextHeight = 20;
    g.setFont(0.8*m_scaleFactor*TextHeight);
    for (auto kk = 0; kk < nrOfYTicks; ++kk)
    {
        float newExaktFreq = int(m_minDisplayFreq + RangePerTick*kk + 0.5);
        String OutText;
        if (newExaktFreq >= 1000.f)
        {
            newExaktFreq = int(newExaktFreq*0.01 +0.5)*100;
            OutText += String(newExaktFreq/1000);
            OutText += "k";
        }
        else
        {
            if (newExaktFreq >= 150.f)
            {
                newExaktFreq = int(newExaktFreq*0.1 +0.5)*10;
                OutText += String(newExaktFreq);
            }
            else
            {
                 OutText += String(newExaktFreq);
            }
        }
        
        


        int ydelta = (h-m_scaleFactor*g_menuHeight)* (newExaktFreq-m_minDisplayFreq)/(m_maxDisplayFreq - m_minDisplayFreq);
        int x = wStartPic-g_FreqMeter*m_scaleFactor;
        int y ;
        if (kk < nrOfYTicks-1)
            y = h-m_scaleFactor*g_menuHeight-0.5*TextHeight*m_scaleFactor - ydelta;
        else
        {
            y = h-m_scaleFactor*g_menuHeight - ydelta;
        }
        g.drawText(OutText,x,y,g_FreqMeter*m_scaleFactor,m_scaleFactor*TextHeight,
                juce::Justification::centred,true);
    }

    // Plot Colorbar
    int cbHeight = h-m_scaleFactor*g_menuHeight;
    Image colorbar(Image::RGB,1,cbHeight,true);
    for (int kk = 0; kk < cbHeight; kk++)
    {   
        float val = float(kk)/cbHeight*(g_maxColorVal - g_minColorVal) + g_minColorVal;
        int color = m_colorpalette.getRGBColor(val);
        color = color|0xFF000000; // kein alpha blending
        colorbar.setPixelAt(0,cbHeight-1-kk,juce::Colour(color));
    }
    g.drawImage(colorbar,w-m_scaleFactor*(g_colorbar_width+g_FreqMeter + g_SliderWidth),0,m_scaleFactor*g_colorbar_width,h-m_scaleFactor*g_menuHeight,
                0,0,1,cbHeight);

    // draw scale
    // Add colorbar scale
    nrOfYTicks = 15;
    RangePerTick = float(g_maxColorVal - g_minColorVal)/(nrOfYTicks-1);
    for (auto kk = 0; kk < nrOfYTicks; ++kk)
    {
        float newExaktFreq = int((g_minColorVal + RangePerTick*kk)*0.1 + 0.5)*10;
        String OutText;
        OutText += String(newExaktFreq);

        int ydelta = (h-m_scaleFactor*g_menuHeight)* (newExaktFreq-g_minColorVal)/(g_maxColorVal - g_minColorVal);
        int x = w - (g_FreqMeter + g_SliderWidth + g_SliderMaxFreq_x) *m_scaleFactor;
        int y ;
        if (kk < nrOfYTicks-1)
            y = h-m_scaleFactor*g_menuHeight-0.5*TextHeight*m_scaleFactor - ydelta;
        else
        {
            y = h-m_scaleFactor*g_menuHeight - ydelta;
        }
        
        g.drawText(OutText,x,y,g_FreqMeter*m_scaleFactor,m_scaleFactor*TextHeight,
                juce::Justification::centred,true);
    }


}
void SpectrogramComponent::resized() 
{
    m_DisplayMaxFreqSlider.setBounds(m_scaleFactor*g_SliderMaxFreq_x,m_scaleFactor*g_SliderMaxFreq_y,
            m_scaleFactor*g_SliderWidth,m_scaleFactor*g_SliderHeight);
    m_DisplayMinFreqSlider.setBounds(m_scaleFactor*g_SliderMinFreq_x,m_scaleFactor*g_SliderMinFreq_y,
            m_scaleFactor*g_SliderWidth,m_scaleFactor*g_SliderHeight);

    m_DisplayMaxColorSlider.setBounds(m_scaleFactor*g_SliderMaxColor_x,m_scaleFactor*g_SliderMaxColor_y,
            m_scaleFactor*g_SliderWidth,m_scaleFactor*g_SliderHeight);
    m_DisplayMinColorSlider.setBounds(m_scaleFactor*g_SliderMinColor_x,m_scaleFactor*g_SliderMinColor_y,
            m_scaleFactor*g_SliderWidth,m_scaleFactor*g_SliderHeight);

    m_pauseButton.setBounds(m_scaleFactor*g_PauseButton_x, m_scaleFactor*g_PauseButton_y,
                    m_scaleFactor*g_ButtonWidth,m_scaleFactor*g_ButtonHeight);

    int w = getWidth();
    int x = m_scaleFactor*(g_SliderWidth + g_FreqMeter) + 0.8*w - m_scaleFactor*g_ButtonWidth;

    m_runModeButton.setBounds(x, m_scaleFactor*g_PauseButton_y,
                    m_scaleFactor*g_ButtonWidth,m_scaleFactor*g_ButtonHeight);

    m_colorScheme.setBounds(w-m_scaleFactor*(g_colorbar_width+g_FreqMeter + g_SliderWidth), m_scaleFactor*g_PauseButton_y,
                    m_scaleFactor*g_colorbar_width, m_scaleFactor*g_ButtonHeight);

    x = m_scaleFactor*(g_SliderWidth + g_FreqMeter) + 0.4*w - m_scaleFactor*0.5*100;
    m_FreqLabel.setBounds(x ,m_scaleFactor*g_PauseButton_y,m_scaleFactor*100,m_scaleFactor*g_ButtonHeight );                    

    x = m_scaleFactor*(g_SliderWidth + g_FreqMeter) + 0.2*w - m_scaleFactor*0.5*100;
    m_windowFktCombo.setBounds(x,m_scaleFactor*g_PauseButton_y,m_scaleFactor*100,m_scaleFactor*g_ButtonHeight);

}
void SpectrogramComponent::timerCallback()
{
    int wData = m_spectrogram.getMemorySize();
    int hData = m_spectrogram.getSpectrumSize();
    
    if (wData != m_internalWidth || hData != m_internalHeight)
    {
        m_internalWidth = wData;
        m_internalHeight = hData;
        m_internalImg = m_internalImg.rescaled(m_internalWidth,m_internalHeight);
        m_displaymem.resize(wData);
        for (size_t kk = 0; kk < m_displaymem.size(); ++kk)
        {
            m_displaymem.at(kk).resize(hData);
        }
    }

    int pos;
    int newVals = m_spectrogram.getMem(m_displaymem,pos);
    
    if (newVals > m_internalWidth)
    {
        m_recomputeAll = true;
    }
    float maxValColor = m_DisplayMaxColorSlider.getValue();
    float minValColor = m_DisplayMinColorSlider.getValue();

    m_colorpalette.setValueRange(minValColor,maxValColor);

    CriticalSection crit;
    crit.enter();
    if (m_recomputeAll == true)
    {
        int newwstart = m_internalWidth-pos;
        for (size_t ww = 0; ww < m_internalWidth; ++ww)
        {
            int neww = ww+newwstart;
            if (neww>=m_internalWidth)
                neww -= m_internalWidth;
            for (size_t hh = 0; hh < m_internalHeight; ++hh)
            {
                float val = m_displaymem.at(ww).at(hh);

                int color = m_colorpalette.getRGBColor(val);
                color = color|0xFF000000; // kein alpha blending
                if (m_isRunning)
                {
                    m_internalImg.setPixelAt(neww,m_internalHeight-1-hh,juce::Colour(color));
                }
                else
                {
                    m_internalImg.setPixelAt(ww,m_internalHeight-1-hh,juce::Colour(color));
                }
            }
        }
        if (!m_isRunning)
        {
            for (size_t hh = 0; hh < m_internalHeight; ++hh)
            {
                m_internalImg.setPixelAt(pos,m_internalHeight-1-hh,juce::Colours::red);
            }
        }
    }
    crit.exit();

/*
    m_internalImg.moveImageSection(0,0,1,0,m_internalWidth-1,m_internalHeight);
    //int x = m_internalWidth* float(rand())/RAND_MAX;
    int y = m_internalHeight* float(rand())/RAND_MAX;
    unsigned int color = pow(2,24) * float(rand())/RAND_MAX;
    color = color|0xFF000000;
    m_internalImg.setPixelAt(m_internalWidth-1,y,juce::Colour(color));
 //*/   
    repaint();
}
void SpectrogramComponent::pauseClicked()
{
    m_isPaused = !m_isPaused;
    m_spectrogram.setPauseMode(m_isPaused);
    if (m_isPaused)
    {
        m_pauseButton.setToggleState(true,NotificationType::dontSendNotification);
    }
    else
    {
        m_pauseButton.setToggleState(false,NotificationType::dontSendNotification);
    }
}
void SpectrogramComponent::runClicked()
{
    m_isRunning = !m_isRunning;
    
    if (m_isRunning)
    {
        m_runModeButton.setButtonText("Fix");
        m_runModeButton.setToggleState(false,NotificationType::dontSendNotification);
    }
    else
    {
        m_runModeButton.setButtonText("Run");
        m_runModeButton.setToggleState(true,NotificationType::dontSendNotification);
    }
}

void SpectrogramComponent::mouseMove (const MouseEvent& event)
{
    int x = event.getMouseDownX();
    int y = event.getMouseDownY();

    int w = getWidth();
    int h = getHeight();
    int wstart = m_scaleFactor*(g_FreqMeter+g_SliderMaxFreq_x+g_SliderWidth);
    if (y < h-m_scaleFactor*g_ButtonHeight && x > wstart && x < wstart + 0.8*w)
    {
        float freq = (1.0-float(y)/(float(h)-m_scaleFactor*g_ButtonHeight))*(m_maxDisplayFreq - m_minDisplayFreq)+m_minDisplayFreq;
        MidiMessage msg;
        int midinotenumber =  int(log(freq/440.0)/log(2) * 12 + 69 + 0.5);
        String midiNoteName = msg.getMidiNoteName(midinotenumber,true,true,4);

        m_FreqLabel.setText(String(int(freq+0.5)) + String(" Hz | ") + midiNoteName,juce::NotificationType::dontSendNotification);

    }


}

int SpectrogramParameter::addParameter(std::vector < std::unique_ptr<RangedAudioParameter>>& paramVector)
{
       	paramVector.push_back(std::make_unique<AudioParameterFloat>(paramDisplayMinFreq.ID,
		paramDisplayMinFreq.name,
		NormalisableRange<float>(paramDisplayMinFreq.minValue, paramDisplayMinFreq.maxValue),
		paramDisplayMinFreq.defaultValue,
		paramDisplayMinFreq.unitName,
		AudioProcessorParameter::genericParameter,
		[](float value, int MaxLen) { return (String(0.1*int(exp(value)*10 + 0.5), MaxLen)); },
		[](const String& text) {return text.getFloatValue(); }));

       	paramVector.push_back(std::make_unique<AudioParameterFloat>(paramDisplayMaxFreq.ID,
		paramDisplayMaxFreq.name,
		NormalisableRange<float>(paramDisplayMaxFreq.minValue, paramDisplayMaxFreq.maxValue),
		paramDisplayMaxFreq.defaultValue,
		paramDisplayMaxFreq.unitName,
		AudioProcessorParameter::genericParameter,
		[](float value, int MaxLen) { return (String(0.1*int(exp(value)*10 + 0.5), MaxLen)); },
		[](const String& text) {return text.getFloatValue(); }));

       	paramVector.push_back(std::make_unique<AudioParameterFloat>(paramDisplayMinColor.ID,
		paramDisplayMinColor.name,
		NormalisableRange<float>(paramDisplayMinColor.minValue, paramDisplayMinColor.maxValue),
		paramDisplayMinColor.defaultValue,
		paramDisplayMinColor.unitName,
		AudioProcessorParameter::genericParameter,
		[](float value, int MaxLen) { return (String(1.0*int((value) + 0.5), MaxLen)); },
		[](const String& text) {return text.getFloatValue(); }));

       	paramVector.push_back(std::make_unique<AudioParameterFloat>(paramDisplayMaxColor.ID,
		paramDisplayMaxColor.name,
		NormalisableRange<float>(paramDisplayMaxColor.minValue, paramDisplayMaxColor.maxValue),
		paramDisplayMaxColor.defaultValue,
		paramDisplayMaxColor.unitName,
		AudioProcessorParameter::genericParameter,
		[](float value, int MaxLen) { return (String(1.0*int((value) + 0.5), MaxLen)); },
		[](const String& text) {return text.getFloatValue(); }));
//*/
    return 0;
}
