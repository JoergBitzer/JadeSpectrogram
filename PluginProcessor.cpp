#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
JadeSpectrogramAudioProcessor::JadeSpectrogramAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
/*   	m_paramVector.push_back(std::make_unique<AudioParameterFloat>(paramDisplayMinFreq.ID,
		paramDisplayMinFreq.name,
		NormalisableRange<float>(paramDisplayMinFreq.minValue, paramDisplayMinFreq.maxValue),
		paramDisplayMinFreq.defaultValue,
		paramDisplayMinFreq.unitName,
		AudioProcessorParameter::genericParameter,
		[](float value, int MaxLen) { return (String(0.1*int(exp(value)*10 + 0.5), MaxLen)); },
		[](const String& text) {return text.getFloatValue(); }));

    m_paramVector.push_back(std::make_unique<AudioParameterFloat>(paramDisplayMaxFreq.ID,
		paramDisplayMaxFreq.name,
		NormalisableRange<float>(paramDisplayMaxFreq.minValue, paramDisplayMaxFreq.maxValue),
		paramDisplayMaxFreq.defaultValue,
		paramDisplayMaxFreq.unitName,
		AudioProcessorParameter::genericParameter,
		[](float value, int MaxLen) { return (String(0.1*int(exp(value)*10 + 0.5), MaxLen)); },
		[](const String& text) {return text.getFloatValue(); }));

    m_paramVector.push_back(std::make_unique<AudioParameterFloat>(paramDisplayMinColor.ID,
		paramDisplayMinColor.name,
		NormalisableRange<float>(paramDisplayMinColor.minValue, paramDisplayMinColor.maxValue),
		paramDisplayMinColor.defaultValue,
		paramDisplayMinColor.unitName,
		AudioProcessorParameter::genericParameter,
		[](float value, int MaxLen) { return (String(1.0*int((value) + 0.5), MaxLen)); },
		[](const String& text) {return text.getFloatValue(); }));

    m_paramVector.push_back(std::make_unique<AudioParameterFloat>(paramDisplayMaxColor.ID,
		paramDisplayMaxColor.name,
		NormalisableRange<float>(paramDisplayMaxColor.minValue, paramDisplayMaxColor.maxValue),
		paramDisplayMaxColor.defaultValue,
		paramDisplayMaxColor.unitName,
		AudioProcessorParameter::genericParameter,
		[](float value, int MaxLen) { return (String(1.0*int((value) + 0.5), MaxLen)); },
		[](const String& text) {return text.getFloatValue(); }));
    //m_paramVector.resize(4);
    //m_paramVector.clear();*/
    m_specParameter.addParameter(m_paramVector);
    m_parameterVTS = std::make_unique<AudioProcessorValueTreeState>(*this, nullptr, Identifier("SpectrogramVTS"),
        AudioProcessorValueTreeState::ParameterLayout(m_paramVector.begin(), m_paramVector.end()));

	m_presets.setAudioValueTreeState(m_parameterVTS.get());
#ifdef FACTORY_PRESETS    
    m_presets.DeployFactoryPresets();
#endif
    // if needed add categories
    // m_presets.addCategory(StringArray("Unknown", "Soft", "Medium", "Hard", "Experimental"));
    // m_presets.addCategory(JadeSynthCategories);
	m_presets.loadfromFileAllUserPresets();    
    m_spectrogram.prepareParameter(m_parameterVTS);
}

JadeSpectrogramAudioProcessor::~JadeSpectrogramAudioProcessor()
{

}

//==============================================================================
const juce::String JadeSpectrogramAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JadeSpectrogramAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool JadeSpectrogramAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool JadeSpectrogramAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double JadeSpectrogramAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int JadeSpectrogramAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int JadeSpectrogramAudioProcessor::getCurrentProgram()
{
    return 0;
}

void JadeSpectrogramAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String JadeSpectrogramAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void JadeSpectrogramAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void JadeSpectrogramAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (samplesPerBlock);
    m_fs = sampleRate;
    m_spectrogram.preparetoProcess(getTotalNumInputChannels(),samplesPerBlock);
    m_spectrogram.setSamplerate(sampleRate);
    m_spectrogram.setmemoryTime_s(10.0);
    m_spectrogram.setFFTSize(2048);
    m_spectrogram.setfeed_percent(Spectrogram::FeedPercentage::perc50);
}

void JadeSpectrogramAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool JadeSpectrogramAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void JadeSpectrogramAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    m_spectrogram.processBlock(buffer, midiMessages);
 
}

//==============================================================================
bool JadeSpectrogramAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* JadeSpectrogramAudioProcessor::createEditor()
{
    return new JadeSpectrogramAudioProcessorEditor (*this);
}

//==============================================================================
void JadeSpectrogramAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
	auto state = m_parameterVTS->copyState();
	std::unique_ptr<XmlElement> xml(state.createXml());
	copyXmlToBinary(*xml, destData);

}

void JadeSpectrogramAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
 	std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

	if (xmlState.get() != nullptr)
		if (xmlState->hasTagName(m_parameterVTS->state.getType()))
			m_parameterVTS->replaceState(ValueTree::fromXml(*xmlState));

}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JadeSpectrogramAudioProcessor();
}
