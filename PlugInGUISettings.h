#pragma once

const int g_minGuiSize_x(800);
const int g_maxGuiSize_x(1400);
const int g_minGuiSize_y(550);
const float g_guiratio = float(g_minGuiSize_y)/g_minGuiSize_x;

const int g_spec_x(5);
const int g_spec_y(35);

const int g_spec_width(790);
const int g_spec_height(510);

// Slider
const int g_FreqMeter(40);
const int g_SliderWidth(20);
const int g_SliderHeight(200);

const int g_menuHeight(20);

// PosFreqSlider
const int g_SliderMinFreq_x(2);
const int g_SliderMinFreq_y(240);

const int g_SliderMaxFreq_x(2);
const int g_SliderMaxFreq_y(10);

const int g_SliderMaxColor_x(g_spec_width - g_SliderWidth-g_SliderMinFreq_x);
const int g_SliderMaxColor_y(g_SliderMaxFreq_y);

const int g_SliderMinColor_x(g_spec_width - g_SliderWidth-g_SliderMinFreq_x);
const int g_SliderMinColor_y(g_SliderMinFreq_y);


const int g_colorbar_width(30);

const float g_maxColorVal(50.0);
const float g_minColorVal(-50.0);

const int g_ButtonHeight(g_menuHeight);
const int g_ButtonWidth(50);
const int g_PauseButton_x(g_FreqMeter+g_SliderWidth+g_SliderMinFreq_x);
const int g_PauseButton_y(g_spec_height-g_menuHeight);

