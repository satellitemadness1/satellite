// satellite/bytecode/pointer_calls.cpp -- the header says what every object answers.

#include "pointer_calls.hpp"

#include "../satellite_object/object_copy.hpp"
#include "../satellite_object/satellite_spacesuit.hpp"

#include <utility>

namespace satellite004 {

std::string object_is_gone_because(const std::string &written, const std::string &receiver)
{
    return written + " -- " + receiver + " points at an object that is gone: nothing else held it, and a "
                                         "pointer does not keep its object living";
}

Value every_object_method(const std::vector<std::bitset<16>> &row, std::size_t &at, token::Code method,
                          const Value &value, const std::string &receiver, std::size_t dot,
                          ExpressionContext &context)
{
    const auto code_here = [&row](std::size_t k) {
        return k < row.size() ? static_cast<token::Code>(row[k].to_ulong()) : token::Code(0);
    };
    const std::string written = receiver + "." + token::method_name_of(method);
    if (code_here(at) != token::left_parenthesis_token || code_here(at + 1) != token::right_parenthesis_token) {
        context.refuse(satl_line_not_understood, written + "() takes nothing, in its brackets", dot);
        return Value();
    }
    at += 2;
    const ObjectPointer *pointer = value.as_pointer();
    const UserDefinedHandle object = object_behind(value);

    // ok(): is there an object here. A pointer whose object is gone is the one way to say no.
    if (method == token::ok_token)
        return Value(object != nullptr);

    // pointer(): at the same object, and a pointer's pointer is the pointer itself -- gone or not.
    if (method == token::pointer_token) {
        if (pointer != nullptr)
            return Value(*pointer);
        return Value(ObjectPointer{object, object->layout});
    }

    // reference(): an exact copy -- of a pointer's object, which must still be there to be copied.
    if (object == nullptr) {
        context.refuse(object_is_gone, object_is_gone_because(written + "()", receiver), dot);
        return Value();
    }
    UserDefinedHandle copy;
    const signed long long int copied = exact_copy_of(object, copy);
    if (copied != success) {
        context.refuse(copied, written + "() waited for a lock it could never be given, to read the object it "
                                         "copies",
                       dot);
        return Value();
    }
    return Value(std::move(copy));
}

} // namespace satellite004
