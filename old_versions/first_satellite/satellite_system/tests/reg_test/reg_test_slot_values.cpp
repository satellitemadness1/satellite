// reg_test_slot_values.cpp — what one slot can hold: the three slot states, and
// the round trip through every alternative a Value has. Part of the reg_test
// binary; the harness and main() are in reg_test.cpp.

#include "reg_test.hpp"

#include <memory>
#include <variant>

using namespace satellite;

void reg_test_slot_states()
{
    // --- the three states ---------------------------------------------------
    // The rule the whole type exists to preserve. A default Reg is EMPTY, and
    // EMPTY is not nil: src/evaluator/slots.cpp reports "is read before its declaration
    // runs" by testing exactly this distinction, and it survives the move to
    // registers only if the tag carries it.
    Reg fresh;
    check(fresh.is_empty(), "a default Reg is EMPTY");
    check(!fresh.is_set(), "an EMPTY slot is not set");
    check(!fresh.is_nil(), "EMPTY is not nil");
    check(Reg::EMPTY == 0, "EMPTY is tag 0, so a zeroed slot is unset");

    Reg n = Reg::nil();
    check(n.is_nil(), "an explicit nil is nil");
    check(n.is_set(), "a nil slot IS set — nil is a value");
    check(!n.is_empty(), "nil is not EMPTY");
    check(fresh.tag != n.tag, "EMPTY and NIL are different tags");
}

void reg_test_value_round_trips()
{
    // --- round trips --------------------------------------------------------
    // Every alternative a Value can hold, including the one added with the map.
    //
    // This assert is the tripwire the comment below USED to be. Reg boxes
    // anything it does not recognise, so a new alternative compiles, passes and
    // is never converted in either direction — which is exactly what happened
    // to ArgsRef. A count in a printf did not catch it. This does, at compile
    // time, and the fix when it fires is to add the alternative to the block
    // below and to the loop at the end of it, not to bump the number.
    static_assert(std::variant_size_v<ValueBase> == 12,
                  "a Value alternative was added: extend the round-trip block "
                  "below, then update this count");
    check(Reg::from_value(nullptr).is_empty(),
          "a null ValuePtr becomes EMPTY, not nil");

    auto v_nil = std::make_shared<const Value>(std::monostate{});
    check(Reg::from_value(v_nil).is_nil(), "monostate becomes NIL");

    auto v_true = std::make_shared<const Value>(true);
    Reg r_true = Reg::from_value(v_true);
    check(r_true.tag == Reg::BOOL && r_true.b, "true rides inline");

    auto v_small = std::make_shared<const Value>(Number(42));
    Reg r_small = Reg::from_value(v_small);
    check(r_small.is_small(), "a small number rides inline");
    check_str(r_small.to_string(), "42", "an inline number renders");

    // A number too large for the small form must BOX rather than truncate.
    Number huge = Number(1);
    for (int i = 0; i < 40; i++)
        huge = Number::mul(huge, Number(10));
    auto v_huge = std::make_shared<const Value>(huge);
    Reg r_huge = Reg::from_value(v_huge);
    check(r_huge.is_heap(), "a number past the small form is boxed");
    check_str(r_huge.to_string(), huge.to_string(),
              "a boxed number renders as itself");

    auto v_str = std::make_shared<const Value>(make_string(encode("hi")));
    check(Reg::from_value(v_str).is_heap(), "a string is boxed");

    auto v_list = std::make_shared<const Value>(make_list(List{}));
    check(Reg::from_value(v_list).is_heap(), "a list is boxed");

    auto v_map = std::make_shared<const Value>(make_map(MapBody{}));
    Reg r_map = Reg::from_value(v_map);
    check(r_map.is_heap(), "a map is boxed");
    check_str(r_map.to_string(), "{}", "a boxed map renders");

    auto v_time = std::make_shared<const Value>(Time{7});
    check(Reg::from_value(v_time).is_heap(), "a time is boxed");

    // §21. Enumerated by hand like every alternative above it, which is
    // exactly why this line has to exist: Reg boxes anything it does not
    // recognise, so a tenth alternative COMPILES and passes without ever
    // being converted in either direction. The count in the PASS line is
    // the tripwire that catches the next one.
    auto v_bits = std::make_shared<const Value>(make_bits(16, "00FF"));
    Reg r_bits = Reg::from_value(v_bits);
    check(r_bits.is_heap(), "a hex value is boxed");
    check_str(r_bits.to_string(), "x00FF", "a boxed hex value renders");

    // The eleventh, and the proof that the tripwire above was worth
    // writing: ArgsRef landed, this block was NOT extended, and the suite
    // went on passing while the PASS line went on claiming ten. The count
    // is asserted below now, because a number in a printf is a note and
    // not a check.
    Arguments body;
    body.entries.push_back({"program", std::make_shared<const Value>(
                                           make_string(encode("satl")))});
    body.index["program"] = 0;
    body.command_line_count = 1;
    auto v_args = std::make_shared<const Value>(make_arguments(body));
    Reg r_args = Reg::from_value(v_args);
    check(r_args.is_heap(), "an arguments object is boxed");
    check_str(r_args.to_string(), "program  satl",
              "a boxed arguments object renders");

    // The twelfth. satellite.container.result, appended when Satellite Orbit's
    // answer became its own type -- and the assert above is the reason this
    // block exists at all: it fired on the day ResultRef landed, which is
    // exactly the job it was given after ArgsRef slipped past unnoticed.
    //
    // The canonical index key is spelled out rather than reached for. map_key_of
    // lives behind eval_internal.hpp, which is private to src/evaluator/, and a
    // test of the REGISTER FILE has no business including the evaluator's
    // insides. §8.6 is the rule it restates: a one-byte type tag, then the raw
    // SatChars, never the decoded text.
    SatString why_key = encode("why");
    std::string canonical = "s";
    canonical.append(reinterpret_cast<const char *>(why_key.data()),
                     why_key.size() * sizeof(SatChar));
    MapBody rfields;
    rfields.entries.push_back(
        {std::make_shared<const Value>(make_string(why_key)),
         std::make_shared<const Value>(make_string(encode("found")))});
    rfields.index.emplace(canonical, 0);

    ResultBody rbody;
    rbody.fields = std::make_shared<const Value>(make_map(std::move(rfields)));
    auto v_result = std::make_shared<const Value>(make_result(std::move(rbody)));
    Reg r_result = Reg::from_value(v_result);
    check(r_result.is_heap(), "a result is boxed");
    check_str(r_result.to_string(), "{why: found}",
              "a boxed result renders as its fields");

    // Out again, by value.
    for (const auto &v : {v_nil, v_true, v_small, v_huge, v_str, v_list,
                          v_map, v_time, v_bits, v_args, v_result}) {
        ValuePtr back = Reg::from_value(v).to_value();
        check(back != nullptr, "a set slot converts back to a Value");
        if (back)
            check_str(to_string(*back), to_string(*v),
                      "the round trip preserves the value");
    }
    check(Reg().to_value() == nullptr,
          "an EMPTY slot converts back to no Value at all");
}
