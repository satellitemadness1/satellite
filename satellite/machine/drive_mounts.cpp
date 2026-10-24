// The drives this machine has. See drive_mounts.hpp.

#include "drive_mounts.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>

namespace satellite004 {

namespace {

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

// udev WRITES A BYTE THAT IS NOT PLAIN AS \xNN in its _ENC values -- \x20 for each space.
std::string decoded(const std::string &encoded)
{
    std::string out;
    for (std::size_t at = 0; at < encoded.size(); ++at) {
        unsigned int byte = 0;
        if (encoded[at] == '\\' && at + 3 < encoded.size() && encoded[at + 1] == 'x' &&
            std::sscanf(encoded.c_str() + at + 2, "%2x", &byte) == 1) {
            out += static_cast<char>(byte);
            at += 3;
        } else {
            out += encoded[at];
        }
    }
    return out;
}

// THE E: LINES OF ONE udev RECORD, `E:KEY=value`, as key and value.
std::vector<std::pair<std::string, std::string>> udev_record(const std::string &path)
{
    std::vector<std::pair<std::string, std::string>> facts;
    std::FILE *const file = std::fopen(path.c_str(), "re");
    if (file == nullptr)
        return facts;
    std::string line;
    int c;
    do {
        line.clear();
        while ((c = std::fgetc(file)) != EOF && c != '\n')
            line += static_cast<char>(c);
        const std::size_t equals = line.find('=');
        if (line.compare(0, 2, "E:") == 0 && equals != std::string::npos)
            facts.emplace_back(line.substr(2, equals - 2), line.substr(equals + 1));
    } while (c != EOF);
    std::fclose(file);
    return facts;
}

std::string fact(const std::vector<std::pair<std::string, std::string>> &facts, const char *key)
{
    for (const auto &[name, value] : facts)
        if (name == key)
            return value;
    return std::string();
}

// sda2 BEFORE sda10: a run of digits is compared as the number it is.
bool before(const std::string &a, const std::string &b)
{
    std::size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (std::isdigit(static_cast<unsigned char>(a[i])) && std::isdigit(static_cast<unsigned char>(b[j]))) {
            const std::size_t i_end = a.find_first_not_of("0123456789", i), j_end = b.find_first_not_of("0123456789", j);
            std::string x = a.substr(i, i_end == std::string::npos ? std::string::npos : i_end - i);
            std::string y = b.substr(j, j_end == std::string::npos ? std::string::npos : j_end - j);
            x.erase(0, std::min(x.find_first_not_of('0'), x.size() - 1));
            y.erase(0, std::min(y.find_first_not_of('0'), y.size() - 1));
            if (x.size() != y.size())
                return x.size() < y.size();
            if (x != y)
                return x < y;
            i = i_end == std::string::npos ? a.size() : i_end;
            j = j_end == std::string::npos ? b.size() : j_end;
            continue;
        }
        if (a[i] != b[j])
            return a[i] < b[j];
        ++i;
        ++j;
    }
    return a.size() - i < b.size() - j;
}

} // namespace

std::string model_of(const std::string &disk_in_sys, const std::string &udev)
{
    const std::string model = trimmed(decoded(fact(udev_record(udev + "/b" + first_line_of(disk_in_sys + "/dev")),
                                                   "ID_MODEL_ENC")));
    return !model.empty() ? model : trimmed(first_line_of(disk_in_sys + "/device/model"));
}

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

std::vector<DriveRow> every_drive(const DriveRoots &roots, const std::vector<filesystems::MountedDrive> &mounted)
{
    std::vector<std::string> names;
    if (DIR *const listing = ::opendir(roots.block.c_str())) {
        while (const struct dirent *entry = ::readdir(listing))
            if (entry->d_name[0] != '.')
                names.emplace_back(entry->d_name);
        ::closedir(listing);
    }
    std::sort(names.begin(), names.end(), before);

    std::vector<DriveRow> rows;
    for (const std::string &name : names) {
        const std::string in_sys = real_path(roots.block + "/" + name);
        if (in_sys.empty())
            continue;
        // A LOOP DEVICE, OR A PARTITION OF ONE (loop0p1: a hybrid .iso opened with partitions), is neither a
        // partition nor a disk of the machine's (the review, 2026-10-23): only the loop disk has loop/.
        const std::string disk = disk_under(in_sys);
        if (exists(in_sys + "/loop") || (!disk.empty() && exists(disk + "/loop")))
            continue;
        const std::string numbers = first_line_of(in_sys + "/dev");
        unsigned int major = 0, minor = 0;
        if (std::sscanf(numbers.c_str(), "%u:%u", &major, &minor) != 2)
            continue;
        const auto facts = udev_record(roots.udev + "/b" + numbers);
        if (fact(facts, "ID_FS_USAGE") != "filesystem")
            continue;
        DriveRow row;
        row.name = name;
        row.label = decoded(fact(facts, "ID_FS_LABEL_ENC"));
        if (row.label.empty())
            row.label = fact(facts, "ID_FS_LABEL");
        row.type = fact(facts, "ID_FS_TYPE");
        row.size = std::strtoull(first_line_of(in_sys + "/size").c_str(), nullptr, 10) * 512;   // 512-byte sectors, always
        for (const filesystems::MountedDrive &drive : mounted)
            if (drive.major == major && drive.minor == minor) {
                row.mounted_on = drive.point;
                break;
            }
        // A btrfs OR bcachefs MOUNT NAMES AN ANONYMOUS 0:NN, not its partition's numbers (the review, 2026-10-23):
        // then the device mountinfo names is followed -- /dev/mapper/x to dm-N -- and matched by its name, as
        // satellite.directory.system() names such a device (satl/drives.cpp).
        for (const filesystems::MountedDrive &drive : mounted) {
            if (!row.mounted_on.empty())
                break;
            const std::string source = roots.dev_real ? real_path(drive.source) : drive.source;
            if (!source.empty() && source.substr(source.rfind('/') + 1) == name)
                row.mounted_on = drive.point;
        }
        row.drive = disk.empty() ? std::string() : model_of(disk, roots.udev);
        if (row.drive.empty())
            row.drive = "-";
        if (disk.find("/usb") != std::string::npos)
            row.drive += " (usb)";
        rows.push_back(row);
    }
    return rows;
}

} // namespace satellite004
