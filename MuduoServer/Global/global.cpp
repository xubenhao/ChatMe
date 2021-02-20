//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#include "global.h"

bool operator==(const COLOR& nColor1_, const COLOR& nColor2_)
{
    if(nColor1_.m_nR == nColor2_.m_nR
        && nColor1_.m_nG == nColor2_.m_nG
        && nColor1_.m_nB == nColor2_.m_nB
        && nColor1_.m_nA == nColor2_.m_nA)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool operator!=(const COLOR& nColor1_, const COLOR& nColor2_)
{
    return !operator==(nColor1_, nColor2_);
}


bool operator==(const ColorObj& nObj1_, const ColorObj& nObj2_)
{
    if(strcmp(nObj1_.m_strObjName, nObj2_.m_strObjName) == 0
        && nObj1_.m_nColor == nObj2_.m_nColor)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool operator!=(const ColorObj& nObj1_, const ColorObj& nObj2_)
{
    return !operator==(nObj1_, nObj2_);
}
