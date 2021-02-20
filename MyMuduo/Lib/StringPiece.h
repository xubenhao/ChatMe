#ifndef NLIB_STRINGPIECE_H
#define NLIB_STRINGPIECE_H
#include "header.h"

class StringArg
{
public:
    StringArg(const char* str_)
        : m_str(str_)
    { 
    
    }

    StringArg(const string& str_)
        : m_str(str_.c_str())
    {
    }

    const char* c_str() const 
    { 
        return m_str; 
    }
    
private:
    const char* m_str;
};


class StringPiece
{
private:
    const char* m_ptr;
    int m_length;
    
public:
    StringPiece()
        : m_ptr(NULL), m_length(0) 
    { 
    }
      
    StringPiece(const char* str_)
        : m_ptr(str_)
    {
        m_length = strlen(m_ptr);
    }

    StringPiece(const unsigned char* str_)
    { 
        m_ptr = (const char*)str_;
        m_length = strlen(m_ptr);
    }

    StringPiece(const string& str_)
        : m_ptr(str_.data()), 
        m_length((int)(str_.size())) 
    { 
    }
    
    StringPiece(const char* offset_, int len_)
        : m_ptr(offset_), m_length(len_) 
    { 
    }

    const char* data() const 
    { 
        return m_ptr; 
    }

    int size() const 
    { 
        return m_length; 
    }
    
    bool empty() const 
    { 
        return m_length == 0; 
    }
      
    const char* begin() const 
    { 
        return m_ptr; 
    }

    const char* end() const 
    { 
        return m_ptr + m_length; 
    }

    void clear() 
    { 
        m_ptr = NULL; 
        m_length = 0; 
    }
      
    void set(const char* buffer_, int len_) 
    { 
        m_ptr = buffer_; 
        m_length = len_; 
    }

    void set(const char* str_) 
    {
        m_ptr = str_;
        m_length = strlen(str_);
    }

    void set(const void* buffer_, int len_) 
    {
        m_ptr = (const char*)(buffer_);
        m_length = len_;
    }
    
    char operator[](int i) const 
    { 
        return m_ptr[i]; 
    }

    void remove_prefix(int n) 
    {
        m_ptr += n;
        m_length -= n;
    }
    
    void remove_suffix(int n) 
    {
        m_length -= n;
    }
    
    bool operator==(const StringPiece& x) const 
    {
        return ((m_length == x.m_length) 
                && (memcmp(m_ptr, x.m_ptr, m_length) == 0));
    }

    bool operator!=(const StringPiece& x) const 
    {
        return !(*this == x);
    }

    int compare(const StringPiece& x) const 
    {
        int r = memcmp(
                m_ptr, 
                x.m_ptr, 
                m_length < x.m_length ? m_length : x.m_length);
        if (r == 0) 
        {
          if (m_length < x.m_length) 
              r = -1;
          else if (m_length > x.m_length) 
              r = +1;
        }

        return r;
    }
    
    string as_string() const 
    {
        return string(data(), size());
    }

    void CopyToString(string* target) const 
    {
        target->assign(m_ptr, m_length);
    }

    bool starts_with(const StringPiece& x) const 
    {
        return ((m_length >= x.m_length) && (memcmp(m_ptr, x.m_ptr, x.m_length) == 0));
    }
};

std::ostream& operator<<(
        std::ostream& o, 
        const StringPiece& piece);

#endif  

