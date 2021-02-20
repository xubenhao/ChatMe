#include "Exception.h"
#include "CurrentThread.h"

Exception::Exception(string msg)
    : m_strMessage(std::move(msg)),
    m_strStack(::stackTrace(false))
{
}

