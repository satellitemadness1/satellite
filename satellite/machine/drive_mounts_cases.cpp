// satellite/machine/drive_mounts_cases.cpp -- satellite.drive.list()'s rows, from a fake /sys/class/block and
// /run/udev/data made in a room of its own (FILE_AND_DRIVE_PLAN.md FD-4): a partition with a label, mounted; a
// USB stick's, its label and model written with \x20, not mounted; a filesystem on device-mapper over a LUKS
// partition, followed down to its disk; a disk whose model only /sys has; and what is not a row -- a disk with
// a partition table, swap, a LUKS container and a loop device. The order is the kernel names' (sda2 before
// sda10). Then the machine's own, which must answer without opening a device.
//
//     make build/drive_mounts_cases && build/drive_mounts_cases "${TMPDIR:-/tmp}"    (check.sh runs it)
//
// `ok <name>` for each case that passes, `FAIL <name>: <what happened>` for each that does not; the exit status
// is 0 only when none failed.

#include "drive_mounts.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ftw.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using namespace satellite004;

namespace {

int failed = 0;
int passed = 0;
std::string room;

void check(bool right, const std::string &name, const std::string &detail = "")
{
    if (right) {
        ++passed;
        std::printf("ok %s\n", name.c_str());
    } else {
        ++failed;
        std::printf("FAIL %s: %s\n", name.c_str(), detail.empty() ? "(no detail)" : detail.c_str());
    }
}

void folders(const std::string &path)
{
    for (std::size_t at = room.size() + 1; at != std::string::npos; at = path.find('/', at + 1))
        ::mkdir(path.substr(0, at).c_str(), 0755);
    ::mkdir(path.c_str(), 0755);
}

void write_file(const std::string &path, const std::string &text)
{
    folders(path.substr(0, path.rfind('/')));
    if (std::FILE *file = std::fopen(path.c_str(), "w")) {
        std::fputs(text.c_str(), file);
        std::fclose(file);
    }
}

// ONE BLOCK DEVICE: its folder under devices/, its dev and size, `partition` when it is one, and its link in
// class/block.
void device(const std::string &under, const std::string &name, const std::string &numbers, bool partition,
            unsigned long long int sectors = 2048)
{
    const std::string folder = room + "/sys/devices/" + under + "/" + name;
    write_file(folder + "/dev", numbers + "\n");
    write_file(folder + "/size", std::to_string(sectors) + "\n");
    if (partition)
        write_file(folder + "/partition", "1\n");
    folders(room + "/sys/class/block");
    ::symlink(folder.c_str(), (room + "/sys/class/block/" + name).c_str());
}

void udev(const std::string &numbers, const std::string &lines)
{
    write_file(room + "/udev/b" + numbers, lines);
}

int remove_one(const char *path, const struct stat *, int, struct FTW *)
{
    return ::remove(path);
}

std::string names_of(const std::vector<DriveRow> &rows)
{
    std::string text;
    for (const DriveRow &row : rows)
        text += (text.empty() ? "" : " ") + row.name;
    return text;
}

const DriveRow *row_named(const std::vector<DriveRow> &rows, const std::string &name)
{
    for (const DriveRow &row : rows)
        if (row.name == name)
            return &row;
    return nullptr;
}

std::string said(const DriveRow *row)
{
    return row == nullptr ? "(no row)"
                          : row->name + "|" + row->label + "|" + row->type + "|" + std::to_string(row->size) + "|" +
                                row->mounted_on + "|" + row->drive;
}

void a_fake_machine()
{
    const std::string sata = "pci0000:00/0000:00:17.0/ata1/host0/target0:0:0/0:0:0:0/block";
    device(sata, "sda", "8:0", false, 1953525168);
    udev("8:0", "E:ID_MODEL_ENC=Samsung\\x20SSD\\x20870\\x20EVO\\x201TB\nE:ID_PART_TABLE_TYPE=gpt\n");
    device(sata + "/sda", "sda1", "8:1", true, 2048);
    udev("8:1", "E:ID_FS_USAGE=filesystem\nE:ID_FS_TYPE=ext4\nE:ID_FS_LABEL=purple\nE:ID_FS_LABEL_ENC=purple\n");
    device(sata + "/sda", "sda2", "8:2", true);
    udev("8:2", "E:ID_FS_USAGE=other\nE:ID_FS_TYPE=swap\n");
    device(sata + "/sda", "sda3", "8:3", true);
    udev("8:3", "E:ID_FS_USAGE=crypto\nE:ID_FS_TYPE=crypto_LUKS\n");
    device(sata + "/sda", "sda10", "8:10", true, 4096);
    udev("8:10", "E:ID_FS_USAGE=filesystem\nE:ID_FS_TYPE=vfat\n");

    const std::string other = "pci0000:00/0000:00:17.0/ata2/host1/target1:0:0/1:0:0:0/block";
    device(other, "sdb", "8:16", false);
    write_file(room + "/sys/devices/" + other + "/sdb/device/model", "WDC WD100EMAZ   \n");
    device(other + "/sdb", "sdb1", "8:17", true);
    udev("8:17", "E:ID_FS_USAGE=filesystem\nE:ID_FS_TYPE=xfs\nE:ID_FS_LABEL=big\n");

    const std::string usb = "pci0000:00/0000:00:14.0/usb2/2-1/2-1:1.0/host6/target6:0:0/6:0:0:0/block";
    device(usb, "sdd", "8:48", false);
    udev("8:48", "E:ID_MODEL_ENC=Ventoy\\x20Stick\n");
    device(usb + "/sdd", "sdd1", "8:49", true);
    udev("8:49", "E:ID_FS_USAGE=filesystem\nE:ID_FS_TYPE=exfat\nE:ID_FS_LABEL=Ventoy_Disk\nE:ID_FS_LABEL_ENC=Ventoy\\x20Disk\n");

    device("virtual/block", "dm-0", "254:0", false);
    folders(room + "/sys/devices/virtual/block/dm-0/slaves");
    ::symlink((room + "/sys/devices/" + sata + "/sda/sda3").c_str(),
              (room + "/sys/devices/virtual/block/dm-0/slaves/sda3").c_str());
    udev("254:0", "E:ID_FS_USAGE=filesystem\nE:ID_FS_TYPE=ext4\nE:ID_FS_LABEL=root\n");

    device("virtual/block", "loop0", "7:0", false);
    folders(room + "/sys/devices/virtual/block/loop0/loop");
    udev("7:0", "E:ID_FS_USAGE=filesystem\nE:ID_FS_TYPE=squashfs\n");
    device("virtual/block/loop0", "loop0p1", "259:9", true);   // a hybrid .iso opened with its partitions
    udev("259:9", "E:ID_FS_USAGE=filesystem\nE:ID_FS_TYPE=iso9660\n");

    const std::string nvme = "pci0000:00/0000:00:1d.0/0000:3d:00.0/nvme/nvme0/nvme0n1";
    device("pci0000:00/0000:00:1d.0/0000:3d:00.0/nvme/nvme0", "nvme0n1", "259:0", false);
    udev("259:0", "E:ID_MODEL_ENC=Fast\x20NVMe\n");
    device(nvme, "nvme0n1p3", "259:3", true);
    udev("259:3", "E:ID_FS_USAGE=filesystem\nE:ID_FS_TYPE=btrfs\nE:ID_FS_LABEL=fedora\n");

    std::vector<filesystems::MountedDrive> mounted(3);
    mounted[0].point = "/media/someone/purple";
    mounted[0].major = 8;
    mounted[0].minor = 1;
    mounted[1].point = "/";
    mounted[1].major = 254;
    mounted[1].minor = 0;
    mounted[2].point = "/home";   // btrfs: mountinfo names an anonymous 0:NN, and the device
    mounted[2].major = 0;
    mounted[2].minor = 36;
    mounted[2].source = "/dev/nvme0n1p3";

    const DriveRoots roots{room + "/sys/class/block", room + "/udev", false};
    const std::vector<DriveRow> rows = every_drive(roots, mounted);
    check(names_of(rows) == "dm-0 nvme0n1p3 sda1 sda10 sdb1 sdd1",
          "list: a row is a device that holds a filesystem, sorted with sda1 before sda10; no disk with a partition "
          "table, swap, LUKS container, loop device or partition of one",
          names_of(rows));
    check(said(row_named(rows, "nvme0n1p3")) == "nvme0n1p3|fedora|btrfs|1048576|/home|Fast NVMe",
          "list: a btrfs partition, whose mount names an anonymous 0:NN, is found mounted by the device the mount names",
          said(row_named(rows, "nvme0n1p3")));
    check(said(row_named(rows, "sda1")) == "sda1|purple|ext4|1048576|/media/someone/purple|Samsung SSD 870 EVO 1TB",
          "list: a mounted partition -- its label, type, size from /sys, where it is mounted, and its disk by udev's model",
          said(row_named(rows, "sda1")));
    check(said(row_named(rows, "sdd1")) == "sdd1|Ventoy Disk|exfat|1048576||Ventoy Stick (usb)",
          "list: a USB stick's partition, not mounted -- the label and model written with \\x20 read back, and (usb)",
          said(row_named(rows, "sdd1")));
    check(said(row_named(rows, "dm-0")) == "dm-0|root|ext4|1048576|/|Samsung SSD 870 EVO 1TB",
          "list: a filesystem on device-mapper is followed down its slave, a LUKS partition, to the disk under it",
          said(row_named(rows, "dm-0")));
    check(said(row_named(rows, "sdb1")) == "sdb1|big|xfs|1048576||WDC WD100EMAZ",
          "list: a disk udev has no model for is named by /sys's, trimmed", said(row_named(rows, "sdb1")));
    check(said(row_named(rows, "sda10")) == "sda10||vfat|2097152||Samsung SSD 870 EVO 1TB",
          "list: a filesystem with no label has none", said(row_named(rows, "sda10")));

    const std::vector<DriveRow> nothing = every_drive(DriveRoots{room + "/nowhere", room + "/udev"}, mounted);
    check(nothing.empty(), "list: with no /sys/class/block there is no row, and nothing fails", names_of(nothing));
}

void this_machine()
{
    const std::vector<DriveRow> rows = every_drive(DriveRoots{}, filesystems::mounted_drives());
    bool named = true;
    for (const DriveRow &row : rows)
        named = named && !row.name.empty() && !row.drive.empty();
    check(named, "list: this machine's drives are read, each with a name and a drive (\"-\" when unknown)",
          names_of(rows));
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: drive_mounts_cases <a folder to work in>\n");
        return 2;
    }
    std::string pattern = std::string(argv[1]) + "/drive_mounts_cases.XXXXXX";
    std::vector<char> made(pattern.begin(), pattern.end());
    made.push_back('\0');
    if (::mkdtemp(made.data()) == nullptr) {
        std::fprintf(stderr, "drive_mounts_cases: could not make a folder in %s: %s\n", argv[1], std::strerror(errno));
        return 2;
    }
    room = made.data();
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    a_fake_machine();
    this_machine();
    ::nftw(room.c_str(), remove_one, 16, FTW_DEPTH | FTW_PHYS);
    std::printf("%d ok, %d failed: %s\n", passed, failed, failed == 0 ? "every drive case passed" : "a drive case FAILED");
    return failed == 0 ? 0 : 1;
}
