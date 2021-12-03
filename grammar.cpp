//#define BOOST_SPIRIT_X3_DEBUG

#include "grammar.h"
#include "grammar_actions.hpp"
#include "runtime.h"

#include <boost/fusion/adapted/std_tuple.hpp>

using namespace runtime;
using namespace actions;

namespace main_pass
{
    using x3::int_;
    constexpr x3::real_parser<float, x3::strict_real_policies<float>> strict_float = {};
    using x3::char_;
    using x3::lit;
    using x3::no_case;
    using x3::attr;
    using x3::eoi;
    using x3::eps;
    using x3::omit;
    using x3::lexeme;
    constexpr auto line_num = x3::ulong_long;

    using str_view = boost::iterator_range<std::string_view::const_iterator>;

    expression_type const expression( "expression" );
    statement_seq_type const statement_seq( "statement_seq" );
    statement_type const statement( "statement" );

    x3::rule<class mult_div, value_t> const mult_div( "mult_div" );
    x3::rule<class exponent, value_t> const exponent( "exponent" );
    x3::rule<class term, value_t> const term( "term" );
    x3::rule<class add_sub, value_t> const add_sub( "add_sub" );
    x3::rule<class relational, value_t> const relational( "relational" );
    x3::rule<class log_and, value_t> const log_and( "log_and" );
    x3::rule<class log_or, value_t> const log_or( "log_or" );
    x3::rule<class double_args, std::tuple<value_t, value_t>> const double_args( "double_args" );
    x3::rule<class triple_args, std::tuple<value_t, value_t, value_t>> const triple_args( "triple_args" );
    x3::rule<class identifier, std::string> const identifier( "identifier" );
    x3::rule<class var_name, std::string> const var_name( "var_name" );
    x3::rule<class string_lit, std::string> const string_lit( "string_lit" );
    
    x3::rule<class next_stmt, std::string> const next_stmt( "next_stmt" );
    x3::rule<class expression_int, int_t> const expression_int( "expression_int" );

    const auto expression_def =
        log_or;

    const auto expression_int_def = 
        expression[force_int_op];

    const auto quote = char_( '"' ); //MSVC has problems if we inline it.

    const auto string_lit_def =
        lexeme['"' >> *~quote >> '"'];

    const auto statement_end =
        ':' | eoi;

    const auto statement_seq_def =
        *lit(':') >> statement;

    const auto single_arg =
        '(' >> expression >> ')';

    const auto double_args_def =
        '(' >> expression >> ',' >> expression >> ')';

    const auto triple_args_def =
        '(' >> expression >> ',' >> expression >> ',' >> expression >> ')';

    const auto print_comma =
        ',' >> attr( value_t{ "\t" } )[print_op];

    const auto print_arg =
        +(
            +print_comma |
            no_case["tab"] >> single_arg[print_tab_op] |
            expression[print_op]
            );

    const auto print_stmt =
        (no_case["print"] >> print_arg >> *(';' >> print_arg) >> (
            ';' | (&statement_end >> attr( value_t{ "\n" } )[print_op])
            )) |
        no_case["print"] >> attr( value_t{ "\n" } )[print_op];

    const auto input_stmt =
        no_case["input"] >>
        (
            (string_lit >> ';' >> var_name)[input_op] |
            (attr( std::string{ "?" } ) >> var_name)[input_op]
            ) >>
        *((',' >> attr( std::string{ "??" } ) >> var_name)[input_op]);

    const auto next_stmt_def =
        no_case["next"] >> (var_name % ',')[next_stmt_op] |
        no_case["next"] >> attr( std::vector<std::string>{} )[next_stmt_op];

    const auto for_stmt =
        (no_case["for"] >> var_name >> '=' >> expression >> no_case["to"] >> expression >>
          -(no_case["step"] >> expression)
          )[for_stmt_op];


    //A straightforward approach here will have problems with reseting boost::optional<linenum_t> 
    //during backtracking:
    //  https://github.com/boostorg/spirit/issues/378
    //  https://stackoverflow.com/a/49309385/3415353  
    const auto if_stmt =
        (no_case["if"] >> expression >> no_case["then"] >> line_num)[if_stmt_op] |
        (no_case["if"] >> expression >> -no_case["then"] >> attr( MaxLineNum ))[if_stmt_op]
        ;

    const auto on_stmt =
        (no_case["on"] >> expression >> no_case["goto"] >> line_num % ',')[on_goto_stmt_op] |
        (no_case["on"] >> expression >> no_case["gosub"] >> line_num % ',')[on_gosub_stmt_op]
        ;

    const auto var_name_dim =
        (identifier >> '(' >> expression_int % ',' >> ')')[dim_stmt_op] |
        (identifier >> attr( std::vector<int_t>{} ))[dim_stmt_op]
        ;

    const auto restore_stmt = 
        no_case["restore"] >> line_num [restore_stmt_op] |
        no_case["restore"] >> attr( MaxLineNum )[restore_stmt_op]
        ;

    const auto statement_def =
        no_case["text"] |
        no_case["home"] |
        no_case["cls"] |
        no_case["stop"][stop_stmt_op] |
        print_stmt |
        input_stmt |
        if_stmt |
        on_stmt |
        no_case["goto"] >> line_num[goto_stmt_op] |
        no_case["gosub"] >> line_num[gosub_stmt_op] |
        no_case["return"][return_stmt_op] |
        for_stmt |
        next_stmt |
        no_case["end"][end_stmt_op] |
        no_case["dim"] >> var_name_dim % ',' |
        restore_stmt |
        no_case["read"] >> var_name[read_stmt_op] % ',' |
        no_case["randomize"] >> expression[randomize_stmt_op] |
        no_case["rem"] >> omit[lexeme[*char_]] |
        no_case["def"] >> no_case["fn"] >> (identifier >> '(' >> identifier >> ')' >> '=' >> lexeme[+~char_(':')])[def_stmt_op] |
        (-no_case["let"] >> var_name >> '=' >> expression)[assing_var_op]
        ;

    const auto identifier_def = x3::raw[lexeme[(x3::alpha | '_') >> *(x3::alnum | '_') >> -(lit( '%' ) | '$')]];

    const auto var_name_def =
        identifier[cpy_op] >> char_( '(' )[append_op] >> expression[append_idx_op] % char_( ',' )[append_op] >> char_( ')' )[append_op] |
        identifier[cpy_op]
        ;

    const auto term_def =
        strict_float[cpy_op] |
        int_[cpy_int_op] |
        string_lit[cpy_op] |
        '(' >> expression[cpy_op] >> ')' |
        '-' >> term[neg_op] |
        '+' >> term[cpy_op] |
        (no_case["not"] >> term[not_op]) |
        no_case["sqr"] >> single_arg[sqr_op] |
        no_case["int"] >> single_arg[int_op] |
        no_case["abs"] >> single_arg[abs_op] |
        no_case["left$"] >> double_args[left_op] |
        no_case["mid$"] >> triple_args[mid_op] |
        no_case["len"] >> single_arg[len_op] |
        no_case["asc"] >> single_arg[asc_op] |
        no_case["rnd"] >> single_arg[rnd_op] |
        no_case["inkey$"][inkey_op] |
        no_case["fn"] >> (identifier >> single_arg) [call_fn_op] |
        var_name[load_var_op]
        ;

    const auto exponent_def =
        term[cpy_op] >> *(
            ('^' >> term[exp_op])
            );

    const auto mult_div_def =
        exponent[cpy_op] >> *(
            ('*' >> exponent[mul_op]) |
            ('/' >> exponent[div_op])
            );

    const auto add_sub_def =
        mult_div[cpy_op] >> *(
            ('+' >> mult_div[add_op]) |
            ('-' >> mult_div[sub_op])
            );

    const auto relational_def =
        add_sub[cpy_op] >> *(
            ((lit( "==" ) | '=') >> add_sub[eq_op]) |
            ('<' >> lit( '>' ) >> add_sub[not_eq_op]) |
            ('<' >> add_sub[less_op]) |
            ('>' >> add_sub[greater_op]) |
            ('<' >> lit( '=' ) >> add_sub[less_eq_op]) |
            ('>' >> lit( '=' ) >> add_sub[greater_eq_op])
            );

    const auto log_and_def =
        relational[cpy_op] >> *(
            no_case["and"] >> relational[and_op]
            );

    const auto log_or_def =
        log_and[cpy_op] >> *(
            no_case["or"] >> log_and[or_op]
            );

    BOOST_SPIRIT_DEFINE( expression, expression_int, exponent, mult_div, term, add_sub, relational, log_and, log_or,
                         double_args, triple_args, identifier, var_name, string_lit, statement, statement_seq, next_stmt
    );

    expression_type expression_rule()
    {
        return expression;
    }

    statement_type statement_rule()
    {
        return statement;
    }

    statement_seq_type statement_seq_rule()
    {
        return statement_seq;
    }
}

namespace preparse
{
    using x3::char_;
    using x3::int_;
    using x3::eoi;
    using x3::attr;
    using x3::no_case;
    using x3::omit;
    using x3::lexeme;
    using main_pass::line_num;
    using main_pass::strict_float;
    using main_pass::string_lit;

    line_type const line( "line" );
    x3::rule<class statement> const statement( "statement" );

    const auto data_stmt =
        no_case["data"] >> (
            strict_float[data_op] |
            int_[data_op] |
            string_lit[data_op]
            ) % ',';

    const auto statement_def =
        no_case["rem"] >> omit[lexeme[*char_]] |
        data_stmt;

    const auto num_line =
        (line_num[update_cur_line_op] >> statement % ':' >> attr(std::string{}))[add_num_line_op] |
        (line_num >> lexeme[+char_])[add_num_line_op];

    const auto line_def =
        num_line |
        ':' >> lexeme[+char_][append_line_op] |
        eoi;

    BOOST_SPIRIT_DEFINE( line, statement );

    line_type line_rule()
    {
        return line;
    }
}

namespace x3 = boost::spirit::x3;

using iterator_type = std::string_view::const_iterator;
using phrase_context_type = x3::phrase_parse_context<x3::ascii::space_type>::type;

template<class RuntimeT>
using context_type = x3::context<
    line_begin_tag, iterator_type,  
        x3::context<
            runtime_tag, std::reference_wrapper<RuntimeT>,
            phrase_context_type
        >
    >;
 
namespace main_pass
{
    BOOST_SPIRIT_INSTANTIATE( expression_type, iterator_type, context_type<runtime::TestRuntime> );
    BOOST_SPIRIT_INSTANTIATE( expression_type, iterator_type, context_type<runtime::FunctionRuntime> );
    BOOST_SPIRIT_INSTANTIATE( statement_seq_type, iterator_type, context_type<runtime::TestRuntime> );
    BOOST_SPIRIT_INSTANTIATE( statement_seq_type, iterator_type, context_type<runtime::Runtime> );
    BOOST_SPIRIT_INSTANTIATE( statement_type, iterator_type, context_type<runtime::SkipStatementRuntime> );
}

namespace preparse
{
    BOOST_SPIRIT_INSTANTIATE( line_type, iterator_type, context_type<runtime::Runtime> );
}
