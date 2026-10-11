#pragma once
// `satl --feedback` -- SHOW WHAT IS IN THE BOOK, AND SEND NOTHING.
//
// The author, 2026-09-18: *"I do not want to spy on the users... the users code
// and what they are doing on their machine is their own business"*.
//
// SO THIS PRINTS AND STOPS. There is no network code in satl -- not here, not in
// the word, not anywhere -- and that is worth a person being able to check for
// themselves rather than believe. `satl --feedback` shows the whole file, and
// the file is plain text they can open in any editor.
//
// THAT IS ALSO WHY THE FLOOD THE AUTHOR ASKED ABOUT CANNOT HAPPEN. A program
// that calls satellite.feedback in a loop fills a bounded local file; reaching
// anybody's server is something a PERSON does deliberately, with this in front
// of them, seeing exactly what would go.

#include "config_file.hpp"
#include "../../satellite-numbers/feedback_book.hpp"
#include "../machine/machine_codes.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace satellite004 {

inline signed long long int run_feedback()
{
    const std::string path = feedback_book::book_path();
    std::cout << "satl --feedback: what satellite.feedback has kept on this machine\n\n";

    if (path.empty()) {
        std::cout << "    $HOME is not set, so there is nowhere to keep it.\n";
        return success;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "    nothing yet -- " << path << " is not there.\n\n";
        std::cout << "    A program writes to it with satellite.feedback(\"...\"), and nothing\n";
        std::cout << "    else does. It holds what you typed and the time you typed it.\n";
        return success;
    }

    std::string line;
    unsigned long long int rows = 0;
    while (std::getline(file, line)) {
        std::cout << "    " << line << "\n";
        ++rows;
    }
    std::cout << "\n    " << rows << (rows == 1 ? " entry, in " : " entries, in ") << path << "\n";

    // WHAT IS NOT IN IT, SAID OUT LOUD. A person deciding whether to send this
    // should not have to take anybody's word for what it contains -- but they
    // also should not have to audit it to learn what satl never put there.
    std::cout << "\n    It holds what you typed and when. It does not hold your name, your\n";
    std::cout << "    files, your directories, your machine's name or any of your code --\n";
    std::cout << "    satellite.feedback is handed a string and can see nothing else.\n";
    std::cout << "\n    satl sends nothing, ever. This file is yours; send it if you want to.\n";
    return success;
}

} // namespace satellite004
