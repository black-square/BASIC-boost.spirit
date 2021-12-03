#ifndef BASIC_INT_VALUE_H
#define BASIC_INT_VALUE_H

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>

namespace runtime
{
    using float_t = float;
    using int_t = int16_t;
    using str_t = std::string;
    using linenum_t = unsigned long long;

    constexpr linenum_t MaxLineNum = std::numeric_limits<linenum_t>::max();

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

    std::ostream& operator<<( std::ostream& os, const value_t& v );

    // "All arithmetic operations are done in floating point.No matter what
    //  the operands to + , -, *, / , and^ are, they will be converted to floating
    //  point.The functions SIN, COS, ATN, TAN, SQR, LOG, EXPand RND also
    //  convert their arguments to floating point and give the result as such."
    float_t ForceFloat( const value_t& v );

    // "The operators AND, OR, NOT force both operands to be integers between
    //  -32767 and +32767 before the operation occurs.
    //  When a number is converted to an integer, it is truncated (rounded down).
    //  It will perform as if INT function was applied.No automatic conversion is 
    //  done between strings and numbers"
    int_t ForceInt( const value_t& v );

    const str_t& ForceStr( const value_t& v );

    value_t AddImpl( const value_t& op1, const value_t& op2 );

    int_t LessEqImpl( const value_t& op1, const value_t& op2 );

    bool ToBoolImpl( const value_t& v );
}


#endif // BASIC_INT_VALUE_H

