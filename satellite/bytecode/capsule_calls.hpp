#pragma once
// satellite/bytecode/capsule_calls.hpp -- A CAPSULE CALLED FOR ITS ANSWER, AND A MEMBER
// OF AN OBJECT (2026-09-22).
//
// Until this date a capsule was called only as a statement of its own, and answered
// nothing: satellite.return ended it and handed nothing back, so `n = five()` was
// refused before it ran. The author's programs call capsules for their answers on
// about 1,500 lines (tagged_report.satl: `satellite.return("[" + program + " " + who +
// "] ")`, then `tag_of(program, who) + line`), and call an object's capsules the same
// way -- `plan.call_threads()`, `satellite.statement.if (signal.call_stop())`. Both end
// in program_walk's run_site, the one reader of "call a capsule", through
// run_capsule_for.

#include "capsule_scopes.hpp"
#include "expression.hpp"
#include "token_codes.hpp"

#include <bitset>
#include <string>
#include <vector>

namespace satellite004 {

// A CAPSULE INSIDE AN EXPRESSION -- `five()`, `other.greet(x)`, `tools.inner.deep()` --
// `at` on its first name, `names` already read and `open` on the `(` after them. `at`
// is left past the `)`. Answers what its satellite.return handed back; a spacesuit's
// capsule called by its bare name runs on this body's object.
Value call_capsule_for_its_answer(const std::vector<std::bitset<16>> &row, std::size_t &at,
                                  const std::vector<std::string> &names, std::size_t open,
                                  ExpressionContext &context);

// `.name` AFTER AN OBJECT: `at` on the `.`, and left past the call. The object's
// spacesuit says what `name` is (CapsuleTable::member), and the capsule runs on it.
Value call_member(const std::vector<std::bitset<16>> &row, std::size_t &at, const Value &object,
                  const std::string &receiver, ExpressionContext &context);

// IS `.name` AFTER THIS VALUE ONE OF ITS SPACESUIT'S MEMBERS? An object's capsule may be
// called anything -- `call_name`, and also `size` or `text`, which the lexer makes
// method codes -- so after an object, both are.
inline bool a_member_next(const std::vector<std::bitset<16>> &row, std::size_t at, const Value &value)
{
    if (!value.is_user_defined() || at >= row.size() ||
        static_cast<token::Code>(row[at].to_ulong()) != token::method_token || at + 1 >= row.size())
        return false;
    const token::Code next = static_cast<token::Code>(row[at + 1].to_ulong());
    return next == token::name_token || token::is_method_code(next);
}

} // namespace satellite004
