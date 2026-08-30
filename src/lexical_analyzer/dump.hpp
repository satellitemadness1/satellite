#pragma once

// What `satl --tokens` prints.
//
// THE LEXER GETS A CONSUMER IN THE MILESTONE THAT WRITES IT, which is the same
// rule PLAN M2 made for the registry and for the same reason: the first
// satellite shipped three commits where its word registry had no reader, and
// four defects accumulated behind a guarantee nothing was checking. M3 ends
// with a vector<Token> that no parser exists to read until M4, so without this
// the whole milestone would be a module whose output nobody can look at.
//
// AND IT IS NOT A TOY. Two of the things the lexer decides are invisible in a
// program's text and visible here: which of a program's words the LANGUAGE owns
// (DESIGN §5.6), and where a token's line is, which DESIGN §6.2's same-line
// postfix rule turns into a parse decision at M4.
//
// SEPARATE FROM lexer.cpp BECAUSE IT IS THE ONLY PART THAT PRINTS, exactly as
// satellite_words/dump.cpp is split from the headers around it.

#include <string>

namespace satellite {

// Every token in `source`, one per line, with its line, its byte span and its
// spelling. `clean` comes back false when the stream holds an Error token.
//
// `clean` IS AN OUT-PARAMETER RATHER THAN SOMETHING THE CALLER READS OUT OF THE
// TEXT, which is the lesson satellite_words/dump.hpp records: a caller that
// decides by searching the returned string for a word it happens to contain
// turns a formatting change into a behaviour change, silently.
std::string tokens_text(const std::string &source, bool &clean);

} // namespace satellite
