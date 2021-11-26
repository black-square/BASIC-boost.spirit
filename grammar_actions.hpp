#ifndef BASIC_INT_GRAMMAR_ACTIONS_H
#define BASIC_INT_GRAMMAR_ACTIONS_H

namespace actions
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

        const auto idx = ForceInt( op2 );

        op1 += std::to_string( idx );
    };

    constexpr auto cpy_int_op = []( auto& ctx )
    {
        _val( ctx ) = static_cast<int_t>(_attr( ctx ));
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

    constexpr auto add_op = []( auto& ctx )
    {
        auto& op1 = _val( ctx );
        auto&& op2 = _attr( ctx );

        op1 = AddImpl( op1, op2 );
    };

    constexpr auto sub_op = []( auto& ctx )
    {
        _val( ctx ) = ForceFloat( _val( ctx ) ) - ForceFloat( _attr( ctx ) );
    };

    constexpr auto mul_op = []( auto& ctx )
    {
        _val( ctx ) = ForceFloat( _val( ctx ) ) * ForceFloat( _attr( ctx ) );
    };

    constexpr auto div_op = []( auto& ctx )
    {
        _val( ctx ) = ForceFloat( _val( ctx ) ) / ForceFloat( _attr( ctx ) );
    };

    constexpr auto exp_op = []( auto& ctx )
    {
        _val( ctx ) = std::pow( ForceFloat( _val( ctx ) ), ForceFloat( _attr( ctx ) ) );
    };

    constexpr auto not_op = []( auto& ctx )
    {
        _val( ctx ) = int_t{ !ForceInt( _attr( ctx ) ) };
    };

    constexpr auto neg_op = []( auto& ctx )
    {
        _val( ctx ) = -ForceFloat( _attr( ctx ) ); };

    const auto less_op = []( auto& ctx )
    {
        _val( ctx ) = int_t{ ForceFloat( _val( ctx ) ) < ForceFloat( _attr( ctx ) ) };
    };

    constexpr auto greater_op = []( auto& ctx )
    {
        _val( ctx ) = int_t{ ForceFloat( _val( ctx ) ) > ForceFloat( _attr( ctx ) ) };
    };

    constexpr auto less_eq_op = []( auto& ctx )
    {
        _val( ctx ) = LessEqImpl( _val( ctx ), _attr( ctx ) );
    };

    constexpr auto greater_eq_op = []( auto& ctx )
    {
        _val( ctx ) = int_t{ ForceFloat( _val( ctx ) ) >= ForceFloat( _attr( ctx ) ) };
    };

    constexpr auto and_op = []( auto& ctx )
    {
        _val( ctx ) = int_t{ ForceInt( _val( ctx ) ) && ForceInt( _attr( ctx ) ) };
    };

    constexpr auto or_op = []( auto& ctx )
    {
        _val( ctx ) = int_t{ ForceInt( _val( ctx ) ) || ForceInt( _attr( ctx ) ) };
    };

    constexpr auto left_op = []( auto& ctx ) {
        auto&& [o1, o2] = _attr( ctx );

        _val( ctx ) = ForceStr( o1 ).substr( 0, ForceInt( o2 ) );
    };

    constexpr auto sqr_op = []( auto& ctx ) {
        const auto& v = _attr( ctx );
        _val( ctx ) = std::sqrt( ForceFloat( v ) );
    };

    constexpr auto int_op = []( auto& ctx ) {
        const auto& v = _attr( ctx );
        _val( ctx ) = ForceInt( v );
    };

    constexpr auto abs_op = []( auto& ctx ) {
        const auto& v = _attr( ctx );
        _val( ctx ) = std::fabs( ForceFloat( v ) );
    };

    constexpr auto rnd_op = []( auto& ctx ) {
        const auto v = ForceFloat(_attr( ctx ));

        if( v <= FLT_EPSILON )
            throw std::runtime_error("Only positive arguments of RND are supported");

        _val( ctx ) = v * std::rand() / (RAND_MAX + 1);
    };

    constexpr auto print_op = []( auto& ctx )
    {
        auto& runtime = x3::get<runtime_tag>( ctx ).get();
        boost::apply_visitor( [&runtime]( auto&& v ) { runtime.Print( v ); }, _attr( ctx ) );
    };

    constexpr auto print_tab_op = []( auto& ctx )
    {
        const auto& v = _attr( ctx );
        auto& runtime = x3::get<runtime_tag>( ctx ).get();

        runtime.Print( str_t( ForceInt( v ), ' ' ) );
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
        _val( ctx ) = runtime.Load( std::move( name ) );
    };

    constexpr auto read_stmt_op = []( auto& ctx ) {
        auto&& name = _attr( ctx );
        auto& runtime = x3::get<runtime_tag>( ctx ).get();
        runtime.Read( std::move( name ) );
    };

    constexpr auto if_stmt_op = []( auto& ctx ) {
        auto&& v = _attr( ctx );
        auto&& cond = at_c<0>( v );
        auto&& gotoLine = at_c<1>( v );
        const bool res = ToBoolImpl( cond );
        auto& action = x3::get<partial_parse_action_tag>( ctx ).get();
        auto& runtime = x3::get<runtime_tag>( ctx ).get();

        action = res ? PartialParseAction::Continue : PartialParseAction::Discard;

        if( action == PartialParseAction::Continue && gotoLine != MaxLineNum )
            runtime.Goto( gotoLine );
    };

    constexpr auto on_stmt_impl_op = []( auto& ctx ) {
        auto&& v = _attr( ctx );
        const auto num = ForceInt( at_c<0>( v ) );
        auto&& lines = at_c<1>( v );

        if( num <= 0 || (size_t)num > lines.size() )
            throw std::runtime_error( "ON statement incorrect branch #" + std::to_string(num) );

        return lines[num - 1];
    };

    constexpr auto on_goto_stmt_op = []( auto& ctx ) {
        auto& runtime = x3::get<runtime_tag>( ctx ).get();
        runtime.Goto( on_stmt_impl_op( ctx ) );
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

        runtime.ForLoop( std::move(name), std::move(initVal), std::move(endVal), step ? std::move( *step ) : value_t{ int_t{1} } );
    };

    constexpr auto def_stmt_op = []( auto& ctx ) {
        auto&& v = _attr( ctx );
        auto&& fncName = at_c<0>( v );
        auto&& varName = at_c<1>( v );
        auto&& exprStr = at_c<2>( v );
        auto& runtime = x3::get<runtime_tag>( ctx ).get();

        runtime.DefineFuntion( std::move(fncName), std::move(varName), std::move(exprStr) );
    };     
    
    constexpr auto call_fn_op = []( auto& ctx ) {
        auto&& v = _attr( ctx );
        auto&& fncName = at_c<0>( v );
        auto&& arg = at_c<1>( v );
        auto& runtime = x3::get<runtime_tag>( ctx ).get();

        _val( ctx ) = runtime.CallFuntion( std::move(fncName), std::move(arg) );
    };



    constexpr auto next_stmt_op = []( auto& ctx ) {
        auto&& v = _attr( ctx );
        auto& runtime = x3::get<runtime_tag>( ctx ).get();
        runtime.Next( v );
    };

    constexpr auto input_op = []( auto& ctx ) {
        auto&& v = _attr( ctx );
        auto&& prompt = at_c<0>( v );
        auto&& varName = at_c<1>( v );
        auto& runtime = x3::get<runtime_tag>( ctx ).get();
        runtime.Input( prompt, varName );
    };

    constexpr auto add_num_line_op = []( auto& ctx )
    {
        auto& runtime = x3::get<runtime_tag>( ctx ).get();
        auto&& [line, str] = _attr( ctx );

        if( !str.empty() )
            runtime.AddLine( line, str );
    };

    constexpr auto data_op = []( auto& ctx )
    {
        auto& runtime = x3::get<runtime_tag>( ctx ).get();
        auto&& v = _attr( ctx );

        runtime.AddData( std::move( v ) );
    };
}


#endif // BASIC_INT_GRAMMAR_ACTIONS_H

