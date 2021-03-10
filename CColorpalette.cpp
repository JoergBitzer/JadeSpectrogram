#include "CColorpalette.h"

CColorPalette::CColorPalette() // MonoChrome
:m_NrOfColors(2),
m_ColorScheme(kMono),
m_Color(0),
m_InvertScheme(0)
{
	m_Min = 0.f;
	m_Max = 1.f;
	m_AccessMult = float(m_NrOfColors)/(m_Max-m_Min);
	AllocateColors();
}
CColorPalette::CColorPalette(int NrOfColors)
:m_NrOfColors(NrOfColors),
m_ColorScheme(kMono),
m_Color(0),
m_InvertScheme(0)
{
	m_Min = 0.f;
	m_Max = 1.f;
	m_AccessMult = float(m_NrOfColors)/(m_Max-m_Min);
	AllocateColors();
}
CColorPalette::CColorPalette(int NrOfColors, int ColorScheme)
:m_NrOfColors(NrOfColors),
m_ColorScheme(ColorScheme),
m_Color(0),
m_InvertScheme(0)
{
	m_Min = 0.f;
	m_Max = 1.f; 
	m_AccessMult = float(m_NrOfColors)/(m_Max-m_Min);
	AllocateColors();
}
CColorPalette::~CColorPalette()
{

}
	
	// Methoden
void CColorPalette::setValueRange (float Min, float Max)
{
	if (Max >= Min)
	{
		m_Min = Min;
		m_Max = Max;
	}else
	{
		m_Min = Max;
		m_Max = Min;
	}
	if (m_Max == m_Min)
		m_Min = 0.99*m_Max;

	m_AccessMult = float(m_NrOfColors)/(m_Max-m_Min);
}
void CColorPalette::setNrOfColors (int NrOfColors)
{
	m_NrOfColors = NrOfColors;

	m_AccessMult = float(m_NrOfColors)/(m_Max-m_Min);
	AllocateColors();
}
void CColorPalette::setColorSceme (int ColorScheme)
{
	m_ColorScheme = ColorScheme;
	ComputeColors();
}

	// Datenzugriff
int CColorPalette::getRGBColor(float value) // Zugriff �ber Wert im Bereich Min Max mit �berwachung und S�ttigung
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

}
float CColorPalette::getValue(int iColor)
{
	int kk;
	float value;
	for (kk = 0 ;kk < m_NrOfColors ;kk++)
	{
		if (m_Color[kk] == iColor)
		{
			value = float(kk)/m_AccessMult + m_Min;
			return value;
		}
	}
	return 100000000000000000000000000000.f;
}
void CColorPalette::AllocateColors(void)
{
	m_Color.resize(m_NrOfColors);
	ComputeColors();

}
void CColorPalette::ComputeColors(void)
{
	int kk;
		int Half = m_NrOfColors/2;
	switch (m_ColorScheme)
	{
	case kMono:


		for (kk = 0; kk < m_NrOfColors ; kk++)
		{
			if (kk <= Half) // Black
				m_Color[kk] = 0;
			else // White
			{
				int iRed = 255<<16;
				int iGreen = 255<<8; 			
				int iBlue = 255;
				int iColor = iRed|iGreen|iBlue;

			if (m_InvertScheme)
				m_Color[m_NrOfColors-kk-1] = iColor;
			else
				m_Color[kk] = iColor;
			}
		}
		break;

	case kBW:
		for (kk = 0; kk < m_NrOfColors ; kk++)
		{
			int iRed = int(255.f * float(kk)/m_NrOfColors) <<16;
			int iGreen = int(255.f * float(kk)/m_NrOfColors)<<8; 			
			int iBlue = int(255.f * float(kk)/m_NrOfColors);
			int iColor = iRed|iGreen|iBlue;

			if (m_InvertScheme)
				m_Color[m_NrOfColors-kk-1] = iColor;
			else
				m_Color[kk] = iColor;
		}
		break;
	case kRainbow:
		for (kk = 0; kk < m_NrOfColors ; kk++)
		{
			int iRed;
			int iGreen ; 			
			int iBlue ;
			float SlopeDef = 4.f/float(m_NrOfColors);
			if (kk < m_NrOfColors/8)
			{
				iBlue = int(255.f * (float(kk)*SlopeDef + 0.5));
				iGreen = 0; 			
				iRed = 0;
			}
			else if (kk < 2*m_NrOfColors/8)
			{
				iBlue= 255;
				iGreen = int(255.f * float(kk-m_NrOfColors/8)*SlopeDef) <<8; 			
				iRed = 0;
	
			}
			else if (kk < 3*m_NrOfColors/8)
			{
				iBlue = 255;
				iGreen = int(255.f * float(kk-m_NrOfColors/8)*SlopeDef) <<8;
				iRed = 0;
				 			
			}
			else if (kk < 4*m_NrOfColors/8)
			{
				iBlue = int(255.f * float(1.f - float(kk-3*m_NrOfColors/8)*SlopeDef));
				iGreen = 255 <<8; 			
				iRed = int(255.f * float(kk-3*m_NrOfColors/8)*SlopeDef)<<16;
			}
			else if (kk < 5*m_NrOfColors/8)
			{
				iBlue = int(255.f * float(1.f - float(kk-3*m_NrOfColors/8)*SlopeDef));
				iGreen = 255 <<8; 			
				iRed = int(255.f * float(kk-3*m_NrOfColors/8)*SlopeDef)<<16;
			}
			else if (kk < 6*m_NrOfColors/8)
			{
				iBlue = 0;
				iGreen = int(255.f * float(1.f - float(kk-5*m_NrOfColors/8)*SlopeDef)) <<8; 			
				iRed = 255<<16;

			}
			else if (kk < 7*m_NrOfColors/8)
			{
				iBlue = 0;
				iGreen = int(255.f * float(1.f - float(kk-5*m_NrOfColors/8)*SlopeDef)) <<8; 			
				iRed = 255<<16;

			}
			else
			{
				iBlue = 0;
				iGreen = 0; 			
				iRed = int(255.f * float(1.f - float(kk-7*m_NrOfColors/8)*SlopeDef))<<16;

			}
			int iColor = iRed|iGreen|iBlue;

			if (m_InvertScheme)
				m_Color[m_NrOfColors-kk-1] = iColor;
			else
				m_Color[kk] = iColor;
		}
		break;
	case kHot:
		for (kk = 0; kk < m_NrOfColors ; kk++)
		{
			int iRed;
			int iGreen ; 			
			int iBlue ;
			float SlopeDef = 8.f/float(3*m_NrOfColors);
			float SlopeDef2 = 8.f/float(2*m_NrOfColors);
			if (kk < 3*m_NrOfColors/8)
			{
				iBlue = 0;
				iGreen = 0; 			
				iRed = int(255.f * (float(kk)*SlopeDef))<<16;
			}
			else if (kk < 6*m_NrOfColors/8)
			{
				iBlue= 0;
				iGreen = int(255.f * float(kk-3*m_NrOfColors/8)*SlopeDef) <<8; 			
				iRed = 255 << 16;
	
			}
			else
			{
				iBlue = int(255.f * float(kk-6*m_NrOfColors/8)*SlopeDef2);
				iGreen = 255 << 8; 			
				iRed = 255<<16;

			}
			int iColor = iRed|iGreen|iBlue;

			if (m_InvertScheme)
				m_Color[m_NrOfColors-kk-1] = iColor;
			else
				m_Color[kk] = iColor;
		}

		break;
	
	case kViridis:
		for (kk = 0; kk < m_NrOfColors ; kk++)
		{
			int nrOfColorsSource = 256;
			int index = float(kk)/m_NrOfColors*nrOfColorsSource;


			int iRed = int(cm_viridis[index][0]*255)<<16;
			int iGreen = int(cm_viridis[index][1]*255)<<8 ; 			
			int iBlue = int(cm_viridis[index][2]*255);
			int iColor = iRed|iGreen|iBlue;

			if (m_InvertScheme)
				m_Color[m_NrOfColors-kk-1] = iColor;
			else
				m_Color[kk] = iColor;
		}
		break;
	case kPlasma:
		for (kk = 0; kk < m_NrOfColors ; kk++)
		{
			int nrOfColorsSource = 256;
			int index = float(kk)/m_NrOfColors*nrOfColorsSource;


			int iRed = int(cm_plasma[index][0]*255)<<16;
			int iGreen = int(cm_plasma[index][1]*255)<<8 ; 			
			int iBlue = int(cm_plasma[index][2]*255);
			int iColor = iRed|iGreen|iBlue;

			if (m_InvertScheme)
				m_Color[m_NrOfColors-kk-1] = iColor;
			else
				m_Color[kk] = iColor;
		}
		break;
	case kJade:
		float redstart = 0.3529;
		float redmid = 0.89019;
		float redend = 0.95;

		float greenstart = 0.372549;
		float greenmid = 0.023529;
		float greenend = 0.95;

		float bluestart = 0.33725;
		float bluemid = 0.074509;
		float blueend = 0.95;

		int MixPoint = 2*m_NrOfColors/4;

		for (kk = 0; kk < m_NrOfColors ; kk++)
		{

			int iRed;
			int iGreen ; 			
			int iBlue ;
			if (kk < MixPoint)
			{

				iBlue = int(255*(float(kk)/MixPoint * (bluemid-bluestart) + bluestart));
				iGreen = int(255*(float(kk)/MixPoint * (greenmid-greenstart) + greenstart));
				iRed = int(255*(float(kk)/MixPoint * (redmid-redstart) + redstart));
			}
			else 
			{
				iBlue = int(255*(float(kk-MixPoint)/MixPoint * (blueend-bluemid) + bluemid));
				iGreen = int(255*(float(kk-MixPoint)/MixPoint * (greenend-greenmid) + greenmid));
				iRed = int(255*(float(kk-MixPoint)/MixPoint * (redend-redmid) + redmid));
	
			}
			iRed = iRed << 16;
			iGreen = iGreen << 8;
			int iColor = iRed|iGreen|iBlue;

			if (m_InvertScheme)
				m_Color[m_NrOfColors-kk-1] = iColor;
			else
				m_Color[kk] = iColor;
		}

		break;


	}
}
