// The dispatch tables, and the one function that fills them.
//
// Operators do not branch on type: `a + b` is a lookup indexed by the two type
// tags.  Adding a type to the language means filling in a row here, never
// editing an if-chain somewhere else -- so dispatch cost stays constant no
// matter how many types satellite grows.
//
// See the HYBRID DISPATCH note in container.hpp for why the hot pairs are
// still inline branches in front of these tables.
#include "ops_internal.hpp"

namespace satellite {

BinFn add_table[TCOUNT][TCOUNT];
BinFn sub_table[TCOUNT][TCOUNT];
BinFn mul_table[TCOUNT][TCOUNT];
BinFn div_table[TCOUNT][TCOUNT];
BinFn mod_table[TCOUNT][TCOUNT];
BinFn eq_table[TCOUNT][TCOUNT];
BinFn lt_table[TCOUNT][TCOUNT];

void init_tables() {
    using namespace satellite::ops;

    for (int x = 0; x < TCOUNT; ++x)
        for (int y = 0; y < TCOUNT; ++y) {
            add_table[x][y] = op_unsupported;
            sub_table[x][y] = op_unsupported;
            mul_table[x][y] = op_unsupported;
            div_table[x][y] = op_unsupported;
            mod_table[x][y] = op_unsupported;
            eq_table[x][y]  = op_eq;         // every pair is comparable
            lt_table[x][y]  = op_unsupported;
        }

    constexpr int I = (int)Type::Int, B = (int)Type::Big, R = (int)Type::Real;
    constexpr int S = (int)Type::Str, L = (int)Type::List, O = (int)Type::Bool;
    constexpr int N = (int)Type::Nil;

    add_table[I][I] = add_int_int;
    sub_table[I][I] = sub_int_int;
    mul_table[I][I] = mul_int_int;

    for (int t : {I, B}) {
        add_table[B][t] = add_big_any;  add_table[t][B] = add_big_any;
        sub_table[B][t] = sub_big_any;  sub_table[t][B] = sub_big_any;
        mul_table[B][t] = mul_big_any;  mul_table[t][B] = mul_big_any;
    }

    for (int t : {I, B, R, O}) {
        add_table[R][t] = add_real;  add_table[t][R] = add_real;
        sub_table[R][t] = sub_real;  sub_table[t][R] = sub_real;
        mul_table[R][t] = mul_real;  mul_table[t][R] = mul_real;
    }

    // strings absorb everything on +
    for (int t = 0; t < TCOUNT; ++t) {
        add_table[S][t] = add_str;
        add_table[t][S] = add_str;
    }
    sub_table[S][S] = sub_str;
    mul_table[S][I] = mul_str;
    mul_table[I][S] = mul_str;
    div_table[S][S] = div_str;              // "a,b,c" / "," splits

    // --- division and modulo ------------------------------------------------
    // Order matters below.  The Big rows are laid down first and the Real rows
    // second, because a pair with a Real in it must divide as a real even when
    // the other side is Big: 10000000000000000000000 / 2.0 is a real answer.
    // Writing them the other way round would leave Big/Real doing integer
    // division and silently truncating.
    div_table[I][I] = div_int;
    mod_table[I][I] = mod_int;
    for (int t : {I, B}) {
        div_table[B][t] = div_big_any;  div_table[t][B] = div_big_any;
        mod_table[B][t] = mod_big_any;  mod_table[t][B] = mod_big_any;
    }
    for (int t : {I, B, R, O})
        for (int u : {I, B, R, O})
            if (t == R || u == R) {
                div_table[t][u] = div_real;
                mod_table[t][u] = mod_real;
            }

    // ordering: numbers among themselves, strings among themselves
    for (int t : {I, B, R, O})
        for (int u : {I, B, R, O}) lt_table[t][u] = op_lt;
    lt_table[S][S] = op_lt;

    add_table[L][L] = add_list;

    (void)N;
}

}  // namespace satellite
