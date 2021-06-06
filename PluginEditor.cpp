
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PlugInGUISettings.h"


//==============================================================================
#if WITH_MIDIKEYBOARD   
JadeSpectrogramAudioProcessorEditor::JadeSpectrogramAudioProcessorEditor (JadeSpectrogramAudioProcessor& p)
    : AudioProcessorEditor (&p), m_processorRef (p), m_presetGUI(p.m_presets),
    	m_keyboard(m_processorRef.m_keyboardState, MidiKeyboardComponent::Orientation::horizontalKeyboard)
#else
JadeSpectrogramAudioProcessorEditor::JadeSpectrogramAudioProcessorEditor (JadeSpectrogramAudioProcessor& p)
    : AudioProcessorEditor (&p), m_processorRef (p), m_presetGUI(p.m_presets)
    ,m_spec(*p.m_parameterVTS, p.m_spectrogram, *this), m_aboutboxvisible(false)
#endif
{

    setResizeLimits (g_minGuiSize_x,g_minGuiSize_x*g_guiratio , g_maxGuiSize_x, g_maxGuiSize_x*g_guiratio);
    setResizable(true,true);
    getConstrainer()->setFixedAspectRatio(1./g_guiratio);
    setSize (g_minGuiSize_x, g_minGuiSize_x*g_guiratio);

	//addAndMakeVisible(m_presetGUI);
#if WITH_MIDIKEYBOARD      
	addAndMakeVisible(m_keyboard);
#endif
    addAndMakeVisible(m_spec);
    m_TitleImage = ImageFileFormat::loadFrom(BinaryData::Title_png, BinaryData::Title_pngSize);
    //m_JadeLogo = ImageFileFormat::loadFrom(BinaryData::LogoJadeHochschule_jpg, BinaryData::LogoJadeHochschule_jpgSize);
    m_JadeLogo = ImageFileFormat::loadFrom(BinaryData::LogoJadeHochschuleTrans_png, BinaryData::LogoJadeHochschuleTrans_pngSize);
    m_AboutBox = ImageFileFormat::loadFrom(BinaryData::AboutBox_png, BinaryData::AboutBox_pngSize);
}

JadeSpectrogramAudioProcessorEditor::~JadeSpectrogramAudioProcessorEditor()
{
}

//==============================================================================
void JadeSpectrogramAudioProcessorEditor::paint (juce::Graphics& g)
{
    int width = getWidth();
    int height = getHeight();
    
    if (m_aboutboxvisible == true)
    {
        m_spec.setVisible(false);
        g.fillAll (Colours::white);
        g.drawImage(m_AboutBox, width/2-m_AboutBox.getWidth()/2,height/2-m_AboutBox.getHeight()/2,
        m_AboutBox.getWidth(), m_AboutBox.getHeight(), 0, 0, m_AboutBox.getWidth(),m_AboutBox.getHeight());

    }
    else
    {
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
        m_spec.setVisible(true);

    }
    // (Our component is opaque, so we must completely fill the background with a solid colour)
	float scaleFactor = float(width)/g_minGuiSize_x;
    g.setColour(getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).darker(0.2));
    g.fillRect(scaleFactor*g_spec_x,scaleFactor*(g_spec_y-30),scaleFactor*g_spec_width,scaleFactor*30);
    g.drawImage(m_TitleImage, scaleFactor*(g_spec_x+60),scaleFactor*(g_spec_y-30),
    scaleFactor*m_TitleImage.getWidth(),scaleFactor*30,0,0,m_TitleImage.getWidth(),m_TitleImage.getHeight());
    int LogoSize = 32;
    float newLogo_x = LogoSize*m_JadeLogo.getWidth()/m_JadeLogo.getHeight();
    g.drawImage(m_JadeLogo, scaleFactor*(g_spec_x+g_spec_width-newLogo_x),scaleFactor*(g_spec_y-30),
    scaleFactor*newLogo_x,scaleFactor*LogoSize,0,0,m_JadeLogo.getWidth(),m_JadeLogo.getHeight());
    

}
const int g_minPresetHandlerHeight(30);
const float g_midikeyboardratio(0.13);
void JadeSpectrogramAudioProcessorEditor::resized()
{
    int height = getHeight();
    // necessary to change fontsize of comboboxes and PopUpmenus
    // 0.5 is a good compromisecould be slightly higher or lower
    m_jadeLAF.setFontSize(0.5*height*g_minPresetHandlerHeight/g_minGuiSize_y);
    // top presethandler
    //m_presetGUI.setBounds(0, 0, getWidth(), height*g_minPresetHandlerHeight/g_minGuiSize_y);
    // bottom a small midkeyboard
#if WITH_MIDIKEYBOARD    
    m_keyboard.setBounds(0, height*(1-g_midikeyboardratio), getWidth(), height*g_midikeyboardratio);
#endif
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    int width = getWidth();
	float scaleFactor = float(width)/g_minGuiSize_x;

    // use setBounds with scaleFactor
    m_spec.setScaleFactor(scaleFactor);
    m_spec.setBounds(scaleFactor*g_spec_x,scaleFactor*g_spec_y,scaleFactor*g_spec_width,scaleFactor*g_spec_height);
}
void JadeSpectrogramAudioProcessorEditor::mouseDown (const MouseEvent& event)
{
    int x = event.getMouseDownX();
    int y = event.getMouseDownY();

    int w = getWidth();
    int h = getHeight();
    float scaleFactor = float(w)/g_minGuiSize_x;
    int LogoSize = 32;
    float newLogo_x = LogoSize*m_JadeLogo.getWidth()/m_JadeLogo.getHeight();

    if (m_aboutboxvisible == false)
    {
        // Is the Logo cicked
        if (x>scaleFactor*(g_spec_x+g_spec_width-newLogo_x) & y > scaleFactor*(g_spec_y-30) &
        x < scaleFactor*(g_spec_x+g_spec_width) & y < scaleFactor*(g_spec_y))
        {
            m_aboutboxvisible = true; 
        }
    }else
    {
        m_aboutboxvisible = false; 
    }
    repaint();
}