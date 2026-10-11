// The prompt's table of drives. See drives.hpp.

#include "drives.hpp"

#include "listing.hpp"

#include "../machine/filesystems.hpp"
#include "../machine/shown.hpp"

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <sys/statvfs.h>
#include <unistd.h>

namespace satellite004 {

namespace {

struct Space {
    bool known = false;
    unsigned long long int size = 0, used = 0, free = 0;
};

Space space_of(const std::string &point)
{
    Space out;
    struct statvfs about;
    if (::statvfs(point.c_str(), &about) != 0)
        return out;
    const unsigned long long int block = about.f_frsize != 0 ? about.f_frsize : about.f_bsize;
    out.known = true;
    out.size = static_cast<unsigned long long int>(about.f_blocks) * block;
    out.used = static_cast<unsigned long long int>(about.f_blocks - about.f_bfree) * block;
    out.free = static_cast<unsigned long long int>(about.f_bavail) * block;
    return out;
}

std::string real_path(const std::string &path)
{
    char resolved[PATH_MAX];
    return ::realpath(path.c_str(), resolved) != nullptr ? std::string(resolved) : std::string();
}

bool exists(const std::string &path)
{
    return ::access(path.c_str(), F_OK) == 0;
}

std::string first_line_of(const std::string &path)
{
    std::FILE *const file = std::fopen(path.c_str(), "re");
    if (file == nullptr)
        return std::string();
    std::string text;
    int c;
    while ((c = std::fgetc(file)) != EOF && c != '\n')
        text += static_cast<char>(c);
    std::fclose(file);
    return text;
}

std::string trimmed(const std::string &text)
{
    const std::size_t first = text.find_first_not_of(" \t");
    return first == std::string::npos ? std::string() : text.substr(first, text.find_last_not_of(" \t") - first + 1);
}

// THE DISK'S WHOLE NAME, as lsblk shows it: udev keeps it in /run/udev/data/b<major>:<minor>
// as ID_MODEL_ENC, with \x20 for each space. /sys has only the first 16 characters a
// SCSI inquiry holds ("Samsung SSD 870") and, behind some USB bridges, the bridge's
// name instead of the disk's -- so it is the fallback.
std::string model_of(const std::string &disk)
{
    std::FILE *const file = std::fopen(("/run/udev/data/b" + first_line_of(disk + "/dev")).c_str(), "re");
    if (file != nullptr) {
        std::string line, found;
        int c;
        do {
            line.clear();
            while ((c = std::fgetc(file)) != EOF && c != '\n')
                line += static_cast<char>(c);
            if (line.compare(0, 15, "E:ID_MODEL_ENC=") == 0)
                found = line.substr(15);
        } while (c != EOF && found.empty());
        std::fclose(file);
        std::string model;
        for (std::size_t at = 0; at < found.size(); ++at) {
            unsigned int byte = 0;
            if (found[at] == '\\' && at + 3 < found.size() && found[at + 1] == 'x' &&
                std::sscanf(found.c_str() + at + 2, "%2x", &byte) == 1) {
                model += static_cast<char>(byte);
                at += 3;
            } else {
                model += found[at];
            }
        }
        if (!trimmed(model).empty())
            return trimmed(model);
    }
    return trimmed(first_line_of(disk + "/device/model"));
}

std::string leaf_of(const std::string &path)
{
    const std::size_t slash = path.rfind('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

// The disk a block device is on: a partition's disk is the directory above it in
// /sys, and a device-mapper or md device sits on its first slave -- followed down
// until a real disk, a few hops at most (LVM on LUKS on a partition is three).
std::string disk_under(std::string device_in_sys)
{
    for (int hop = 0; hop < 8 && !device_in_sys.empty(); ++hop) {
        if (exists(device_in_sys + "/partition")) {
            device_in_sys = device_in_sys.substr(0, device_in_sys.rfind('/'));
            continue;
        }
        DIR *const slaves = ::opendir((device_in_sys + "/slaves").c_str());
        if (slaves == nullptr)
            return device_in_sys;
        std::string next;
        while (const struct dirent *entry = ::readdir(slaves))
            if (entry->d_name[0] != '.') {
                next = entry->d_name;
                break;
            }
        ::closedir(slaves);
        if (next.empty())
            return device_in_sys;
        device_in_sys = real_path(device_in_sys + "/slaves/" + next);
    }
    return device_in_sys;
}

void device_and_drive(const filesystems::MountedDrive &mounted, std::string &device, std::string &drive)
{
    const std::string in_sys =
        real_path("/sys/dev/block/" + std::to_string(mounted.major) + ":" + std::to_string(mounted.minor));
    device = in_sys.empty() ? leaf_of(mounted.source) : leaf_of(in_sys);
    const std::string disk = disk_under(in_sys);
    drive = disk.empty() ? std::string() : model_of(disk);
    if (drive.empty())
        drive = "-";
    if (disk.find("/usb") != std::string::npos)
        drive += " (usb)";
}

const filesystems::MountedDrive *drive_at(const std::vector<filesystems::MountedDrive> &drives,
                                          const std::string &point)
{
    for (const filesystems::MountedDrive &drive : drives)
        if (drive.point == point)
            return &drive;
    return nullptr;
}

} // namespace

std::vector<std::string> drives_lines(const std::vector<std::string> &points)
{
    unsigned long long int size = 0, free = 0;
    for (const std::string &point : points) {
        const Space space = space_of(point);
        size += space.size;
        free += space.free;
    }
    return {"SPACE ON ALL DRIVES: " + megabytes_with_commas(size),
            "FREE SPACE ON ALL DRIVES: " + megabytes_with_commas(free)};
}

std::string drives_table(const std::vector<std::string> &points)
{
    const std::vector<filesystems::MountedDrive> drives = filesystems::mounted_drives();
    std::vector<std::vector<std::string>> rows;
    rows.push_back({"mounted on", "type", "size", "used", "free", "device", "drive"});
    for (const std::string &point : points) {
        const filesystems::MountedDrive *const mounted = drive_at(drives, point);
        const Space space = space_of(point);
        std::string device = "-", drive = "-";
        if (mounted != nullptr)
            device_and_drive(*mounted, device, drive);
        rows.push_back({shown(point), mounted != nullptr ? shown(mounted->type) : std::string("-"),
                        space.known ? megabytes_with_commas(space.size) : std::string("-"),
                        space.known ? megabytes_with_commas(space.used) : std::string("-"),
                        space.known ? megabytes_with_commas(space.free) : std::string("-"), shown(device),
                        shown(drive)});
    }
    return lined_up(rows, {false, false, true, true, true, false, false});
}

} // namespace satellite004
