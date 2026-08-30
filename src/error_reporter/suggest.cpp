// "Did you mean" over one node's children. See error_reporter/suggest.hpp for
// why the search is one level and not the language.

#include "error_reporter/suggest.hpp"

#include "satellite_words/words.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace satellite::errors {

namespace {

// The longest spelling in the language, plus room. Measured rather than
// guessed: the longest single word in words.def is `arguments` at 9 and the
// longest alias text is `satellite.random.ultra.range(min, max)`'s tail, so 64
// is far above anything the table can hold and the buffer never grows.
//
// A FIXED ROW RATHER THAN A vector, because this runs once per failed segment
// and a heap allocation to compare two short words is the kind of cost that
// only shows up when a program has a hundred mistakes in it.
constexpr size_t kRow = 64;

} // namespace

size_t distance(std::string_view a, std::string_view b)
{
    if (a.size() >= kRow || b.size() >= kRow)
        return kTooFar;
    if (a.empty())
        return b.size();
    if (b.empty())
        return a.size();

    // THREE ROWS AND NOT A MATRIX. Optimal string alignment needs the row
    // before last for the transposition arm and nothing older than that, which
    // is the whole difference between this and a full Damerau-Levenshtein --
    // and the reason this one is a fixed 192 bytes of stack instead of a
    // rectangle whose size depends on the input.
    size_t before_last[kRow];
    size_t last[kRow];
    size_t here[kRow];

    for (size_t j = 0; j <= b.size(); j++)
        last[j] = j;

    for (size_t i = 1; i <= a.size(); i++) {
        here[0] = i;
        size_t best_in_row = here[0];
        for (size_t j = 1; j <= b.size(); j++) {
            const size_t cost = a[i - 1] == b[j - 1] ? 0 : 1;
            here[j] = std::min({last[j] + 1, here[j - 1] + 1, last[j - 1] + cost});
            // THE ARM THAT MAKES THIS WORTH WRITING. `wihle` against `while` is
            // two edits without it and one with it, and a transposition is the
            // commonest typing mistake there is -- suggest.hpp has the numbers.
            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1])
                here[j] = std::min(here[j], before_last[j - 2] + 1);
            best_in_row = std::min(best_in_row, here[j]);
        }
        // Nothing later can beat the best cell in this row, so a word already
        // ruled out is not paid for to the end.
        if (best_in_row >= kTooFar)
            return kTooFar;
        for (size_t j = 0; j <= b.size(); j++) {
            before_last[j] = last[j];
            last[j] = here[j];
        }
    }
    return std::min(last[b.size()], kTooFar);
}

std::string_view suggest(words::PathId under, std::string_view word)
{
    if (word.empty() || !words::is_language_word(under))
        return {};
    const words::NodeId parent = static_cast<words::NodeId>(under);

    std::string_view best;
    size_t best_edits = kTooFar;

    // THE CHILDREN FIRST, IN NUMBER ORDER, so a tie goes to the lower number --
    // which is registration order, which is the order WORD_NUMBERS §2.2 writes
    // them in. An arbitrary tie-break would make the answer depend on how the
    // table happens to be laid out.
    for (words::PathId c = words::first_child(parent); c != words::kNoPath;
         c = words::next_sibling(c)) {
        const std::string_view spelling =
            words::spelling_of(static_cast<words::NodeId>(c));
        // The 38 bare rows and the 6 argument rows have an empty spelling by
        // design (words.def) and are never walked to by name, so they are never
        // what somebody meant to type.
        if (spelling.empty())
            continue;
        const size_t edits = distance(word, spelling);
        if (edits < best_edits) {
            best_edits = edits;
            best = spelling;
        }
    }

    // Then the aliases under the same parent. An alias may carry a dot
    // (WORD_NUMBERS §2.3), and one that does is not a candidate for a single
    // misspelled segment -- `fast.range(min, max)` is two segments and a call
    // shape, and offering it for one wrong word would be advice nobody can act
    // on. The same `find('.')` filter the lexer's spelling table uses.
    for (size_t i = 0; i < words::kAliasCount; i++) {
        if (words::parent_of(words::kAliases[i].of) != parent)
            continue;
        const std::string_view text = words::kAliases[i].text;
        if (text.find('.') != std::string_view::npos)
            continue;
        const size_t edits = distance(word, text);
        if (edits < best_edits) {
            best_edits = edits;
            best = text;
        }
    }

    if (!close_enough(best_edits, std::max(word.size(), best.size())))
        return {};
    return best;
}

} // namespace satellite::errors
