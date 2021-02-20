#ifndef GOLBAL_GOLBAL_H
#define GOLBAL_GOLBAL_H
#include "header.h"
#define PI 3.141592

class COLOR
{
public:
    COLOR()
    {
        m_nR = 0;
        m_nG = 0;
        m_nB = 0;
        m_nA = 255;
    }

    COLOR(int nR_, int nG_, int nB_, int nA_)
    {
        m_nR = nR_;
        m_nG = nG_;
        m_nB = nB_;
        m_nA = nA_;
    }

    int m_nR;
    int m_nG;
    int m_nB;
    int m_nA;
};

struct ColorObj
{
public:
    ColorObj()
    {
        memset(m_strObjName, 0, sizeof(m_strObjName));
        m_nObjectId = -1;
    }

    ColorObj(char (&strObjName_)[100], COLOR nColor_)
    {
        strncpy(m_strObjName, strObjName_, sizeof(m_strObjName));
        m_nColor = nColor_;
        m_nObjectId = -1;
    }

    char m_strObjName[100];
    COLOR m_nColor;
    long m_nObjectId;
};

bool operator==(const COLOR& nColor1_, const COLOR& nColor2_);
bool operator!=(const COLOR& nColor1_, const COLOR& nColor2_);

bool operator==(const ColorObj& nObj1_, const ColorObj& nObj2_);
bool operator!=(const ColorObj& nObj1_, const ColorObj& nObj2_);


#endif // GOLBAL_H
