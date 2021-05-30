#pragma once

#include <vector>
#include "ColormapData.h"

class CColorPalette
{
public: 
	enum 
	{
		kMono = 0,
		kBW,
		kHot,
		kRainbow,
		kViridis,
		kPlasma,
		kJade
	};

	CColorPalette(); // MonoChrome
	CColorPalette(int NrOfColors);
	CColorPalette(int NrOfColors, int ColorScheme=kRainbow);
	~CColorPalette();
	
	// setter
	void setValueRange (float Min, float Max);
	void setNrOfColors (int NrOfColors);
	void setColorSceme (int ColorScheme);
	void setInvertStatus(bool status){m_InvertScheme = status;};

	// Access
	inline int getRGBColor(float value)
	{
	if (value >= m_Max)
		value = m_Max*0.9999f;

	if (value < m_Min)
		value = m_Min;

	int index = int ((value-m_Min) * m_AccessMult);

	if (index < m_NrOfColors)
		return m_Color[index];
	else
		return m_Color[m_NrOfColors-1];

	}; 
	float getValue(int iColor);


protected:
	void ComputeColors(void);
	void AllocateColors(void);
	std::vector<int> m_Color;
	int m_NrOfColors;
	float m_Max;
	float m_Min;
	float m_AccessMult;
	int m_ColorScheme;
	int m_InvertScheme;


};
