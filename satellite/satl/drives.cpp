// The prompt's table of drives. See drives.hpp.

#include "drives.hpp"

#include "listing.hpp"

#include "../machine/drive_mounts.hpp"
#include "../machine/filesystems.hpp"
#include "../machine/shown.hpp"

#include <climits>
#include <cstdio>
#include <cstdlib>
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

std::string leaf_of(const std::string &path)
{
    const std::size_t slash = path.rfind('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
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

std::string drives_listed(const std::vector<DriveRow> &drives)
{
    std::vector<std::vector<std::string>> rows;
    rows.push_back({"name", "label", "type", "size", "mounted on", "drive"});
    for (const DriveRow &drive : drives)
        rows.push_back({shown(drive.name), drive.label.empty() ? std::string("-") : shown(drive.label),
                        drive.type.empty() ? std::string("-") : shown(drive.type), megabytes_with_commas(drive.size),
                        drive.mounted_on.empty() ? std::string("not mounted") : shown(drive.mounted_on),
                        shown(drive.drive)});
    return lined_up(rows, {false, false, false, true, false, false});
}

} // namespace satellite004
