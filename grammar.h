#ifndef BASIC_INT_GRAMMAR_H
#define BASIC_INT_GRAMMAR_H

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>

#include "value.h"

namespace main_pass
{
    namespace x3 = boost::spirit::x3;
    using runtime::value_t;

    using expression_type = x3::rule<class expression, value_t>;
    BOOST_SPIRIT_DECLARE( expression_type );

    using statement_type = x3::rule<class statement, value_t>;
    BOOST_SPIRIT_DECLARE( statement_type );

    using else_statement_type = x3::rule<class else_statement>;
    BOOST_SPIRIT_DECLARE( else_statement_type );

    using sequence_separator_type = x3::rule<class sequence_separator>;
    BOOST_SPIRIT_DECLARE( sequence_separator_type );

    expression_type expression_rule();
    statement_type statement_rule();
    else_statement_type else_statement_rule();
    sequence_separator_type sequence_separator_rule();
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


