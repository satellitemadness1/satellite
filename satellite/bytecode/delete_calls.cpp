// satellite/bytecode/delete_calls.cpp -- satellite.delete(name)'s shape, and the name taken back from
// what the name held. delete_calls.hpp says what the line deletes and from where; the deleting is the
// walker's (run_delete) and the forgetting the checker's (judge_a_delete), each beside what it changes.

#include "delete_calls.hpp"

#include "bytecode_registry.hpp"
#include "satellite_legal.hpp"
#include "word_codes.hpp"
#include "../satellite_object/satellite_index.hpp"
#include "../satellite_object/satellite_list.hpp"
#include "../satellite_object/satellite_spacesuit.hpp"
#include "../satellite_variable_program/satellite_program.hpp"

#include <mutex>
#include <unordered_map>

namespace satellite004 {
namespace {

using token::Code;

Code code_here(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<Code>(row[at].to_ulong()) : token::end_of_file_token;
}

const char kTakesOneName[] = "satellite.delete(name) takes the name of one variable and nothing else, and deletes "
                             "it -- satellite.delete(rows)";

} // namespace

const char kDeleteIsALineOfItsOwn[] = "satellite.delete(name) is a line of its own -- it deletes the name and "
                                      "answers nothing, so nothing can be made of it";

bool is_delete_word(Code code)
{
    return code == word::fixed_code<1, 32> || code == word::fixed_code<1, 32, 1>;
}

std::string the_name_to_delete(const std::vector<std::bitset<16>> &row, std::size_t at, std::size_t &past,
                               std::string &why)
{
    std::size_t k = at + 1;
    // satellite, THE FIRST LEGAL OBJECT OF EVERY PLACE, IS IMMUTABLE (satellite_legal.hpp): never deleted.
    if (code_here(row, k) == token::left_parenthesis_token && code_here(row, k + 1) == word::kFirst) {
        why = kSatelliteIsImmutable;
        return std::string();
    }
    if (code_here(row, k) != token::left_parenthesis_token || code_here(row, k + 1) != token::name_token) {
        why = kTakesOneName;
        return std::string();
    }
    ++k;
    const std::string name = text_at(row, k);
    if (code_here(row, k) != token::right_parenthesis_token) {
        why = kTakesOneName;
        return std::string();
    }
    ++k;
    const Code after = code_here(row, k);
    if (after != token::line_end_token && after != token::comment_token && after != token::end_of_file_token) {
        why = "satellite.delete(" + name + ") is the whole line, and something follows its )";
        return std::string();
    }
    past = k;
    return name;
}

// A STACK AND NOT RECURSION, as files_kept walks (program_walk.cpp): a list can be nested deeper than the
// stack goes, and `seen` walks a list, map or object two names share, or one inside itself, once.
void forget_the_name_in(const satelliteObject &held, const std::string &name)
{
    std::unordered_map<const void *, bool> seen;
    std::vector<const satelliteObject *> waiting{&held};
    while (!waiting.empty()) {
        const satelliteObject *value = waiting.back();
        waiting.pop_back();
        if (const ProgramHandle *program = value->program_handle()) {
            if (*program == nullptr || !seen.emplace(program->get(), true).second) continue;
            satellite_program &each = **program;
            const std::lock_guard<std::mutex> hold(each.lock);
            if (each.name == name || each.name.rfind(name + "[", 0) == 0 || each.name.rfind(name + ".", 0) == 0)
                each.name.clear();
        } else if (const ListHandle *list = value->as_list()) {
            if (*list != nullptr && seen.emplace(list->get(), true).second)
                for (const satelliteObject &item : (*list)->items) waiting.push_back(&item);
        } else if (const IndexHandle *index = value->as_index()) {
            if (*index != nullptr && seen.emplace(index->get(), true).second)
                for (const satelliteMapEntry &entry : (*index)->entries) {
                    waiting.push_back(&entry.key);
                    waiting.push_back(&entry.value);
                }
        } else if (const UserDefinedHandle *object = std::get_if<UserDefinedHandle>(&value->held)) {
            if (*object != nullptr && seen.emplace(object->get(), true).second)
                for (const satelliteObject &field : (*object)->fields) waiting.push_back(&field);
        }
    }
}

} // namespace satellite004
