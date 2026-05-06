// C++可以保存任意类型的容器
#pragma once
#include "utils/public.h"
#include <typeinfo>
class Any
{
   private:
    class Base
    {
       public:
        virtual ~Base();
        virtual std::type_info const &GetType() const = 0;
        virtual Base *Clone() const = 0;
    };

    template <class T>
    class Placeholder : public Base
    {
       public:
        Placeholder(T const &value);
        virtual std::type_info const &GetType() const;
        virtual Base *Clone() const;
        T value;
    };
    Base *_content;

   public:
    Any();
    template <class T>
    Any(T const &value);
    Any(Any const &other);
    ~Any();

    Any &Swap(Any &other);

    template <class T>
    T *Get();

    template <class T>
    Any &operator=(T const &value);

    Any &operator=(Any const &other);
};

// 模板实现:必须放在头文件以便在实例化点可见
template <class T>
Any::Placeholder<T>::Placeholder(T const &value) : value(value)
{
}

template <class T>
std::type_info const &Any::Placeholder<T>::GetType() const
{
    return typeid(T);
}

template <class T>
Any::Base *Any::Placeholder<T>::Clone() const
{
    return new Placeholder(value);
}

template <class T>
Any::Any(T const &value) : _content(new Placeholder<T>(value))
{
}

template <class T>
T *Any::Get()
{
    if (_content && _content->GetType() == typeid(T)) return &static_cast<Placeholder<T> *>(_content)->value;
    return nullptr;
}

template <class T>
Any &Any::operator=(T const &value)
{
    Any(value).Swap(*this);
    return *this;
}