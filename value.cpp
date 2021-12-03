#include "value.h"

#include <boost/algorithm/string/replace.hpp>

namespace runtime
{
std::ostream& operator<<( std::ostream& os, const value_t& v )
{
    struct Visitor
    {
        void operator()( float_t v ) const { os << v << 'f'; }
        void operator()( int_t v ) const { os << v << 'i'; }
        void operator()( str_t v ) const
        {
            boost::replace_all( v, "\n", "\\n" );
            boost::replace_all( v, "\t", "\\t" );
            boost::replace_all( v, "\r", "\\r" );
            os << '"' << v << '"';
        }

        std::ostream& os;
    };

    boost::apply_visitor( Visitor{ os }, v );

    return os;
}

float_t ForceFloat( const value_t& v )
{
    struct Impl
    {
        float_t operator()( float_t v ) const { return v; }
        float_t operator()( int_t v ) const { return static_cast<float_t>(v); }
        float_t operator()( const str_t& v ) const { throw std::runtime_error( "Cannot be string" ); }
    };

    return boost::apply_visitor( Impl{}, v );
}

int_t ForceInt( const value_t& v )
{
    struct Impl
    {
        int_t operator()( float_t v ) const { return static_cast<int_t>(v); }
        int_t operator()( int_t v ) const { return v; }
        int_t operator()( const str_t& v ) const { throw std::runtime_error( "Cannot be string" ); }
    };

    return boost::apply_visitor( Impl{}, v );
}

const str_t& ForceStr( const value_t& v )
{
    struct Impl
    {
        const str_t& operator()( float_t v ) const { throw std::runtime_error( "Must be string" ); }
        const str_t& operator()( int_t v ) const { throw std::runtime_error( "Must be string" ); }
        const str_t& operator()( const str_t& v ) const { return v; }
    };

    return boost::apply_visitor( Impl{}, v );
}

value_t AddImpl( const value_t& op1, const value_t& op2 )
{
    const str_t* pS1 = boost::get<str_t>( &op1 );
    const str_t* pS2 = boost::get<str_t>( &op2 );

    //Strings may also be concatenated (put or joined together) through
    //the use of the "+" operator.
    return pS1 && pS2 ?
        value_t{ *pS1 + *pS2 } :
        value_t{ ForceFloat( op1 ) + ForceFloat( op2 ) };
}

int_t LessEqImpl( const value_t& op1, const value_t& op2 )
{
    return int_t{ ForceFloat( op1 ) <= ForceFloat( op2 ) };
}

bool ToBoolImpl( const value_t& v )
{
    struct Impl
    {
        bool operator()( float_t v ) const { return v != 0; }
        bool operator()( int_t v ) const { return v != 0; }
        bool operator()( const str_t& v ) const { return !v.empty(); }
    };

    return boost::apply_visitor( Impl{}, v );
}
}
