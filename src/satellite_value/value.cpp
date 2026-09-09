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

#include <string>
#include <utility>
#include <vector>

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
    // THE RUNTIME READS AS A THING AND NOT AS A TYPE PATH, because every
    // sentence that uses this word already names the type it wanted. "a
    // condition is a `satellite.variable.bool` and this one is the satellite
    // runtime" is a sentence; "and this one is satellite" is a word the reader
    // has to reconstruct a meaning for. DESIGN §8's table calls it "the runtime
    // singleton" and that is the phrase this is short for.
    if (value.is_runtime())
        return "the satellite runtime";
    if (value.is_time())
        return "time";
    if (value.is_float())
        return "float";
    if (value.is_list())
        return "list";
    if (value.is_map())
        return "map";
    if (value.is_file())
        return "file";
    if (value.is_binary())
        return "binary";
    return "nothing";
}

bool map_key_of(const Value &value, std::string &out)
{
    // A NUMBER CANONICALISES THROUGH to_string(). That is exact rather than
    // convenient -- v1's finding, kept whole: to_string() renders only from
    // the normalized form, so a.to_string() == b.to_string() exactly when
    // a == b, and 1 and 1.0 are ONE key -- the only answer consistent with
    // `1 == 1.0` being true in the language.
    if (const Number *number = std::get_if<Number>(&value)) {
        out = "n";
        out += number->to_string();
        return true;
    }
    // A STRING CANONICALISES THROUGH ITS RAW CODES AND NEVER THROUGH decode().
    // v1 called this "the sharpest trap in the whole feature": DESIGN §5's
    // live codes expand at decode time, so hashing decoded text would make a
    // key's identity depend on the machine, the user and the current
    // directory -- a key inserted before a directory change would stop being
    // findable after it. Two strings that decode alike are different keys,
    // and that is correct. The one-byte tag is what stops the number 12 and
    // the string "12" being the same key.
    if (const Str *text = std::get_if<Str>(&value)) {
        out = "s";
        if (*text)
            out.append(reinterpret_cast<const char *>((*text)->data()),
                       (*text)->size() * sizeof(SatChar));
        return true;
    }
    return false;
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
    // A PAIR STACK AND NOT A RECURSION, SINCE M16. Two lists are equal when
    // their elements are, which makes equality a walk over depth the user's
    // program chose -- `l == l` a hundred thousand levels deep is a legal
    // question -- and DESIGN §7.5 says no such walk may use the C++ stack.
    // Every pair below either answers false, or settles as equal, or pushes
    // the pairs its answer depends on.
    std::vector<std::pair<const Value *, const Value *>> pending;
    pending.push_back({&left, &right});

    while (!pending.empty()) {
        const auto [at, other] = pending.back();
        pending.pop_back();
        const Value &a = *at;
        const Value &b = *other;

        // THE TWO NUMERIC ARMS COMPARE AS VALUES, AND THAT IS M15's DECISION
        // WITH A RECORD (MILESTONES/M15.md §2). Arithmetic promotes a number
        // into a float exactly -- §8.6's conversion, "exact and always
        // succeeds" -- so `x * 0.85` mixes the arms and `x == 0.85` asked
        // afterwards must not be a condition no program can satisfy. And
        // QUAD.md §3.3's comparators are all `if (a != b) return a > b`:
        // equality and ordering disagreeing across these two arms would
        // quietly break strict weak ordering, which is how a deterministic
        // program stops being one.
        if (const Number *n = std::get_if<Number>(&a)) {
            if (const Flo *f = std::get_if<Flo>(&b)) {
                if (*f && Number::compare(*n, (*f)->to_number()) == 0)
                    continue;
                return false;
            }
        }
        if (const Flo *f = std::get_if<Flo>(&a)) {
            if (const Number *n = std::get_if<Number>(&b)) {
                if (*f && Number::compare((*f)->to_number(), *n) == 0)
                    continue;
                return false;
            }
        }

        // EVERY OTHER PAIR OF DIFFERENT ARMS IS NEVER EQUAL AND THAT IS NOT A
        // SHORTCUT. There is no conversion anywhere else in this language --
        // DESIGN §8.1 refuses `double` at the C++ type level for the same
        // reason one level down -- so `1` and a string holding "1" are two
        // values and the answer is false rather than an error. A comparison
        // that refused would make `==` a thing a program can fail at, which is
        // what a variant type is for (M12) and not what equality is.
        if (a.index() != b.index())
            return false;

        if (const bool *flag = std::get_if<bool>(&a)) {
            if (*flag != std::get<bool>(b))
                return false;
            continue;
        }

        if (const Number *number = std::get_if<Number>(&a)) {
            if (!(*number == std::get<Number>(b)))
                return false;
            continue;
        }

        // TWO INSTANTS ARE THE SAME INSTANT WHEN THE COUNTS MATCH, and the arm
        // has to be written out: the both-empty tail below answers true, so
        // leaving `Time` to fall through would make every instant equal every
        // other -- exactly the silent fallthrough the file note promises the
        // arms refuse.
        if (const Time *when = std::get_if<Time>(&a)) {
            if (when->ns != std::get<Time>(b).ns)
                return false;
            continue;
        }

        if (const Flo *value = std::get_if<Flo>(&a)) {
            const Flo &twin = std::get<Flo>(b);
            // BY VALUE AND NOT BY HANDLE, the string arm's rule one row down:
            // two computations landing on 2.5 are two allocations and one
            // value.
            if (value->get() == twin.get())
                continue;
            if (*value && twin && Float::compare(**value, *twin) == 0)
                continue;
            return false;
        }

        // TWO BIT RUNS ARE EQUAL WHEN THEIR BITS ARE, WIDTH INCLUDED -- M19.5,
        // and the width is not an extra check bolted on: DESIGN §8.5 makes it
        // part of the value, so `b0010 == b10` is FALSE and the vector compare
        // below answers that by itself, two vectors of different length never
        // being equal. Trimming a leading zero anywhere in this module would
        // break it here, which is why nothing does.
        //
        // BY VALUE AND NOT BY HANDLE, the string arm's rule directly below.
        if (const Bin *run = std::get_if<Bin>(&a)) {
            const Bin &twin = std::get<Bin>(b);
            if (run->get() == twin.get())
                continue;
            if (*run && twin && **run == *twin)
                continue;
            return false;
        }

        if (const Str *text = std::get_if<Str>(&a)) {
            const Str &twin = std::get<Str>(b);
            // A STRING IS COMPARED BY ITS CODES AND NOT BY ITS HANDLE. Two
            // literals with the same body are two allocations, and a language
            // where that made them unequal would be one where equality
            // depended on how the compiler happened to share.
            if (text->get() == twin.get())
                continue;
            if (*text && twin && **text == *twin)
                continue;
            return false;
        }

        // TWO LISTS ARE EQUAL ELEMENTWISE, IN ORDER -- a list IS its order.
        // The handle fast path first: a list handed around is one body seen
        // from two slots, and comparing it against itself must not walk it.
        if (const Lst *handle = std::get_if<Lst>(&a)) {
            if (handle->get() == std::get<Lst>(b).get())
                continue;
            const List *la = as_list(a);
            const List *lb = as_list(b);
            if (la->size() != lb->size())
                return false;
            for (size_t i = 0; i < la->size(); i++)
                pending.push_back({&(*la)[i], &(*lb)[i]});
            continue;
        }

        // TWO MAPS ARE EQUAL ORDER-INSENSITIVELY, which is deliberate and is
        // the one place a map's insertion order does not count -- v1's rule,
        // kept with its reason: order is how a map is PRINTED and WALKED,
        // because those need to be deterministic; it is not part of what a map
        // IS. Two symbol tables that disagree only about which name was seen
        // first hold the same symbols. Looked up through b's index, so this is
        // O(n) rather than O(n^2).
        if (const Map *handle = std::get_if<Map>(&a)) {
            if (handle->get() == std::get<Map>(b).get())
                continue;
            const MapBody *ma = as_map(a);
            const MapBody *mb = as_map(b);
            if (ma->entries.size() != mb->entries.size())
                return false;
            for (const MapEntry &entry : ma->entries) {
                std::string key;
                if (!map_key_of(entry.key, key))
                    return false;
                const auto found = mb->index.find(key);
                if (found == mb->index.end())
                    return false;
                pending.push_back(
                    {&entry.value, &mb->entries[found->second].value});
            }
            continue;
        }

        // TWO FILES ARE EQUAL WHEN THEY ARE THE SAME FILE -- identity, and it
        // is the only answer a reference type has. M19. Every arm above
        // compares what a value HOLDS, because holding the same thing is what
        // being the same value means for a string or a list; a file is not a
        // value in that sense -- DESIGN §8's table calls it a reference type,
        // and two handles on one open file ARE one open file, so closing
        // through either closes both.
        //
        // COMPARING THE PATHS WOULD BE WRONG AND IT IS THE OBVIOUS MISTAKE. Two
        // separate `satellite.file.open` calls on one name are two descriptions
        // with two cursors: reading a line from one does not advance the other,
        // and closing one leaves the other open. Answering true for them would
        // say they were interchangeable, and every one of those behaviours says
        // they are not.
        //
        // AND WITHOUT THIS ARM THEY WOULD ALL BE EQUAL, which is why it is here
        // and not left to the fall-through below: the index check has already
        // proved both sides are files, so any two files would reach the end of
        // the loop and be called the same.
        if (const Fil *handle = std::get_if<Fil>(&a)) {
            if (handle->get() == std::get<Fil>(b).get())
                continue;
            return false;
        }

        // BOTH ARE Nothing OR BOTH ARE Runtime, and in either case they are
        // equal because neither arm holds anything to differ about. There is
        // exactly one runtime -- value.hpp's Runtime note is why the arm
        // carries no payload -- so `satellite == satellite` is true for the
        // same reason nothing equals nothing, and the index check above
        // already separated the two.
    }
    return true;
}

} // namespace satellite
