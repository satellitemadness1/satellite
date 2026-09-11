// SMBIOS type 17, the Memory Device record. See system_facts/facts.hpp.
//
// PORTED FROM THE FIRST SATELLITE'S memory_facts.cpp, WHICH M6 DELIBERATELY
// LEFT BEHIND. That file's header names these two by name -- "forty lines of
// DMI offsets whose own header records that /sys/firmware/dmi/entries is mode
// 0400 root, so a run as anybody else gets 0" -- and says they belong to
// satellite.system at M20. This is M20 collecting them.
//
// A FILE OF THEIR OWN AND NOT memory_facts.cpp's, WHICH IS THAT FILE'S OWN RULE
// APPLIED RATHER THAN DEPARTED FROM. "One file because they read one source":
// everything there reads /proc/meminfo through one helper, and nothing here
// touches /proc at all. This reads a binary firmware table out of /sys through
// a directory walk, it is the only thing in the tree that parses a structure by
// byte offset, and the failure it has to handle -- a permission denied that is
// NORMAL -- is one no /proc reader has. Two sources, two files.
//
// WHAT THESE ANSWER WHEN THEY FAIL IS 0, AND 0 IS A TRUTHFUL ANSWER. The
// entries are root-only on every ordinary Linux, so an ordinary user gets 0
// here and the language reports 0. PLAN M20's done-when says that in as many
// words: these two "cannot be demonstrated as an ordinary user", and the
// program that demonstrates them has to SAY that 0 is the truthful answer
// rather than treat it as a failure. There is deliberately no way to tell "the
// firmware would not say" from "you may not look" -- both mean the program has
// no number, and inventing a third state would be inventing information.

#include "system_facts/facts.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

#include <dirent.h>

namespace satellite::facts {

namespace {

// The first POPULATED memory slot, which answers for the machine.
//
// THE OFFSETS ARE THE SPECIFICATION'S OWN and are written as it writes them:
//
//   0x08  Total Width   bits INCLUDING error correction -- 72 on ECC, 64 not
//   0x0A  Data Width    bits carrying data alone -- 64 on both
//   0x0C  Size          0 means the slot is empty
//   0x15  Speed         MT/s; 0 or 0xFFFF mean the firmware would not say
//
// A BOARD WITH MISMATCHED STICKS HAS MORE THAN ONE ANSWER AND THIS RETURNS THE
// FIRST. Reporting per slot would need a list, and a list of one number per
// slot is a different feature from "what is this machine's memory" -- which is
// the question `satellite.system.memory.bit` asks.
bool memory_device(unsigned long *speed_mhz, unsigned *width_bits)
{
    DIR *dir = opendir("/sys/firmware/dmi/entries");
    if (!dir)
        return false;

    bool found = false;
    for (;;) {
        errno = 0;
        const struct dirent *entry = readdir(dir);
        if (!entry)
            break;
        // "17-0", "17-1": the type, a dash, the instance.
        if (std::strncmp(entry->d_name, "17-", 3) != 0)
            continue;

        const std::string path =
            std::string("/sys/firmware/dmi/entries/") + entry->d_name + "/raw";
        std::FILE *file = std::fopen(path.c_str(), "rb");
        if (!file)
            continue;               // root-only, which is the usual answer

        unsigned char raw[64] = {0};
        const size_t got = std::fread(raw, 1, sizeof raw, file);
        std::fclose(file);
        if (got < 0x17)
            continue;

        const unsigned size = static_cast<unsigned>(raw[0x0C]) |
                              (static_cast<unsigned>(raw[0x0D]) << 8);
        if (size == 0)
            continue;               // an empty slot has nothing to report

        const unsigned total_width = static_cast<unsigned>(raw[0x08]) |
                                     (static_cast<unsigned>(raw[0x09]) << 8);
        const unsigned data_width = static_cast<unsigned>(raw[0x0A]) |
                                    (static_cast<unsigned>(raw[0x0B]) << 8);
        const unsigned speed = static_cast<unsigned>(raw[0x15]) |
                               (static_cast<unsigned>(raw[0x16]) << 8);

        // TOTAL WIDTH FIRST AND DATA WIDTH AS THE FALLBACK, because the
        // question is how wide the bus is and ECC parts really are 72 bits
        // wide. 0xFFFF is the specification's "unknown" and is not a width.
        if (width_bits) {
            unsigned bits = total_width;
            if (bits == 0 || bits == 0xFFFF)
                bits = data_width;
            *width_bits = (bits == 0xFFFF) ? 0 : bits;
        }
        if (speed_mhz)
            *speed_mhz = (speed == 0xFFFF) ? 0 : speed;
        found = true;
        break;
    }

    closedir(dir);
    return found;
}

} // namespace

unsigned long mem_frequency_mhz()
{
    unsigned long speed = 0;
    memory_device(&speed, nullptr);
    return speed;
}

unsigned mem_width_bits()
{
    unsigned bits = 0;
    memory_device(nullptr, &bits);
    return bits;
}

} // namespace satellite::facts
