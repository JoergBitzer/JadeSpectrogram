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
	int getRGBColor(float value); 
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
