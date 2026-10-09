#pragma once
// satellite/machine/filesystems.hpp -- what the machine says about its filesystems:
// which ones store nothing, where a mount begins, how much memory there is, and
// which drives are mounted (the author, 2026-09-25: "satellite.directory.system()
// is space attached to the machine", and list() must not walk from one drive into
// the next or call /proc 128 TB).
//
// HEADER-ONLY, because satellite.directory.system()'s library reads the drives too,
// and a library is compiled alone (build_libraries.py): what it shares with satl
// has to be inline.
//
// A FILESYSTEM THAT STORES NOTHING is one the kernel makes up as it is read -- /proc,
// /sys, the cgroup and debug trees. A file there has no size of its own: ls says 0
// for most of /proc and 4096 for every attribute in /sys, and /sys's PCI resource
// files give the size of a device's memory window. None of that is on any disk, so
// a directory there adds nothing up (listing_counts.hpp). tmpfs and devtmpfs are
// NOT in the list: what is in them is real, and held in memory.
//
// A MOUNTED DRIVE is a mount whose source is a block device -- /dev/sda1,
// /dev/mapper/...-home, /dev/nvme0n1p2 -- listed once however many places it is
// mounted, at the first place /proc/self/mountinfo names it.

#include <cstdio>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/vfs.h>
#include <vector>

#ifndef STATX_ATTR_MOUNT_ROOT
#define STATX_ATTR_MOUNT_ROOT 0x00002000
#endif

namespace satellite004 {
namespace filesystems {

inline constexpr unsigned long long int proc_type = 0x9fa0;

inline bool stores_nothing(unsigned long long int type)
{
    switch (type) {
    case proc_type:        // proc
    case 0x62656572:       // sysfs
    case 0x27e0eb:         // cgroup
    case 0x63677270:       // cgroup2
    case 0x64626720:       // debugfs
    case 0x74726163:       // tracefs
    case 0x73636673:       // securityfs
    case 0xcafe4a11:       // bpf
    case 0x6165676C:       // pstore
    case 0x1cd1:           // devpts
    case 0x42494e4d:       // binfmt_misc
    case 0x6e736673:       // nsfs
    case 0xf97cff8c:       // selinuxfs
    case 0x62656570:       // configfs
    case 0x65735543:       // fusectl
    case 0x19800202:       // mqueue
        return true;
    default:
        return false;
    }
}

// Whether `name` inside the directory `at` (AT_FDCWD for a path, and "" for `at`
// itself) is the top of a mount -- another filesystem, or a bind mount of this one.
// AT_NO_AUTOMOUNT, so asking never mounts anything. Never an error: a name statx
// cannot answer for is not called one.
inline bool is_a_mount_top(int at, const char *name)
{
    struct statx about;
    const int flags = AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT | (name[0] == '\0' ? AT_EMPTY_PATH : 0);
    return ::statx(at, name, flags, 0, &about) == 0 &&
           (about.stx_attributes_mask & STATX_ATTR_MOUNT_ROOT) != 0 &&
           (about.stx_attributes & STATX_ATTR_MOUNT_ROOT) != 0;
}

// THE MEMORY THE KERNEL HAS, which is what /proc/kcore holds. Its st_size is the
// kernel's whole address space -- 128 TiB on x86-64 -- because that is the range a
// debugger may read through it; what is there to read is this.
inline unsigned long long int memory_bytes()
{
    struct sysinfo about;
    if (::sysinfo(&about) != 0)
        return 0;
    return static_cast<unsigned long long int>(about.totalram) * about.mem_unit;
}

struct MountedDrive {
    std::string point;    // where it is mounted
    std::string source;   // the device, as mountinfo names it
    std::string type;     // ext4, xfs, vfat ...
    unsigned int major = 0, minor = 0;
};

// mountinfo writes a space, tab, newline and backslash in a path as \040 \011 \012
// \134, so that a line is always its fields.
inline std::string unescaped(const std::string &field)
{
    std::string out;
    for (std::size_t at = 0; at < field.size(); ++at) {
        if (field[at] == '\\' && at + 3 < field.size() && field[at + 1] >= '0' && field[at + 1] <= '3' &&
            field[at + 2] >= '0' && field[at + 2] <= '7' && field[at + 3] >= '0' && field[at + 3] <= '7') {
            out += static_cast<char>((field[at + 1] - '0') * 64 + (field[at + 2] - '0') * 8 + (field[at + 3] - '0'));
            at += 3;
        } else {
            out += field[at];
        }
    }
    return out;
}

// Every mounted drive, in the order the kernel mounted them. A line of mountinfo is
// "id parent major:minor root point options [optional fields] - type source options".
inline std::vector<MountedDrive> mounted_drives()
{
    std::vector<MountedDrive> drives;
    std::FILE *const file = std::fopen("/proc/self/mountinfo", "re");
    if (file == nullptr)
        return drives;
    std::string line;
    int c;
    for (;;) {
        line.clear();
        while ((c = std::fgetc(file)) != EOF && c != '\n')
            line += static_cast<char>(c);
        if (line.empty() && c == EOF)
            break;
        std::vector<std::string> fields;
        std::size_t start = 0;
        while (start < line.size()) {
            const std::size_t end = line.find(' ', start);
            fields.push_back(line.substr(start, end == std::string::npos ? std::string::npos : end - start));
            if (end == std::string::npos)
                break;
            start = end + 1;
        }
        std::size_t dash = 6;
        while (dash < fields.size() && fields[dash] != "-")
            ++dash;
        if (fields.size() < 5 || dash + 2 >= fields.size())
            continue;
        MountedDrive drive;
        if (std::sscanf(fields[2].c_str(), "%u:%u", &drive.major, &drive.minor) != 2)
            continue;
        drive.point = unescaped(fields[4]);
        drive.type = fields[dash + 1];
        drive.source = unescaped(fields[dash + 2]);
        if (drive.source.compare(0, 5, "/dev/") != 0)
            continue;
        bool seen = false;
        for (const MountedDrive &earlier : drives)
            seen = seen || (earlier.major == drive.major && earlier.minor == drive.minor);
        if (!seen)
            drives.push_back(drive);
        if (c == EOF)
            break;
    }
    std::fclose(file);
    return drives;
}

} // namespace filesystems
} // namespace satellite004
