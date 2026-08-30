// The satellite Handler Dispatch Table implementation.

#include "dispatch.hpp"
#include "evaluator.hpp"
#include "satellite_words/words.hpp"

#include <iostream>

namespace satellite {

namespace {

// Built-in Handler Implementations

Value handler_display(ExecContext &ctx, const std::vector<Value> &args)
{
    std::string text;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) text += " ";
        if (args[i].is_string()) {
            text += decode(*std::get<Str>(args[i]));
        } else {
            text += args[i].to_string();
        }
    }
    ctx.record_output(text);
    return Value::satellite_singleton();
}

Value handler_include(ExecContext &/*ctx*/, const std::vector<Value> &/*args*/)
{
    return Value::satellite_singleton();
}

Value handler_return(ExecContext &/*ctx*/, const std::vector<Value> &args)
{
    if (args.empty()) return Value::nil();
    return args[0];
}

Value handler_bool_true(ExecContext &/*ctx*/, const std::vector<Value> &/*args*/)
{
    return Value::boolean(true);
}

Value handler_bool_false(ExecContext &/*ctx*/, const std::vector<Value> &/*args*/)
{
    return Value::boolean(false);
}

Value handler_list_append(ExecContext &/*ctx*/, const std::vector<Value> &args)
{
    // DESIGN §6.4: my_list.append(item)
    if (args.size() >= 2 && args[0].is_list()) {
        auto list_ref = std::get<ListRef>(args[0]);
        auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
        mutable_list->push_back(std::make_shared<Value>(args[1]));
        return Value(std::const_pointer_cast<const List>(mutable_list));
    }
    return Value::nil();
}

Value handler_list_size(ExecContext &/*ctx*/, const std::vector<Value> &args)
{
    if (!args.empty() && args[0].is_list()) {
        auto list_ref = std::get<ListRef>(args[0]);
        size_t sz = list_ref ? list_ref->size() : 0;
        return Value::number(Number(static_cast<uint64_t>(sz)));
    }
    return Value::number(Number(0));
}

Value handler_string_length(ExecContext &/*ctx*/, const std::vector<Value> &args)
{
    if (!args.empty() && args[0].is_string()) {
        auto s = std::get<Str>(args[0]);
        size_t len = s ? s->length() : 0;
        return Value::number(Number(static_cast<uint64_t>(len)));
    }
    return Value::number(Number(0));
}

Value handler_number_abs(ExecContext &/*ctx*/, const std::vector<Value> &args)
{
    if (!args.empty() && args[0].is_number()) {
        return Value::number(std::get<Number>(args[0]).abs());
    }
    return Value::nil();
}

Value handler_number_negated(ExecContext &/*ctx*/, const std::vector<Value> &args)
{
    if (!args.empty() && args[0].is_number()) {
        return Value::number(std::get<Number>(args[0]).negated());
    }
    return Value::nil();
}

} // namespace

DispatchTable &DispatchTable::instance()
{
    static DispatchTable inst;
    return inst;
}

DispatchTable::DispatchTable()
{
    handlers_.resize(words::kNodeCount + 100);
    init_standard_handlers();
}

void DispatchTable::register_handler(words::PathId path_id, HandlerFn fn, bool binds_receiver,
                                     uint32_t arity, const char *name)
{
    if (path_id >= handlers_.size())
        handlers_.resize(path_id + 64);
    handlers_[path_id] = HandlerEntry{fn, binds_receiver, arity, name};
}

const HandlerEntry *DispatchTable::get(words::PathId path_id) const
{
    if (path_id < handlers_.size() && handlers_[path_id].is_valid())
        return &handlers_[path_id];
    return nullptr;
}

void DispatchTable::init_standard_handlers()
{
    auto register_path = [this](const char *path, HandlerFn fn, bool binds_receiver = false, uint32_t arity = 0) {
        words::Walk w = words::walk(path);
        if (w.error == words::WalkError::NONE && w.id != words::kNoPath) {
            register_handler(w.id, fn, binds_receiver, arity, path);
        }
    };

    // Console
    register_path("satellite.console.display", handler_display, false, 1);

    // Include
    register_path("satellite.include", handler_include, false, 0);
    register_path("satellite.include(satellite)", handler_include, false, 0);

    // Return
    register_path("satellite.return", handler_return, false, 0);
    register_path("satellite.return(satellite)", handler_return, false, 1);
    register_path("satellite.return(value)", handler_return, false, 1);

    // Bool
    register_path("satellite.bool.true", handler_bool_true, false, 0);
    register_path("satellite.bool.false", handler_bool_false, false, 0);

    // List methods (binds_receiver = true)
    register_path("satellite.container.list.append", handler_list_append, true, 2);
    register_path("satellite.container.list.size", handler_list_size, true, 1);

    // String methods (binds_receiver = true)
    register_path("satellite.variable.string.length", handler_string_length, true, 1);

    // Number methods (binds_receiver = true)
    register_path("satellite.variable.number.abs", handler_number_abs, true, 1);
    register_path("satellite.variable.number.negate", handler_number_negated, true, 1);
}

} // namespace satellite
