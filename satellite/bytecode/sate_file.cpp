// satellite/bytecode/sate_file.cpp -- the header says why there is one file now
// and not three, and why the format is sixteen binary digits.

#include "sate_file.hpp"

#include "../machine/machine_codes.hpp"

#include <fstream>

namespace satellite004 {

std::string sate_path_of(const std::string &main_file)
{
    // Only the LAST dot, and only if it is in the last segment -- so
    // `dir.of.things/program` gains an extension rather than losing a directory.
    const std::size_t slash = main_file.find_last_of('/');
    const std::size_t dot = main_file.find_last_of('.');
    const bool has_extension = dot != std::string::npos &&
                               (slash == std::string::npos || dot > slash);
    return (has_extension ? main_file.substr(0, dot) : main_file) + ".sate";
}

signed long long int write_sate_file(const std::string &path,
                                     const BytecodeRegistry &registry,
                                     const BytecodeFilenames &filenames,
                                     MachineState &state)
{
    std::ofstream out(path);
    if (!out)
        return report_error("satl(sate): cannot write " + path, display_error);

    unsigned long long int written = 0;
    for (std::size_t r = 0; r < registry.size(); ++r) {
        // ONE ROW A FILE, so the row's own name is what separates them. A reader
        // that only wants the codes skips every line beginning with `>`.
        out << '>' << (r < filenames.size() ? filenames[r] : std::string("?")) << '\n';
        for (const std::bitset<16> &code : registry[r]) {
            out << code.to_string() << '\n';
            ++written;
        }
    }

    // CHECKED AT THE FLUSH, not per line. A full disk succeeds line by line and
    // fails once at the end, so without this a program would exit 0 having
    // written nothing -- which is a trap satellite.console.display fell into
    // once already and check.sh now asserts against.
    out.flush();
    if (!out)
        return report_error("satl(sate): the write to " + path + " was refused", display_error);

    state.set("sate(written): " + path + ", " + std::to_string(registry.size()) + " files, " +
                  std::to_string(written) + " codes",
              success);
    return success;
}

} // namespace satellite004
