// satellite-numbers/words/1.10.2.cpp -- satellite.spacesuit.reference(), built into satl
//
// satellite.spacesuit.reference()   1 10 2
// An exact deep copy of the object -- its fields and every object it holds, copied all the way down --
// its own from then on (satellite_object/object_copy.hpp). Through a pointer it copies the pointer's
// object, and stops with S260 OBJECT_IS_GONE when that object is gone.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we have to give everything a number and then make it
// run those numbers"; and that day, his A: .reference() becomes .satellite.reference()):
// `a.satellite.reference()` reaches the function below through the function table --
// bytecode/pointer_calls.cpp's run_object_number, which has refused anything in its brackets first -- and
// the code in it is what every_object_method ran for reference() until that day, moved here whole.
// The word is new that day: every object has had it since 2026-10-02, with no number until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/pointer_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/satellite_object/object_copy.hpp"

#include <utility>

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &name, ExpressionContext &context)
{
    const UserDefinedHandle object = object_behind(receiver);
    // reference(): an exact copy -- of a pointer's object, which must still be there to be copied.
    if (object == nullptr) {
        context.refuse(object_is_gone, object_is_gone_because(name + ".satellite.reference()", name));
        return Value();
    }
    UserDefinedHandle copy;
    const signed long long int copied = exact_copy_of(object, copy);
    if (copied != success) {
        context.refuse(copied, name + ".satellite.reference() waited for a lock it could never be given, to read "
                                      "the object it copies");
        return Value();
    }
    return Value(std::move(copy));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_10_2(LibraryRow *row)
{
    row->name = "satellite.spacesuit.reference()";
    row->numbers[0] = 1;
    row->numbers[1] = 10;
    row->numbers[2] = 2;
    row->depth = 3;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
