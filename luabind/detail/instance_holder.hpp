// Copyright Daniel Wallin 2008. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LUABIND_INSTANCE_HOLDER_081024_HPP
# define LUABIND_INSTANCE_HOLDER_081024_HPP

# include <luabind/detail/implicit_cast.hpp>
# include <luabind/get_pointer.hpp>
# include <luabind/typeid.hpp>
# include <stdexcept>

namespace luabind { namespace detail {

class instance_holder
{
public:
    instance_holder(class_rep* cls, bool pointee_const)
      : m_cls(cls)
      , m_pointee_const(pointee_const)
    {}

    virtual ~instance_holder()
    {}

    virtual std::pair<void*, int> get(type_id const&) const = 0;

    virtual void release() = 0;

    class_rep* get_class() const
    {
        return m_cls;
    }

    bool pointee_const() const
    {
        return m_pointee_const;
    }

private:
    class_rep* m_cls;
    bool m_pointee_const;
};

namespace mpl = boost::mpl;

inline mpl::false_ check_const_pointer(void*)
{
    return mpl::false_();
}

inline mpl::true_ check_const_pointer(void const*)
{
    return mpl::true_();
}

template <class T>
void release_ownership(std::auto_ptr<T>& p)
{
    p.release();
}

template <class P>
void release_ownership(P const&)
{
    throw std::runtime_error(
        "luabind: smart pointer does not allow ownership transfer");
}

template <class P, class Pointee = void const>
class pointer_holder : public instance_holder
{
public:
    pointer_holder(P p, class_rep* cls)
      : instance_holder(cls, check_const_pointer(get_pointer(p)))
      , p(p)
    {}

    std::pair<void*, int> get(type_id const& target) const
    {
        if (target == typeid(P))
            return std::pair<void*, int>(&this->p, 0);

        int offset = 0;
        int steps = implicit_cast(get_class(), target, offset);

        if (steps < 0)
            return std::pair<void*, int>(0, -1);

        void* ptr = const_cast<void*>(
            static_cast<void const*>(
                reinterpret_cast<char const*>(
                    static_cast<Pointee*>(get_pointer(p))) + offset));

        return std::pair<void*, int>(ptr, steps);
    }

    void release()
    {
        release_ownership(p);
    }

private:
    mutable P p;
};

}} // namespace luabind::detail

#endif // LUABIND_INSTANCE_HOLDER_081024_HPP
