#ifndef BASIC_INT_GRAMMAR_H
#define BASIC_INT_GRAMMAR_H

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>

#include "value.hpp"

namespace main_pass
{
    namespace x3 = boost::spirit::x3;
    using runtime::value_t;

    using expression_type = x3::rule<class expression, value_t>;
    BOOST_SPIRIT_DECLARE( expression_type );

    using line_type = x3::rule<class line, value_t>;
    BOOST_SPIRIT_DECLARE( line_type );

    expression_type expression_rule();
    line_type line_rule();
}

namespace preparse
{
    namespace x3 = boost::spirit::x3;
    using runtime::value_t;

    using line_type = x3::rule<class line>;
    BOOST_SPIRIT_DECLARE( line_type );

    line_type line_rule();
}


#endif // BASIC_INT_GRAMMAR_H


