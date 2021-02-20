#ifndef NLIB_WEAKCALLBACK_H
#define NLIB_WEAKCALLBACK_H

template<typename CLASS, typename... ARGS>
class WeakCallback
{
public:
    WeakCallback(
        const std::weak_ptr<CLASS>& object,
        const std::function<void (CLASS*, ARGS...)>& function)
        : m_nObject(object), m_nFunction(function)
    {
    }
    
    void operator()(ARGS&&... args) const
    {
        std::shared_ptr<CLASS> ptr(m_nObject.lock());
        if (ptr)
        {
          m_nFunction(
            ptr.get(), 
            std::forward<ARGS>(args)...);
        }
    }
    
private:
    std::weak_ptr<CLASS> m_nObject;
    std::function<void (CLASS*, ARGS...)> m_nFunction;
};
    
template<typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(
    const std::shared_ptr<CLASS>& object,
    void (CLASS::*function)(ARGS...))
{
    return WeakCallback<CLASS, ARGS...>(object, function);
}
    
template<typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(
    const std::shared_ptr<CLASS>& object,
    void (CLASS::*function)(ARGS...) const)
{
    return WeakCallback<CLASS, ARGS...>(object, function);
}

#endif  

