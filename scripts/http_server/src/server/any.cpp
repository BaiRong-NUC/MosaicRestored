#include "server/any.h"
Any::Base::~Base() {}

Any::Any() : _content(nullptr) {}

Any::Any(Any const &other) : _content(other._content ? other._content->Clone() : nullptr) {}

Any::~Any() { delete _content; }

Any &Any::Swap(Any &other)
{
    std::swap(_content, other._content);
    return *this;
}

Any &Any::operator=(Any const &other)
{
    Any(other).Swap(*this);
    return *this;
}
