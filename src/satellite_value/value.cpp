// What a value is called, whether it is true, and whether two are equal. See
// satellite_value/value.hpp.
//
// THREE FUNCTIONS AND NO ARITHMETIC, which is the seam this module is cut on.
// Adding two numbers is satellite_number's and evaluator/ is what asks it;
// what lives here is the part every arm shares -- the word a diagnostic uses,
// the one conversion the language has, and equality. Each of the three is a
// switch over the arms, so an arm appended to the variant without a case is a
// -Wswitch warning under -Wall rather than a silent fallthrough.

#include "satellite_value/value.hpp"

namespace satellite {

const char *type_name(const Value &value)
{
    // THE NAMES ARE THE LANGUAGE'S, NOT C++'s. DESIGN §8's table spells them
    // `satellite.variable.number` and so on; what a sentence wants is the last
    // segment, because the sentence already says which language it is about.
    // "nothing" is DESIGN §6.4 q3's word for the state and is not a type at
    // all, which is why it reads differently from the other three.
    if (value.is_bool())
        return "bool";
    if (value.is_number())
        return "number";
    if (value.is_string())
        return "string";
    return "nothing";
}

bool truth_of(const Value &value, bool *out)
{
    if (const bool *flag = std::get_if<bool>(&value)) {
        *out = *flag;
        return true;
    }
    return false;
}

bool same(const Value &left, const Value &right)
{
    // DIFFERENT ARMS ARE NEVER EQUAL AND THAT IS NOT A SHORTCUT. There is no
    // conversion anywhere in this language -- DESIGN §8.1 refuses `double` at
    // the C++ type level for the same reason one level down -- so `1` and a
    // string holding "1" are two values and the answer is false rather than an
    // error. A comparison that refused would make `==` a thing a program can
    // fail at, which is what a variant type is for (M12) and not what equality
    // is.
    if (left.index() != right.index())
        return false;

    if (const bool *flag = std::get_if<bool>(&left))
        return *flag == std::get<bool>(right);

    if (const Number *number = std::get_if<Number>(&left))
        return *number == std::get<Number>(right);

    if (const Str *text = std::get_if<Str>(&left)) {
        const Str &other = std::get<Str>(right);
        // A STRING IS COMPARED BY ITS CODES AND NOT BY ITS HANDLE. Two
        // literals with the same body are two allocations, and a language where
        // that made them unequal would be one where equality depended on how
        // the compiler happened to share.
        if (text->get() == other.get())
            return true;
        return *text && other && **text == *other;
    }

    return true; // both are Nothing
}

} // namespace satellite
