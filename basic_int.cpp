//#define BOOST_SPIRIT_X3_DEBUG
#define NOMINMAX
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <iostream>
#include <string>
#include <deque>
#include <list>
#include <numeric>

#define BOOST_TEST_MODULE BasicInt
#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/included/unit_test.hpp>


namespace x3 = boost::spirit::x3;


namespace client
{
    using float_t = float;
    using int_t = int16_t;
    using str_t = std::string;

    struct value_t : x3::variant<
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

    // All arithmetic operations are done in floating point.No matter what
    // the operands to + , -, *, / , and^ are, they will be converted to floating
    // point.The functions SIN, COS, ATN, TAN, SQR, LOG, EXPand RND also
    // convert their arguments to floating point and give the result as such.
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

    // The operators AND, OR, NOT force both operands to be integers between
    // -32767 and +32767 before the operation occurs.
    // When a number is converted to an integer, it is truncated (rounded down).
    // It will perform as if INT function was applied.No automatic conversion is 
    // done between strings and numbers
    int_t ForceInt( const value_t& v )
    {
        struct Impl
        {
            int_t operator()( float_t v ) const { return static_cast<int_t>(v); }
            int_t operator()( int_t v ) const { return v; }
            int_t operator()( const str_t& v ) const { throw std::runtime_error("Cannot be string"); }
        };

        return boost::apply_visitor( Impl{}, v );
    }

    const str_t &ForceStr( const value_t& v )
    {
        struct Impl
        {
            const str_t &operator()( float_t v ) const { throw std::runtime_error( "Must be string" ); }
            const str_t &operator()( int_t v ) const { throw std::runtime_error( "Must be string" ); }
            const str_t &operator()( const str_t& v ) const { return v; }
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
             value_t{ *pS1 + *pS2 }:
             value_t{ ForceFloat( op1 ) + ForceFloat( op2 )};
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

    using linenum_t = unsigned long long;
    constexpr linenum_t MaxLineNum = std::numeric_limits<linenum_t>::max();

    struct partial_parse_action_tag;

    enum class PartialParseAction
    {
        Undefined,
        Continue,
        Discard
    };

    struct runtime_tag;

    class RuntimeBase
    {
    public:
        void Store( std::string name, value_t val )
        {
            if( name.empty() )
                throw std::runtime_error( "variable name cannot be empty" );

            boost::algorithm::to_lower( name );

            value_t res;
            const str_t* pS = boost::get<str_t>( &val );

            switch( DetectVarType(name) )
            {
                case ValueType::Str:
                    if( !pS )
                        throw std::runtime_error( "Expected String variable" );

                    res = std::move(val);
                    break;

                case ValueType::Int:
                    res = ForceInt( val );
                    break;

                default:
                    res = ForceFloat( val );
            };

            mVars[name] = res;
        }

        value_t Load( std::string name )
        {
            if( name.empty() )
                throw std::runtime_error( "variable name cannot be empty" );

            boost::algorithm::to_lower( name );

            const auto it = mVars.find(name);

            if( it != mVars.end() )
                return it->second;

            std::cout << "WARNING: Access var before init: " << name << std::endl;

            switch( DetectVarType(name) )
            {
            case ValueType::Str:
                return value_t{str_t{}};
      
            default:
                return value_t{int_t{}};
            };
        }

        void Clear()
        {
            mVars.clear();
        }
        
    public:
        static ValueType DetectVarType( std::string_view str )
        {
            for( char c: str )
            {
                switch( c )
                {
                case '$': return ValueType::Str;
                case '%': return ValueType::Int;
                case '(': return ValueType::Float;
                }
            }

            return ValueType::Float;
        }

    private:
        std::unordered_map<std::string, value_t> mVars;
    };

    class Runtime: public RuntimeBase
    {
    public:
        void AddLine( linenum_t line, std::string_view str )
        {
            mProgram.emplace(line, str);
        }

        std::tuple<const std::string*, linenum_t> GetNextLine()
        {
            const auto it = mProgram.lower_bound(mCurLine);

            if( it == mProgram.end() )
                return {};

            Goto( it->first + 1 );

            return { &it->second, it->first };
        }

        void Goto( linenum_t line )
        {
            mCurLine = line;
        }

        void Gosub( linenum_t line )
        {
            mGosubStack.push_back( mCurLine );
            Goto( line );
        }

        void Return()
        {
            if( mGosubStack.empty() )
                throw std::runtime_error( "Mismatched GOSUB/RETURN statement" );

            Goto( mGosubStack.back() );
            mGosubStack.pop_back();
        }

        void ForLoop( std::string varName, value_t initVal, value_t targetVal, value_t stepVal )
        {
            Store( varName, std::move(initVal) );
            mForLoopStack.push_back({std::move(varName), std::move(targetVal), std::move(stepVal), mCurLine} );
        }

        void Next( const std::string &varName )
        {
            for( ;; )
            {
                if( mForLoopStack.empty() )
                    throw std::runtime_error( "Mismatched FOR/NEXT statement" );

                if( varName.empty() || varName == mForLoopStack.back().varName )
                    break;

                mForLoopStack.pop_back();
            }

            auto &cur = mForLoopStack.back();    
            auto curVal = Load( cur.varName );
            curVal = AddImpl( curVal, cur.stepVal );
            Store( cur.varName, curVal );

            const int_t eqRes = LessEqImpl( curVal, cur.targetVal );

            if( eqRes != 0 )
                Goto( cur.startBodyLine );
            else
                mForLoopStack.pop_back();
        }

        template<class T>
        void Print( T && val ) const
        {
            std::cout << val;
        }

        void Input( const std::string& prompt, const std::string& name )
        {
            if( name.empty() )
                throw std::runtime_error( "variable name cannot be empty" );

            value_t res;
            std::string str;

            for( ;; )
            {
                std::cout << prompt;

                if( mFakeInput.empty() )
                {
                    if( !std::getline( std::cin, str ) )
                        throw std::runtime_error( "std::getline() error" );
                }
                else
                {
                    str = std::move(mFakeInput.front());
                    mFakeInput.pop_front();
                    std::cout << str << std::endl;
                }

                const char * const pBeg = str.data();
                const char * const pEnd = pBeg + str.size();
                char* pLast = nullptr;
              
                switch( DetectVarType(name) )
                {
                case ValueType::Str:
                    res = std::move( str );
                    pLast = const_cast<char*>(pEnd);
                    break;

                case ValueType::Int:
                    res = static_cast<int_t>(std::strtol( pBeg, &pLast, 10 ));
                    break;

                default:
                    res = std::strtof( pBeg, &pLast );
                };

                if( pLast == pEnd )
                    break;

                std::cout << "?REENTER" << std::endl;
            } 

            Store( name, res );
        }

        void AddFakeInput( std::string str )
        {
            mFakeInput.push_back( std::move(str) );
        }

        void AddData( int v )
        {
            mData.push_back( value_t{ static_cast<int_t>(v) } );
        }            
        
        void AddData( float v )
        {
            mData.push_back( value_t{ float_t{v} } );
        }

        void AddData( str_t v )
        {
            mData.push_back( value_t{ std::move(v) } );
        }

        void Read( std::string name )
        {
            if( mCurDataIdx >= mData.size() )
                throw std::runtime_error("Out of DATA");

            Store( std::move(name), mData[mCurDataIdx]);
            ++mCurDataIdx;
        }

        void Restore()
        {
            mCurDataIdx = 0;
        }

        void Clear()
        {
            RuntimeBase::Clear();
            mProgram.clear();        
            mForLoopStack.clear();
            mGosubStack.clear();
            mFakeInput.clear();
            mCurLine = 0;
            mData.clear();
            mCurDataIdx = 0;
        } 

    private:
        struct ForLoopItem
        {
            std::string varName;
            value_t targetVal;
            value_t stepVal;
            linenum_t startBodyLine;
        };

    private:
        std::map<linenum_t, std::string> mProgram;
        std::vector<ForLoopItem> mForLoopStack;
        std::vector<linenum_t> mGosubStack;
        std::deque<std::string> mFakeInput;
        linenum_t mCurLine = 0;
        std::vector<value_t> mData;
        size_t mCurDataIdx = 0;

    };

    namespace operations
    {
        using boost::fusion::at_c;

        constexpr auto cpy_op = []( auto& ctx )
        { 
            _val( ctx ) = _attr( ctx ); 
        };

        constexpr auto append_op = []( auto& ctx )
        {
            auto& op1 = _val( ctx );
            auto&& op2 = _attr( ctx );

            op1 += op2;
        };

        constexpr auto append_idx_op = []( auto& ctx )
        {
            auto& op1 = _val( ctx );
            auto&& op2 = _attr( ctx );

            const auto idx = ForceInt(op2);

            op1 += std::to_string( idx );
        };

        constexpr auto cpy_int_op = []( auto& ctx )
        { 
            _val( ctx ) = static_cast<int_t>( _attr( ctx) ); 
        };

        constexpr auto eq_op = []( auto& ctx )
        {
            auto& op1 = _val( ctx );
            auto&& op2 = _attr( ctx );

            const str_t* pS1 = boost::get<str_t>( &op1 );
            const str_t* pS2 = boost::get<str_t>( &op2 );
            
            op1 = int_t{ pS1 && pS2 ?
                *pS1 == *pS2 :
                ForceFloat( op1 ) == ForceFloat( op2 )
            };
        };

        constexpr auto not_eq_op = []( auto& ctx )
        {
            auto& op1 = _val( ctx );
            auto&& op2 = _attr( ctx );

            const str_t* pS1 = boost::get<str_t>( &op1 );
            const str_t* pS2 = boost::get<str_t>( &op2 );

            op1 = int_t{ pS1 && pS2 ?
                *pS1 != *pS2 :
                ForceFloat( op1 ) != ForceFloat( op2 )
            };
        };

        constexpr auto add_op = []( auto & ctx )
        { 
            auto & op1 = _val( ctx );
            auto && op2 = _attr( ctx );

            op1 = AddImpl( op1, op2 );
        };

        constexpr auto sub_op = []( auto& ctx )
        { 
            _val(ctx) = ForceFloat(_val( ctx )) - ForceFloat(_attr(ctx)); 
        };

        constexpr auto mul_op = []( auto& ctx )
        {
            _val( ctx ) = ForceFloat(_val( ctx)) * ForceFloat(_attr(ctx));
        };
                
        constexpr auto div_op = []( auto& ctx )
        {
            _val( ctx ) = ForceFloat(_val( ctx)) / ForceFloat(_attr(ctx));
        };

        constexpr auto exp_op = []( auto& ctx )
        { 
            _val( ctx ) = std::pow( ForceFloat(_val(ctx)), ForceFloat(_attr(ctx)) );
        };

        constexpr auto not_op = []( auto& ctx )
        { 
            _val( ctx ) = int_t{ !ForceInt(_attr(ctx)) };
        };

        constexpr auto neg_op = []( auto& ctx )
        { 
            _val( ctx ) = -ForceFloat(_attr( ctx )); };

        const auto less_op = []( auto& ctx )
        { 
            _val( ctx ) = int_t{ ForceFloat(_val( ctx )) < ForceFloat(_attr( ctx )) };
        };

        constexpr auto greater_op = []( auto& ctx )
        { 
            _val( ctx ) = int_t{ ForceFloat(_val( ctx )) > ForceFloat(_attr( ctx )) };
        };
        
        constexpr auto less_eq_op = []( auto& ctx )
        {
            _val( ctx ) = LessEqImpl( _val( ctx ), _attr( ctx ) );
        };

        constexpr auto greater_eq_op = []( auto& ctx )
        { 
            _val( ctx ) = int_t{ ForceFloat(_val( ctx )) >= ForceFloat(_attr( ctx) ) };
        };

        constexpr auto and_op = []( auto& ctx )
        { 
            _val( ctx ) = int_t{ ForceInt(_val( ctx )) && ForceInt(_attr( ctx )) }; 
        };

        constexpr auto or_op = []( auto& ctx )
        { 
            _val( ctx ) = int_t{ ForceInt(_val( ctx )) || ForceInt(_attr( ctx )) };
        };

        constexpr auto left_op = []( auto& ctx ) {
           auto && [o1, o2] = _attr( ctx );

           _val( ctx ) = ForceStr(o1).substr( 0, ForceInt( o2 ) );
        };

        constexpr auto sqr_op = []( auto& ctx ) {
            const auto& v = _attr( ctx );
            _val( ctx ) = std::sqrt( ForceFloat(v) );
        };

        constexpr auto int_op = []( auto& ctx ) {
            const auto& v = _attr( ctx );
            _val( ctx ) = ForceInt( v );
        };

        constexpr auto abs_op = []( auto& ctx ) {
            const auto& v = _attr( ctx );
            _val( ctx ) = std::fabs(ForceFloat(v));
        };

        constexpr auto print_op = []( auto& ctx ) 
        {
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            boost::apply_visitor( [&runtime](auto&& v) { runtime.Print(v); }, _attr( ctx ) );
        };

        constexpr auto print_tab_op = []( auto& ctx )
        {
            const auto& v = _attr( ctx );
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
           
            runtime.Print( str_t(ForceInt(v), ' ') );
        };

        constexpr auto assing_var_op = []( auto& ctx ) {
            auto&& v = _attr( ctx );
            auto&& name = at_c<0>( v );
            auto&& value = at_c<1>( v );
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            runtime.Store( name, value );
        };

        constexpr auto load_var_op = []( auto& ctx ) {
            auto&& name = _attr( ctx );
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            _val( ctx ) = runtime.Load( std::move(name) );
        };
        
        constexpr auto read_stmt_op = []( auto& ctx ) {
            auto&& name = _attr( ctx );
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            runtime.Read( std::move(name) );
        };
        
        constexpr auto if_stmt_op = []( auto& ctx ) {
            auto&& v = _attr( ctx );
            auto&& cond = at_c<0>( v );
            auto&& gotoLine = at_c<1>( v );
            const bool res = ToBoolImpl(cond);
            auto& action = x3::get<partial_parse_action_tag>( ctx ).get();
            auto& runtime = x3::get<runtime_tag>( ctx ).get();

            action = res ? PartialParseAction::Continue : PartialParseAction::Discard;

            if( action == PartialParseAction::Continue && gotoLine != MaxLineNum )
                runtime.Goto(gotoLine );
        };

        constexpr auto on_stmt_impl_op = []( auto& ctx ) {
            auto&& v = _attr( ctx );
            const auto num = (size_t)ForceInt( at_c<0>( v ) );
            auto&& lines = at_c<1>( v );

            if( num >= lines.size() )
                throw std::runtime_error( "ON statement incorrect branch" );

            return lines[num];
        };

        constexpr auto on_goto_stmt_op = []( auto& ctx ) {
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            runtime.Goto( on_stmt_impl_op(ctx) );
        };

        constexpr auto on_gosub_stmt_op = []( auto& ctx ) {
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            runtime.Gosub( on_stmt_impl_op( ctx ) );
        };

        constexpr auto goto_stmt_op = []( auto& ctx ) {
            auto&& v = _attr( ctx );
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            runtime.Goto( v );
        };         
        
        constexpr auto end_stmt_op = []( auto& ctx ) {
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            runtime.Goto( MaxLineNum );
        };        
        
        constexpr auto restore_stmt_op = []( auto& ctx ) {
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            runtime.Restore();
        };

        constexpr auto gosub_stmt_op = []( auto& ctx ) {
            auto&& v = _attr( ctx );
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            runtime.Gosub( v );
        };         
        
        constexpr auto return_stmt_op = []( auto& ctx ) {
            auto&& v = _attr( ctx );
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            runtime.Return();
        };

        constexpr auto for_stmt_op = []( auto& ctx ) {
            auto&& v = _attr( ctx );
            auto&& name = at_c<0>( v );
            auto&& initVal = at_c<1>( v );
            auto&& endVal = at_c<2>( v );
            auto&& step = at_c<3>( v );
            auto& runtime = x3::get<runtime_tag>( ctx ).get();

            runtime.ForLoop( name, initVal, endVal, step ? std::move(*step) : value_t{ int_t{1} } );
        };

        constexpr auto next_stmt_op = []( auto& ctx ) {
            auto&& v = _attr( ctx );
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            runtime.Next(v);
        };

        constexpr auto input_op = []( auto& ctx ) {
            auto&& v = _attr( ctx );
            auto&& prompt = at_c<0>( v );
            auto&& varName = at_c<1>( v );
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            runtime.Input( prompt, varName );
        };
    }

    namespace grammar
    {
        using namespace operations;

        using x3::int_;
        constexpr x3::real_parser<float, x3::strict_real_policies<float>> strict_float = {};
        using x3::char_;
        using x3::lit;
        using x3::no_case;
        using x3::attr;
        using x3::eoi;
        using x3::eps;
        using x3::omit;
        constexpr auto line_num = x3::ulong_long;
        
        using str_view = boost::iterator_range<std::string_view::const_iterator>;

        x3::rule<class mult_div, value_t> const mult_div( "mult_div" );
        x3::rule<class exponent, value_t> const exponent( "exponent" );
        x3::rule<class term, value_t> const term( "term" );
        x3::rule<class add_sub, value_t> const add_sub( "add_sub" );
        x3::rule<class relational, value_t> const relational( "relational" );
        x3::rule<class log_and, value_t> const log_and( "log_and" );
        x3::rule<class log_or, value_t> const log_or( "log_or" );
        x3::rule<class double_args, std::tuple<value_t, value_t>> const double_args( "double_args" );
        x3::rule<class identifier, std::string> const identifier( "identifier" );
        x3::rule<class var_name, std::string> const var_name( "var_name" );
        x3::rule<class string_lit, std::string> const string_lit( "string_lit" );
        x3::rule<class instruction, value_t> const instruction( "instruction" );
        x3::rule<class line_parser, value_t> const line_parser( "line_parser" );
        x3::rule<class return_stmt, std::string> const return_stmt( "return_stmt" );

        const auto expression =
            log_or;

        const auto quote = char_( '"' ); //MSVC has problems if we inline it.

        const auto string_lit_def =
            x3::lexeme['"' >> *~quote >> '"'];

        const auto line_parser_def =
            instruction % ':';

        const auto instruction_end = 
            ':' | eoi;

        const auto single_arg =
            '(' >> expression >> ')';

        const auto double_args_def =
            '(' >> expression >> ',' >> expression >> ')';

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
                ';' | (&instruction_end >> attr( value_t{ "\n" } )[print_op])       
            )) |
            no_case["print"] >> attr( value_t{ "\n" } )[print_op];

        const auto input_stmt =  
            no_case["input"] >> 
                (
                    (string_lit >> ';' >> var_name)[input_op] |
                    (attr(std::string{ "?" }) >> var_name)[input_op]
                ) >>
                *( (',' >> attr( std::string{"??"} ) >> var_name)[input_op] );

        const auto return_stmt_def =
            no_case["next"] >> -var_name;

        const auto for_stmt = 
            (no_case["for"] >> var_name >> '=' >> expression >> no_case["to"] >> expression >>
                -(no_case["step"] >> expression)
            ) [for_stmt_op];

        
        //A straightforward approach here will have problems with reseting boost::optional<linenum_t> 
        //during backtracking:
        //  https://github.com/boostorg/spirit/issues/378
        //  https://stackoverflow.com/a/49309385/3415353  
        const auto if_stmt =
            (no_case["if"] >> expression >> no_case["then"] >> line_num)[if_stmt_op] |
            (no_case["if"] >> expression >> -no_case["then"] >> attr(MaxLineNum))[if_stmt_op]
        ;

        const auto on_stmt =
            (no_case["on"] >> expression >> no_case["goto"] >> line_num % ',')[on_goto_stmt_op] |
            (no_case["on"] >> expression >> no_case["gosub"] >> line_num % ',')[on_gosub_stmt_op]
        ;

        const auto instruction_def =
            no_case["text"] |
            no_case["home"] |
            no_case["cls"] |
            print_stmt |
            input_stmt |
            if_stmt |
            on_stmt |
            no_case["goto"] >> line_num[goto_stmt_op] |
            no_case["gosub"] >> line_num[gosub_stmt_op] |
            no_case["return"][return_stmt_op] |
            for_stmt |
            return_stmt[next_stmt_op] |
            no_case["end"][end_stmt_op] |
            no_case["dim"] >> omit[x3::lexeme[+char_]] |
            no_case["restore"][restore_stmt_op] |
            no_case["read"] >> var_name[read_stmt_op] |
            no_case["rem"] >> omit[x3::lexeme[+char_]] |
            (-no_case["let"] >> var_name >> '=' >> expression )[assing_var_op]
        ;

        const auto identifier_def = x3::raw[ x3::lexeme[(x3::alpha |  '_') >> *(x3::alnum | '_' ) >> -(lit('%') | '$') ]];

        const auto var_name_def = 
            identifier[cpy_op] >> 
                char_('(')[append_op] >> expression[append_idx_op] % char_(',')[append_op] >> char_(')')[append_op] |
            identifier[cpy_op]
        ;
            
        const auto term_def =
            strict_float[cpy_op]      
             | int_[cpy_int_op]
             | string_lit[cpy_op]    
             | '(' >> expression[cpy_op] >> ')'
             | '-' >> term[neg_op]
             | '+' >> term[cpy_op]
             | (no_case["not"] >> term[not_op])    
             | no_case["sqr"] >> single_arg[sqr_op]
             | no_case["int"] >> single_arg[int_op]
             | no_case["abs"] >> single_arg[abs_op]
             | no_case["left$"] >> double_args[left_op]
             | var_name[load_var_op]
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
                ((lit("==") | '=') >> add_sub[eq_op]) |
                ('<' >> lit('>') >> add_sub[not_eq_op]) |
                ('<' >> add_sub[less_op]) |
                ('>' >> add_sub[greater_op]) |
                ('<' >> lit('=') >> add_sub[less_eq_op]) |
                ('>' >> lit('=') >> add_sub[greater_eq_op])
            );

        const auto log_and_def =
            relational[cpy_op] >> *( 
                no_case["and"] >> relational[and_op]
            );

        const auto log_or_def =
            log_and[cpy_op] >> *(
                no_case["or"] >> log_and[or_op]
            );

        BOOST_SPIRIT_DEFINE( exponent, mult_div, term, add_sub, relational, log_and, log_or, 
            double_args, identifier, var_name, string_lit, instruction, line_parser, return_stmt
        );

        const auto calculator = expression;
    }

    namespace preparse
    {
        constexpr auto add_num_line_op = []( auto& ctx )
        {
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            auto && [line, str] = _attr( ctx );

            if( !str.empty() )
                runtime.AddLine( line, str );
        };

        constexpr auto data_op = []( auto& ctx )
        {
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            auto&& v = _attr( ctx );

            runtime.AddData( std::move(v) );
        };

        using x3::char_;
        using x3::int_;
        using x3::eoi;
        using x3::no_case;
        using x3::omit;
        using grammar::line_num;
        using grammar::strict_float;
        using grammar::string_lit;

        x3::rule<class line> const line( "line" );
        x3::rule<class num_line, std::tuple<linenum_t, std::string>> const num_line( "num_line" );

        const auto preparser = line;

        const auto data_stmt = 
            no_case["data"] >> ( 
                strict_float[data_op] |
                int_[data_op] |
                string_lit[data_op]
            ) % ',';

        const auto num_line_def =
            line_num >> x3::lexeme[+char_];

        const auto line_def =
            line_num >> no_case["rem"] >> omit[x3::lexeme[+char_]] |
            line_num >> data_stmt |
            num_line[add_num_line_op] |
            eoi;

        BOOST_SPIRIT_DEFINE( line, num_line );
    }

    using grammar::calculator;
    using preparse::preparser;
    using grammar::line_parser;
}

namespace client // Enables ADL for these methods for BOOST_TEST
{
    inline bool operator==( const value_t& v1, int v2 )
    {
        return v1 == value_t{ static_cast<int_t>(v2) };
    } 
    
    inline bool operator==( const value_t& v1, float v2 )
    {
        return v1 == value_t{ v2 };
    }

    inline bool operator==( const value_t& v1, const char *v2 )
    {
        return v1 == value_t{ v2 };
    }
}

class TestRuntime : public client::Runtime
{
public:
    template<class T>
    void Print( T&& val )
    {
        mStrOut << val;
    }

    void Clear()
    {
        Runtime::Clear();
        ClearOutput();
    }
    
    void ClearOutput()
    {
        mStrOut.str( "" );
    }

    std::string GetOutput() const
    {
        return mStrOut.str();
    }

private:
    std::stringstream mStrOut;
};

template< class ParserT, class RuntimeT, class ResultT>
bool Parse( std::string_view str, ParserT &parser, RuntimeT &runtime, ResultT &result, std::string &err )
{
    using boost::spirit::x3::with;
    using namespace client;

    PartialParseAction parseAction{};

    auto&& exec = with<runtime_tag>( std::ref( runtime ) )[
        with<partial_parse_action_tag>( std::ref( parseAction ) ) [
            parser
       ]
    ];

    boost::spirit::x3::ascii::space_type space;

    bool r = false;
    auto cur = str.begin();
    const auto end = str.end();
    err.clear();

    try
    {
        for( ;; ) 
        {
            parseAction = client::PartialParseAction::Undefined;
            r = phrase_parse( cur, end, exec, space, result );

            if( !r || cur == end )
                break;

            if( parseAction != PartialParseAction::Continue )
            {
                if( parseAction == PartialParseAction::Discard )
                    cur = end;

                break;
            }     
        }
    }
    catch( const std::runtime_error& e )
    {
        err = e.what();
    }

    if( !r || cur != end )
    {
        err = (!r ? "ERROR[" : "UNEXPECTED[") + err + "]";

        if( cur != end )
        {
            err += " pos> ";
            err.append( cur, end );
        }

        return false;
    }

    return true;
}

template<class GrammarT>
struct TestExecutor
{
    TestExecutor(GrammarT &grammar):
        grammar{grammar}
    {}

    auto operator()( std::string_view str )
    {
        using client::value_t;

        runtime.ClearOutput();

        value_t res{};
        std::string err{};

        if( !Parse( str, grammar, runtime, res, err ) )
        {
            return value_t{ std::move(err) };
        }

        auto && strOut = runtime.GetOutput();

        return strOut.empty() || res != value_t{} ? res : value_t{ strOut };
    }

    const GrammarT &grammar;
    TestRuntime runtime;
};

template<class GrammarT>
struct TestExecutorClear : TestExecutor<GrammarT>
{
    using TBase = TestExecutor<GrammarT>;
    using TBase::TestExecutor;
    using TBase::runtime;

    auto operator()( std::string_view str )
    {
        runtime.Clear();
        return TBase::operator()( str );
    }
};

BOOST_AUTO_TEST_CASE( expression_test )
{
    TestExecutorClear calc{client::calculator};

    BOOST_TEST( calc( "3+4" ) == 7.f );
    BOOST_TEST( calc( "3+4*2" ) == 11.f );
    BOOST_TEST( calc( "3+(4*2)" ) == 11.f );
    BOOST_TEST( calc( "(3+4)*2" ) == 14.f );
    BOOST_TEST( calc( "-4^3" ) == -64.f );
    BOOST_TEST( calc( "4.5^3" ) == 91.125f );
    BOOST_TEST( calc( "1 + ( 2 - 3 )" ) == 0.f );
    BOOST_TEST( calc( "2 * 4 ^ 2 - 34" ) == -2.f );
    BOOST_TEST( calc( "-1 + 2 - 3 ^ 4 * 5 - 6" ) == -410.f );
    BOOST_TEST( calc( "( ( 1 + ( 2 - 3 ) + 5 ) / 2 ) ^ 2" ) == 6.25f );
    BOOST_TEST( calc( "5 + ( ( 1 + 2 ) * 4 ) - 3" ) == 14.f );
    BOOST_TEST( calc( "5 +  1 + 2  * 4 ^ 2 - 34" ) == 4.f );
    BOOST_TEST( calc( "-(123-2+5*5)-23 *32*(22-4-6)" ) == -8978.f );


    BOOST_TEST( calc( "1 + not 2^3 + 4" ) == 5.f );
    BOOST_TEST( calc( "2 + 3 = 3 - 1 * -2" ) == 1 );
    BOOST_TEST( calc( "2 + (1+1+1) == 3 - 1 * -2" ) == 1 );
    BOOST_TEST( calc( "1 < 2 and 2 < 10" ) == 1 );
    BOOST_TEST( calc( "3+(4*2) + 3 = (3+4)*2" ) == 1 );
    BOOST_TEST( calc( "1 <= 1" ) == 1 );
    BOOST_TEST( calc( "1 <= 2" ) == 1 );
    BOOST_TEST( calc( "1 < = 1" ) == 1 );
    BOOST_TEST( calc( "1 < = 2" ) == 1 );
    BOOST_TEST( calc( "1 <> 1" ) == 0 );
    BOOST_TEST( calc( "1 >= 2" ) == 0 );
    BOOST_TEST( calc( "1 > = 1" ) == 1 );
    BOOST_TEST( calc( "1 > = 2" ) == 0 );

    BOOST_TEST( calc( R"(("a " + "b")+" c")") == "a b c" );

    BOOST_TEST( calc( "SQR(156.25)" ) == 12.5f );
    BOOST_TEST( calc( "ABS(12.34)" ) == 12.34f );
    BOOST_TEST( calc( "-12.34" ) == -12.34f );
    BOOST_TEST( calc( "ABS(-12.34)" ) == 12.34f );
    BOOST_TEST( calc( R"(left$("applesoft", 5))" ) == "apple" );
}

BOOST_AUTO_TEST_CASE( line_parser_test )
{
    TestExecutorClear calc{ client::line_parser };

    BOOST_TEST( calc( R"(print)" ) == "\n" );
    BOOST_TEST( calc( R"(Print "Hello World!")" ) == "Hello World!\n" );
    BOOST_TEST( calc( R"(Print "Hello World!" ; )" ) == "Hello World!" );
    BOOST_TEST( calc( R"(print 1+1;)" ) == "2" );
    BOOST_TEST( calc( R"(print "Test": print 42 ;)" ) == "Test\n42" );
    BOOST_TEST( calc( R"(xvar3 = 37 : print "X: "; 2 + xvar3 + 3)" ) == "X: 42\n" );
    BOOST_TEST( calc( R"(xvar3 = 37 : print "X: "; : print 2 + xvar3 + 3)" ) == "X: 42\n" );
    BOOST_TEST( calc( R"(print "t", 2+45 ;:print "!!!";)" ) == "t\t47!!!" );
    BOOST_TEST( calc( R"(print ,"t",,;)" ) == "\tt\t\t" );
    BOOST_TEST( calc( R"(print ,,,"t",, 2+45,""; "ab" + "cd";)" ) == "\t\t\tt\t\t47\tabcd" );
    BOOST_TEST( calc( R"(print tab(3);"a";)" ) == "   a" );

    BOOST_TEST( calc( R"(x$ = "test":print x$)" ) == "test\n" );
    BOOST_TEST( calc( R"(x% = 12:print x%)" ) == "12\n" );
    BOOST_TEST( calc( R"(x% = 2.5:print x%)" ) == "2\n" );

    BOOST_TEST( calc( R"(a(2+1)=42 : print a(3);)" ) == "42" );
    BOOST_TEST( calc( R"(n = 3: a(3, 8)=42*2 : print a(n, n*2+2);)" ) == "84" );
    BOOST_TEST( calc( R"(n = 3: a(2, n*2)=43 : b( a(2,6), 1,2,3) = 400/4 : print b(43, 1,   2, 3);)" ) == "100" );

    BOOST_TEST( calc( R"(if 1=1 then print "OK")" ) == "OK\n" );
    BOOST_TEST( calc( R"(if 0=1 then print "OK")" ) == 0 );
    BOOST_TEST( calc( R"(if "" + "" then print "OK")" ) == 0 );
    BOOST_TEST( calc( R"(if "" + "" + "a" then print "OK")" ) == "OK\n" );
    BOOST_TEST( calc( R"(x=23 : if x==23 then print "OK")" ) == "OK\n" );
    BOOST_TEST( calc( R"(if 2 then if 3 then x$ = "OK": print x$;)" ) == "OK" );
    BOOST_TEST( calc( R"(if 2 then if 2 - 1 * 2 then x$ = "OK": print x$;)" ) == 0 );
    BOOST_TEST( calc( R"(if 2 then if 2 - 1 * 3 then x$ = "OK": print x$;)" ) == "OK" );
}

void InteractiveMode()
{
    using namespace client;
    using boost::spirit::x3::with;
                                             
    std::cout << "-------------------------\n";
    std::cout << "Interactive mode\n";
    std::cout << "-------------------------\n";

    TestExecutor calc{ client::line_parser };

    std::string str;

    for(;;)
    {
        std::cout << ">>> ";

        if( !std::getline( std::cin, str ) || str.empty() )
            break;

        const auto res = calc(str);

        std::cout << res << std::endl;
    }
}

bool Preparse( const char* szFileName, client::Runtime &runtime )
{
    using boost::spirit::x3::with;

    std::ifstream flIn{ szFileName };

    std::string str;

    auto&& parser = with<client::runtime_tag>( std::ref( runtime ) )[
        client::preparser
    ];

    while( std::getline( flIn, str ) )
    {
        auto iter = str.begin();
        auto end = str.end();
        boost::spirit::x3::ascii::space_type space;

        bool r = false;
        std::string err{};

        try
        {
            r = phrase_parse( iter, end, parser, space );
        }
        catch( const std::runtime_error& e )
        {
            err = e.what();
        }

        if( !r || iter != end )
        {
            std::string rest( iter, end );
            std::cout << "-------------------------\n";
            std::cout << "Preparse failed" << err << "\n";
            std::cout << "stopped at: \"" << rest << "\"\n";
            std::cout << "-------------------------\n";

            return false;
        }
    }

    return true;
}

bool Execute( client::Runtime& runtime )
{
    using namespace client;

    for(;;) 
    {
        const auto [pStr, lineNum] = runtime.GetNextLine();

        if( !pStr )
            return true;

        value_t res{};
        std::string err{};

        if( !Parse( *pStr, line_parser, runtime, res, err ) )
        {
            std::cout << "-------------------------\n";
            std::cout << "Execute failed\n" << lineNum << '\t' << *pStr << "\n";
            std::cout << "Error: " << err << "\n";
            std::cout << "-------------------------\n";
            return false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    const char *rgBootTestArgs[] = { argv[0], "--log_level=test_suite" };
    
    boost::unit_test::unit_test_main( init_unit_test, std::size(rgBootTestArgs), const_cast<char**>(rgBootTestArgs) );

    client::Runtime runtime;

    for( int i = 1; i < argc; ++i )
    {
        if( boost::algorithm::ends_with( argv[i], ".input" ) )
        {
            std::cout << "-------------------------\n";
            std::cout << "Read input: " << argv[i] << std::endl;
            std::cout << "-------------------------\n";

            std::ifstream flIn( argv[i] );
            std::string str;

            while( std::getline( flIn, str ) )
                runtime.AddFakeInput( std::move(str) );

            continue;
        }

        std::cout << "-------------------------\n";
        std::cout << "Running: " << argv[i] << std::endl;
        std::cout << "-------------------------\n";

        bool res = Preparse( argv[i], runtime );

        if( res )
            res = Execute( runtime );

        std::cout << std::endl << (res ? "[SUCCESS]": "[FAIL]")  << std::endl << std::endl;

        runtime.Clear();
    }

    InteractiveMode();

    return 0;
}
