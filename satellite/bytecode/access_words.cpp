// satellite/bytecode/access_words.cpp -- satellite.access's words for a shape (access_words.hpp).

#include "access_words.hpp"

#include "console_calls.hpp"
#include "word_codes.hpp"
#include "../machine/shown.hpp"
#include "../satellite_object/satellite_index.hpp"
#include "../satellite_object/satellite_list.hpp"

namespace satellite004 {
namespace access_words {

using token::Code;

namespace {

std::string plural_of(const std::string &one)
{
    if (one == "arguments") return one;
    const char last = one.empty() ? ' ' : one.back();
    const char before = one.size() > 1 ? one[one.size() - 2] : ' ';
    if (last == 'y' && std::string("aeiou").find(before) == std::string::npos) return one.substr(0, one.size() - 1) + "ies";
    if (last == 'x' || last == 's') return one + "es";
    return one + "s";
}

// WHAT display SHOWS FOR ONE VALUE -- and, for a container display has no text for,
// its items one by one, an object as <shelf object>: display refuses a list of objects,
// and access is asked about exactly those.
std::string as_text(const Value &value, bool inside);

} // namespace

// AN INFO IS A LIST OF MAPS, string -> anything (info_calls.hpp), and is reached as one.
const TypeShape &as_reached(const TypeShape &shape)
{
    if (shape.word != word::code_of(1, 6, 22))
        return shape;
    static const TypeShape info = [] {
        TypeShape map = plain_shape(word::code_of(1, 4, 1));
        map.parameters = {plain_shape(word::code_of(1, 6, 1)), TypeShape{}};
        TypeShape list = plain_shape(word::code_of(1, 4, 2));
        list.parameters = {map};
        return list;
    }();
    return info;
}

bool reached_into(const TypeShape &shape)
{
    const TypeShape &reached = as_reached(shape);
    return reached.is_a_suit() || a_list(reached) || is_an_index_word(reached.word) || a_multiple(reached);
}

// THE WORD'S LAST PART, AS WRITTEN: number, string, map, index.
std::string last_part(Code word)
{
    std::string spelled(word::spelling_of(word));
    spelled = spelled.substr(0, spelled.find('('));
    return spelled.substr(spelled.rfind('.') + 1);
}


// WHAT A SHAPE IS, IN WORDS: "list of maps (string -> list of numbers)". `many` for a
// level's items, "lists of numbers".
std::string noun(const TypeShape &declared, bool many)
{
    if (declared.is_a_suit()) return suit_written(declared) + (many ? " objects" : " object");
    if (declared.word == 0) return many ? "values of any type" : "anything";
    const TypeShape &shape = as_reached(declared);
    if (a_list(shape))
        return std::string(many ? "lists" : "list") +
               (shape.parameters.empty() ? " of anything" : " of " + noun(shape.parameters[0], true));
    if (is_an_index_word(shape.word)) {
        const std::string head = many ? plural_of(last_part(shape.word)) : last_part(shape.word);
        if (shape.parameters.size() < 2) return head + " of anything";
        return head + " (" + noun(shape.parameters[0], false) + " -> " + noun(shape.parameters[1], false) + ")";
    }
    if (a_multiple(shape)) {
        if (shape.parameters.empty()) return many ? "values of any type" : "anything";
        std::string arms;
        for (std::size_t at = 0; at < shape.parameters.size(); ++at) {
            if (at != 0) arms += at + 1 == shape.parameters.size() ? " or " : ", ";
            arms += noun(shape.parameters[at], false);
        }
        return std::string(many ? "multiples" : "multiple") + " (" + arms + ")";
    }
    return many ? plural_of(last_part(shape.word)) : last_part(shape.word);
}

// THE SAME, WITHOUT WHAT IS BETWEEN ( AND ): "lists of maps". The reach line above has
// said the rest, so the lines that fill and count a level say only what goes in.
std::string short_noun(const TypeShape &declared, bool many)
{
    const std::string said = noun(declared, many);
    return said.substr(0, said.find(" ("));
}

std::string with_article(const std::string &said)
{
    if (said == "anything") return said;
    return (std::string("aeiou").find(said.empty() ? ' ' : said.front()) == std::string::npos ? "a " : "an ") + said;
}

// THE TYPE OF A MULTIPLE THAT ITS FILL LINE PUTS IN: the first that can be reached into and
// written, since the lines below it reach into that one -- `x["key"] = {1}` and then
// `x["key"].append(1)`, never `x["key"] = "text"` and then an append to a string.
const TypeShape &example_arm(const TypeShape &shape)
{
    if (!a_multiple(shape) || shape.parameters.empty()) return shape;
    for (const TypeShape &arm : shape.parameters)
        if (reached_into(arm) && !example_of(arm).empty()) return example_arm(arm);
    for (const TypeShape &arm : shape.parameters)
        if (!example_of(arm).empty()) return example_arm(arm);
    return shape;
}

// ONE VALUE OF A SHAPE, WRITTEN AS A PROGRAM WOULD WRITE IT, for the lines that fill a
// level -- or "" when there is no literal for it (an object is made by declaring one).
std::string example_of(const TypeShape &declared)
{
    if (declared.is_a_suit()) return "";
    if (declared.word == 0) return "1";
    const TypeShape &shape = as_reached(declared);
    if (a_list(shape)) {
        const std::string item = shape.parameters.empty() ? "1" : example_of(shape.parameters[0]);
        return item.empty() ? "" : "{" + item + "}";
    }
    if (is_an_index_word(shape.word)) {
        const std::string key = shape.parameters.empty() ? "\"key\"" : example_of(shape.parameters[0]);
        const std::string held = shape.parameters.size() < 2 ? "1" : example_of(shape.parameters[1]);
        if (key.empty() || held.empty()) return "";
        return "{" + (key == "\"text\"" ? std::string("\"key\"") : key) + ": " + held + "}";
    }
    if (a_multiple(shape)) {
        if (shape.parameters.empty()) return "1";
        const TypeShape &arm = example_arm(shape);
        return &arm == &shape ? "" : example_of(arm);
    }
    switch (kind_of_type_word(shape.word)) {
    case satelliteObject::number: return "1";
    case satelliteObject::string: return "\"text\"";
    case satelliteObject::floating: return "1.5";
    case satelliteObject::boolean: return "satellite.bool.true";
    case satelliteObject::binary: return "b1010";
    case satelliteObject::percentage: return "50%";
    case satelliteObject::hexadecimal: return "xFF";
    // a colour has no literal a value can be written with outside its own declaration
    // (`= 87ceeb` there, "ceeb has no ... line" anywhere else), so its fill line says x
    case satelliteObject::fraction: return "1/3";
    default: return "";
    }
}

// n, m, p, q ... for the positions, one letter a level, as a person counting would.
std::string position_letter(std::size_t level)
{
    static const char *const letters[] = {"n", "m", "p", "q", "r", "s", "t", "u", "v", "w"};
    return level < 10 ? letters[level] : "n" + std::to_string(level - 8);
}

// "key", "key2" ... for the keys: quoted where the key is a string, bare where it is not.
std::string key_placeholder(const TypeShape &key, std::size_t level)
{
    const std::string counted = level == 0 ? "key" : "key" + std::to_string(level + 1);
    const bool text = key.word == 0 || key.word == word::code_of(1, 6, 1);
    return text ? "\"" + counted + "\"" : counted;
}

namespace {
std::string as_text(const Value &value, bool inside)
{
    std::string text, why;
    if (display_text(value, text, why)) return value.is_string() && inside ? "\"" + text + "\"" : text;
    if (const UserDefinedHandle *object = value.as_user_defined())
        return "<" + (*object != nullptr && (*object)->layout != nullptr ? (*object)->layout->shown : std::string("spacesuit")) +
               " object>";
    if (const ListHandle *list = value.as_list()) {
        text = "{";
        if (*list != nullptr)
            for (std::size_t at = 0; at < (*list)->items.size(); ++at)
                text += (at == 0 ? "" : ", ") + as_text((*list)->items[at], true);
        return text + "}";
    }
    if (const IndexHandle *index = value.as_index()) {
        text = "{";
        if (*index != nullptr)
            for (std::size_t at = 0; at < (*index)->entries.size(); ++at)
                text += (at == 0 ? "" : ", ") + as_text((*index)->entries[at].first, true) + ": " +
                        as_text((*index)->entries[at].second, true);
        return text + "}";
    }
    return std::string("<") + value.kind_name() + ">";
}
} // namespace

// WHAT display SHOWS, with a string in quotes so it is not mistaken for a word, cut short
// past a few lines' worth and every control character shown, never obeyed.
std::string value_shown(const Value &value)
{
    if (value.is_nothing()) return "nothing yet -- it has not been given a value";
    std::string text = as_text(value, true);
    constexpr std::size_t kAtMost = 240;
    if (text.size() > kAtMost) {
        std::size_t cut = kAtMost;
        while (cut > 0 && (static_cast<unsigned char>(text[cut]) & 0xC0) == 0x80) --cut;   // a whole character
        std::size_t characters = 0;                  // counted as characters, not bytes: é is one
        for (const char byte : text) characters += (static_cast<unsigned char>(byte) & 0xC0) != 0x80;
        text = text.substr(0, cut) + " ... (" + std::to_string(characters) + " characters in all)";
    }
    return shown(text);
}

} // namespace access_words
} // namespace satellite004
