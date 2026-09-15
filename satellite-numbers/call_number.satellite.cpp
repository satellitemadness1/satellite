// The vector number index -- the author's call_number.satellite.cpp.
//
// Their first version built the vector with 1,000,000 push_backs of empty
// maps "to hold enough space". reserve() gives the same room without making a
// million empty maps; here it reserves exactly the number of libraries found.

#include "call_number.hpp"

#include "../machine_codes.hpp"
#include "../machine_state.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <dirent.h>
#include <dlfcn.h>

namespace satellite004 {

std::string numbers_text(const std::vector<unsigned long long int> &numbers)
{
    std::string text;
    for (size_t i = 0; i < numbers.size(); i++) {
        if (i > 0)
            text += ' ';
        text += std::to_string(numbers[i]);
    }
    return text;
}

signed long long int NumberIndex::load(const std::string &folder, MachineState &state)
{
    state.set("vector.number.index(loading)", success);

    DIR *directory = opendir(folder.c_str());
    if (directory == nullptr)
        return report_error("vector.number.index(error): cannot open " + folder, vector_loading_error);

    // Sorted, so the index is built in the same order on every run.
    std::vector<std::string> files;
    while (const dirent *entry = readdir(directory)) {
        const std::string file = entry->d_name;
        if (file.size() > 3 && file.compare(file.size() - 3, 3, ".so") == 0)
            files.push_back(folder + "/" + file);
    }
    closedir(directory);
    std::sort(files.begin(), files.end());

    rows_.clear();
    rows_.reserve(files.size());

    for (const std::string &file : files) {
        // RTLD_NOW: resolve everything now, so a broken library fails at
        // start-up instead of in the middle of a program.
        void *library = dlopen(file.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (library == nullptr)
            return report_error(std::string("vector.number.index(error): ") + dlerror(), vector_loading_error);

        const auto describe = reinterpret_cast<DescribeFunction>(dlsym(library, kDescribeSymbol));
        if (describe == nullptr)
            return report_error("vector.number.index(error): " + file + " has no " + kDescribeSymbol,
                                vector_loading_error);

        LibraryRow described;
        if (describe(&described) != success || described.name == nullptr || described.depth == 0 ||
            described.depth > kMaxDepth)
            return report_error("vector.number.index(error): " + file + " described itself wrongly",
                                vector_loading_error);

        NumberRow row;
        row.name = described.name;
        row.numbers.assign(described.numbers, described.numbers + described.depth);
        row.scenarios = described.scenarios;
        row.file = file;

        if (find(row.name) != nullptr)
            return report_error("vector.number.index(error): " + row.name + " is loaded twice",
                                vector_loading_error);
        for (const NumberRow &other : rows_)
            if (other.numbers == row.numbers)
                return report_error("vector.number.index(error): " + row.name + " and " + other.name +
                                        " both claim " + numbers_text(row.numbers),
                                    vector_loading_error);

        rows_.push_back(std::move(row));
        state.set("vector.number.index(loaded " + rows_.back().name + " " +
                      numbers_text(rows_.back().numbers) + ")",
                  success);
    }

    return state.set("vector.number.index(defined)", number_vector_defined);
}

const NumberRow *NumberIndex::find(const std::string &name) const
{
    for (const NumberRow &row : rows_)
        if (row.name == name)
            return &row;
    return nullptr;
}

} // namespace satellite004
