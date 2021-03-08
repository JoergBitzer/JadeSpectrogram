#include "Spectrogram.h"

#include <cmath>
#include <stdlib.h>    
#include <time.h>    

Spectrogram::Spectrogram()
:m_fs(48000.0),m_channels(2), m_feed_percent (100.0), m_feed_samples(-1), m_memsize_s(20.0),
m_fftsize(1024),m_feedcounter(0),m_inCounter(0),m_feedblocks(1),m_newEntryCounter(0),m_memCounter(0)
{
    m_mode = Spectrogram::ChannelMixMode::AbsMean;
    m_windowChoice = Spectrogram::Windows::Hann;
    buildmem();
}

float g_minValForLogSpectrogram(0.000001f);
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
                    m_powerfinal.at(kk) = 1000000.0;
                    m_powerfinal.at(kk) = m_power[0][kk];
                    
                    break;
                case Spectrogram::ChannelMixMode::Right:
                    m_powerfinal.at(kk) = 1000000.0;
                    if (m_channels>0)
                        m_powerfinal.at(kk) = m_power[1][kk];
                    
                    break;
            }
            m_powerfinal.at(kk) = 20.0*log10(m_powerfinal.at(kk) + g_minValForLogSpectrogram);
        }
        // save into mem
        m_protect.enter();
        //m_mem.push_back(m_powerfinal);
        m_newEntryCounter++;
        //m_mem.pop_front();
        m_mem[m_memCounter] = m_powerfinal;
        m_memCounter++;
        if (m_memCounter == m_memsize_blocks)
            m_memCounter = 0;


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
    m_feed_samples = int(m_feed_percent*0.01*m_fftsize);
    m_memsize_blocks = int(m_memsize_s*m_fs/m_feed_samples + 0.5);
    m_freqsize = m_fftsize/2+1;
    m_mem = {};
    for (auto kk = 0 ; kk< m_memsize_blocks; kk++)
    {
        std::vector<float> in;
        in.resize(m_freqsize);
        std::fill(in.begin(),in.end(),-120.0); // fill with -120 dB = silence
        m_protect.enter();
        m_mem.push_back(in);
        m_protect.exit();
    }
    m_intime.resize(m_fftsize);
    m_powerfinal.resize(m_freqsize);
    m_power.resize(m_channels);
    m_indatamem.resize(m_channels);
    for (size_t cc = 0 ; cc < m_channels ; ++cc)
    {
        m_power[cc].resize(m_freqsize);
        m_indatamem[cc].resize(2*m_fftsize);
        std::fill(m_indatamem[cc].begin(),m_indatamem[cc].end(),0.0);
    }
    m_inCounter = m_fftsize;
    m_newEntryCounter = 0;
}
void Spectrogram::setWindowFkt()
{
    m_window.resize(m_fftsize);

    for (size_t kk = 0; kk < m_fftsize ; kk++)
    {
        switch (m_windowChoice)
        {
            case Spectrogram::Windows::Rect:
                m_window[kk] = 1.f;
            break;
            case Spectrogram::Windows::Hann:
                m_window[kk] = 0.5f*(1.f-cos(2*M_PI*kk/m_fftsize));
            break;
        }
    }
}
int Spectrogram::getMem(std::deque<std::vector<float >>& mem, int& pos)
{
    mem = m_mem;

    int newVals = m_newEntryCounter;
    m_newEntryCounter = 0;
    pos = m_memCounter;
    return newVals;
}

SpectrogramComponent::SpectrogramComponent(Spectrogram& spectrogram)
:m_scaleFactor(1.f),m_spectrogram(spectrogram),
m_internalImg(Image::RGB,1,1,true),m_plottetImg(Image::RGB,1,1,true),m_internalWidth(1),
m_internalHeight(1), m_recomputeAll(true),m_maxColorVal(40.0),m_minColorVal(-120.0)
{
    startTimer(40) ;
    srand (time(NULL));
}
void SpectrogramComponent::paint(Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).darker(0.2));

    int w = getWidth();
    int h = getHeight();

    g.drawImage(m_internalImg,0,0,0.9*w,h,0,0,m_internalWidth,m_internalHeight);

}
void SpectrogramComponent::resized() 
{


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
    }
    std::deque<std::vector<float >> mem;
    int pos;
    int newVals = m_spectrogram.getMem(mem,pos);
    if (newVals > m_internalWidth)
    {
        m_recomputeAll = true;
    }
    CriticalSection crit;
    crit.enter();
    if (m_recomputeAll == true)
    {
        for (size_t ww = 0; ww < m_internalWidth; ++ww)
        {
            for (size_t hh = 0; hh < m_internalHeight; ++hh)
            {
                float val = mem[ww][hh];
                if (val > m_maxColorVal)
                    val = m_maxColorVal;
                if (val < m_minColorVal)
                    val = m_minColorVal;
                
                float ScaledVal = (val - m_minColorVal)/(m_maxColorVal-m_minColorVal);

                // Colormap grey first
                int red = int(ScaledVal*255)<<16;
                int green = int(ScaledVal*255)<<8;
                int blue = int(ScaledVal*255);
                int alpha = 255<<24;
                int color = red + green + blue + alpha;
                m_internalImg.setPixelAt(ww,m_internalHeight-1-hh,juce::Colour(color));

            }
        }
        for (size_t hh = 0; hh < m_internalHeight; ++hh)
        {
            m_internalImg.setPixelAt(pos,m_internalHeight-1-hh,juce::Colours::red);
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
