#include "Spectrogram.h"

#include <cmath>

Spectrogram::Spectrogram()
:m_fs(48000.0),m_channels(2), m_feed_percent (100.0), m_feed_samples(-1), m_memsize_s(20.0),
m_fftsize(1024),m_feedcounter(0),m_inCounter(0),m_feedblocks(1)
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
        m_mem.push(m_powerfinal);
        m_mem.pop();
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
        m_mem.push(in);
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
