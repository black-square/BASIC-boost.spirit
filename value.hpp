#ifndef BASIC_INT_VALUE_H
#define BASIC_INT_VALUE_H

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>

namespace runtime
{
    using float_t = float;
    using int_t = int16_t;
    using str_t = std::string;

    struct value_t : boost::spirit::x3::variant<
        int_t,
        float_t,
        str_t
    >
    {
        using base_type::base_type;
        using base_type::operator=;

        friend bool operator==( const value_t& v1, const value_t& v2 )
        {
            return v1.var == v2.var;
        }

        friend bool operator!=( const value_t& v1, const value_t& v2 )
        {
            return !(v1 == v2);
        }
    };

    enum class ValueType
    {
        Int,
        Float,
        Str
    };
}


#endif // BASIC_INT_VALUE_H

