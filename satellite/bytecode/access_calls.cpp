// satellite/bytecode/access_calls.cpp -- satellite.access(name). access_calls.hpp says
// what it shows and why each line is worked out from the declaration.

#include "access_calls.hpp"

#include "access_words.hpp"
#include "capsule_scopes.hpp"
#include "suit_layout.hpp"
#include "type_shape.hpp"
#include "word_codes.hpp"
#include "../machine/console_lock.hpp"
#include "../satellite_object/satellite_index.hpp"
#include "../satellite_object/satellite_list.hpp"

#include <algorithm>
#include <iostream>

namespace satellite004 {

using namespace access_words;

namespace {

using token::Code;

Code code_here(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<Code>(row[at].to_ulong()) : token::end_of_file_token;
}

const TypeShape kAnything;                   // a level declared with no <>: anything at all

const char kTakesAName[] = "satellite.access(name) takes the name of something declared, and shows what it "
                           "is, what it holds and how to reach into it -- satellite.access(rows)";
struct Line {
    std::string left, right;
};

struct Walk {
    std::vector<Line> reach, fill, count;
    std::size_t positions = 0, keys = 0;
    const CapsuleTable *capsules = nullptr;
};

// A SPACESUIT'S PUBLIC CAPSULES, its supertypes' first and a replaced one once, in the
// order they are declared: `path.call_x(a, b)`, and what each argument is.
void capsule_lines(std::size_t suit, const std::string &path, const std::string &tail, Walk &walk)
{
    if (walk.capsules == nullptr || suit >= walk.capsules->scopes.size() ||
        walk.capsules->scopes[suit].layout == nullptr)
        return;
    const satelliteSuitLayout &layout = *walk.capsules->scopes[suit].layout;
    std::vector<std::string> order;
    std::unordered_map<std::string, std::size_t> site_of;
    std::vector<std::size_t> lineage(layout.lineage.rbegin(), layout.lineage.rend());
    if (lineage.empty() || lineage.back() != suit) lineage.push_back(suit);
    for (const std::size_t each : lineage) {
        if (each >= walk.capsules->scopes.size()) continue;
        std::vector<std::pair<std::size_t, std::string>> declared;
        for (const auto &capsule : walk.capsules->scopes[each].capsules) declared.emplace_back(capsule.second, capsule.first);
        std::sort(declared.begin(), declared.end());
        for (const auto &capsule : declared) {
            const CapsuleSite &site = walk.capsules->sites[capsule.first];
            if (!site.is_public || site.constructor) continue;
            if (site_of.count(capsule.second) == 0) order.push_back(capsule.second);
            site_of[capsule.second] = capsule.first;
        }
    }
    for (const std::string &name : order) {
        const CapsuleSite &site = walk.capsules->sites[site_of[name]];
        std::string written, about;
        for (const CapsuleParameter &parameter : site.parameters) {
            written += (written.empty() ? "" : ", ") + parameter.name;
            about += (about.empty() ? "" : ", ") + parameter.name + " " + with_article(noun(parameter.shape, false));
        }
        walk.reach.push_back({path + "." + name + "(" + written + ")",
                              (about.empty() ? std::string("takes nothing") : about) + tail});
    }
}

// EVERY LEVEL UNDER `path`, one reach line each, and how to fill it. `value` is what the
// top level holds now, for how many there are; below the top the declaration alone speaks,
// since each item may hold a different number.
void reach_into(const TypeShape &declared, const std::string &path, const std::string &when, const Value *value,
                bool top, Walk &walk)
{
    const TypeShape &shape = as_reached(declared);
    const std::string tail = when.empty() ? std::string() : " -- " + when;

    if (shape.is_a_suit()) {
        capsule_lines(shape.suit, path, tail, walk);
        return;
    }
    // A MULTIPLE ADDS NO LEVEL: each of its types that can be reached into is, marked with
    // which one the name must be holding for the line to work.
    if (a_multiple(shape)) {
        // A multiple among its types is walked as more of them: its own arms say which.
        for (const TypeShape &arm : shape.parameters) {
            if (!reached_into(arm)) continue;
            if (a_multiple(arm)) {
                reach_into(arm, path, when, value, top, walk);
                continue;
            }
            const bool holds_it = value != nullptr && &arm_holding(shape, *value) == &arm;
            reach_into(arm, path, when.empty() ? "when " + path + " holds " + with_article(short_noun(arm, false)) : when,
                       holds_it ? value : nullptr, top, walk);
        }
        return;
    }
    if (a_list(shape)) {
        const TypeShape &item = shape.parameters.empty() ? kAnything : shape.parameters[0];
        const std::string letter = position_letter(walk.positions++);
        const std::string reached = path + "[" + letter + "]";
        std::string about = with_article(noun(item, false)) + ", " + letter + " counting from 1";
        const ListHandle *held = value != nullptr ? value->as_list() : nullptr;
        if (held != nullptr && *held != nullptr && !(*held)->items.empty())
            about += " (1 to " + std::to_string((*held)->items.size()) + ")";
        walk.reach.push_back({reached, about + tail});
        const std::string example = example_of(item);
        walk.fill.push_back({path + ".append(" + (example.empty() ? "x" : example) + ")",
                             "adds " + with_article(short_noun(example_arm(item), false)) +
                                 (example.empty() ? ", x" : "") + tail});
        if (top) walk.count.push_back({path + ".size", "how many " + short_noun(item, true) + " it holds" + tail});
        if (reached_into(item)) reach_into(item, reached, when, nullptr, false, walk);
        return;
    }
    if (is_an_index_word(shape.word)) {
        const TypeShape &key = shape.parameters.empty() ? kAnything : shape.parameters[0];
        const TypeShape &item = shape.parameters.size() > 1 ? shape.parameters[1] : kAnything;
        const std::string placeholder = key_placeholder(key, walk.keys++);
        const std::string reached = path + "[" + placeholder + "]";
        std::string about = with_article(noun(item, false)) + ", " + placeholder + " " +
                            (key.word == 0 ? std::string("any number, string, bool, binary or percentage")
                                           : with_article(noun(key, false)));
        // THE KEYS IT HOLDS NOW, at the top: a few, so a person sees what to write.
        const IndexHandle *held = value != nullptr ? value->as_index() : nullptr;
        if (held != nullptr && *held != nullptr && !(*held)->entries.empty()) {
            const std::size_t shown_keys = std::min<std::size_t>(4, (*held)->entries.size());
            about += ", one of ";
            for (std::size_t at = 0; at < shown_keys; ++at)
                about += (at == 0 ? "" : ", ") + value_shown((*held)->entries[at].first);
            if ((*held)->entries.size() > shown_keys)
                about += " and " + std::to_string((*held)->entries.size() - shown_keys) + " more";
        }
        walk.reach.push_back({reached, about + tail});
        const std::string example = example_of(item);
        walk.fill.push_back({reached + " = " + (example.empty() ? "x" : example),
                             "puts " + with_article(short_noun(example_arm(item), false)) + (example.empty() ? ", x," : "") +
                                 " under " +
                                 placeholder + tail});
        if (top) walk.count.push_back({path + ".size", "how many keys it holds" + tail});
        walk.count.push_back({path + ".keys",
                              "the keys of " + (top ? std::string("it") : with_article(last_part(shape.word))) +
                                  ", as a list" + tail});
        if (reached_into(item)) reach_into(item, reached, when, nullptr, false, walk);
    }
}

// THE LINES IN TWO COLUMNS: what to write, then what it reaches.
void lay_out(const std::vector<Line> &lines, std::size_t width, std::string &text)
{
    for (const Line &line : lines) {
        text += "\n  " + line.left;
        text += std::string(line.left.size() < width ? width - line.left.size() : 0, ' ') + "  " + line.right;
    }
}

} // namespace

bool is_access_word(Code code)
{
    return code == word::fixed_code<1, 31> || code == word::fixed_code<1, 31, 1>;
}

std::string access_refused(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    if (code_here(row, at + 1) != token::left_parenthesis_token) return kTakesAName;
    std::size_t k = at + 2;
    if (code_here(row, k) != token::name_token) return kTakesAName;
    text_at(row, k);
    return code_here(row, k) == token::right_parenthesis_token ? std::string() : std::string(kTakesAName);
}

std::string access_text(const std::string &name, const TypeShape &shape, const Value &value,
                        const CapsuleTable *capsules)
{
    // WHAT IT IS, and how many it holds now.
    const TypeShape &reached = as_reached(shape);
    std::string text = name + " is ";
    if (shape.word == word::code_of(1, 6, 22)) text += "an info, ";
    text += with_article(noun(shape, false));
    const TypeShape &held_as = arm_holding(reached, value);
    if (a_multiple(reached) && &held_as != &reached && !value.is_nothing())
        text += ", holding " + with_article(noun(held_as, false));
    if (const ListHandle *list = value.as_list()) {
        const std::size_t items = *list != nullptr ? (*list)->items.size() : 0;
        text += ", " + std::to_string(items) + (items == 1 ? " item" : " items");
    } else if (const IndexHandle *index = value.as_index()) {
        const std::size_t keys = *index != nullptr ? (*index)->entries.size() : 0;
        text += ", " + std::to_string(keys) + (keys == 1 ? " key" : " keys");
    }

    Walk walk;
    walk.capsules = capsules;
    std::vector<Line> held;
    // AN OBJECT'S FIELDS, which it holds and only its own capsules reach.
    const UserDefinedHandle *object = value.as_user_defined();
    if (object != nullptr && *object != nullptr && (*object)->layout != nullptr) {
        const satelliteSuitLayout &layout = *(*object)->layout;
        for (std::size_t at = 0; at < layout.fields.size() && at < (*object)->fields.size(); ++at)
            held.push_back({layout.fields[at].name, with_article(noun(layout.fields[at].shape, false)) + ", " +
                                                        value_shown((*object)->fields[at])});
        if (!held.empty())
            held.insert(held.begin(), Line{"its fields", "reached only from inside its own capsules"});
        capsule_lines(layout.suit, name, std::string(), walk);
    } else {
        held.push_back({"value", value_shown(value)});
        reach_into(shape, name, std::string(), &value, true, walk);
    }

    std::size_t width = 0;
    for (const std::vector<Line> *lines : {&held, &walk.reach, &walk.fill, &walk.count})
        for (const Line &line : *lines) width = std::max(width, line.left.size());
    width = std::min<std::size_t>(width, 48);
    lay_out(held, width, text);
    lay_out(walk.reach, width, text);
    lay_out(walk.fill, width, text);
    lay_out(walk.count, width, text);
    return text;
}

Value call_access(const std::vector<std::bitset<16>> &row, std::size_t &at, ExpressionContext &context)
{
    const std::size_t word_at = at - 1;
    const std::size_t open = at;
    ++at;
    if (code_here(row, at) != token::name_token) {
        context.refuse(satl_line_not_understood, kTakesAName, open);
        return Value();
    }
    const std::string name = text_at(row, at);
    if (code_here(row, at) != token::right_parenthesis_token) {
        context.refuse(satl_line_not_understood, kTakesAName, open);
        return Value();
    }
    ++at;
    const Seen seen = context.variables.seen(name);
    if (!seen) {
        context.refuse(name_not_declared, name + " is not a name this capsule can see -- " + kTakesAName, open);
        return Value();
    }
    const std::string text = access_text(name, seen.shape != nullptr ? *seen.shape : plain_shape(seen.declared),
                                         *seen.value, context.state.capsules);

    // A LINE THAT IS NOTHING BUT THIS PRINTS IT (access_calls.hpp).
    const Code before = word_at == 0 ? token::line_end_token : code_here(row, word_at - 1);
    const Code after = code_here(row, at);
    if (before == token::line_end_token &&
        (after == token::line_end_token || after == token::comment_token || after == token::end_of_file_token)) {
        ConsoleHold hold;
        std::cout << text << '\n' << std::flush;
        return Value();
    }
    Value answer;
    std::size_t bad = 0;
    if (Value::of_utf8(text, answer, bad) != success) {
        context.refuse(string_error, "satellite.access(" + name + ") made text that is not text, at byte " +
                                         std::to_string(bad + 1));
        return Value();
    }
    return answer;
}

} // namespace satellite004
