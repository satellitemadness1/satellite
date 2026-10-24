#pragma once
// satellite/machine/drive_mounts.hpp -- the drives this machine has, mounted or not, for satellite.drive
// (FILE_AND_DRIVE_PLAN.md FD-4, rule 7; the author, 2026-10-21: "satellite.drive.list() // lists all drives").
//
// READ FROM /sys/class/block AND /run/udev/data ALONE, and from the mounts the caller gives: no device is
// opened, so a sleeping disk is never woken -- the 9 TB spinning drives on the author's machine. udev's
// records are a tmpfs, and /sys answers from the kernel's memory.
//
// A ROW IS A PARTITION OR A DISK THAT HOLDS A FILESYSTEM -- udev's ID_FS_USAGE=filesystem. Swap, an LVM or
// RAID member and a LUKS container hold no files and are not rows; the device-mapper device a LUKS or LVM
// volume opens as is one when it holds a filesystem, named dm-N as the kernel names it. A loop device (a snap,
// a mounted .iso) and a partition of one are neither a partition nor a disk of the machine's, and are not rows.
//
// THE MODEL AND THE DISK UNDER A DEVICE are the ones `satellite.directory.system()` shows (satl/drives.cpp),
// and come from here for both.

#include "filesystems.hpp"

#include <string>
#include <vector>

namespace satellite004 {

// WHERE THE FACTS ARE READ: the machine's own, or a fake tree the cases make (drive_mounts_cases.cpp).
struct DriveRoots {
    std::string block = "/sys/class/block";   // a link to each device's folder under /sys/devices
    std::string udev = "/run/udev/data";      // b<major>:<minor>, udev's record of each block device
    bool dev_real = true;                     // a mount's /dev/ path is followed (/dev/mapper/x to dm-N); the
                                              // cases' fake mounts name their devices as they are
};

struct DriveRow {
    std::string name;         // the kernel's: sdd1, nvme0n1p2, dm-0
    std::string label;        // the filesystem's own name, "" when it has none
    std::string type;         // ext4, ntfs, vfat ...
    unsigned long long int size = 0;   // bytes
    std::string mounted_on;   // "" when it is not mounted
    std::string drive;        // the disk it is on, by its model, and " (usb)" on a USB port; "-" when unknown
};

// Every drive that holds a filesystem, sorted by name with a run of digits read as its number (sda1, sda2, ...
// sda10, sdb1), each with where `mounted` says it is mounted: by its numbers, or -- for btrfs, whose mount names
// an anonymous 0:NN -- by the device the mount names.
std::vector<DriveRow> every_drive(const DriveRoots &roots, const std::vector<filesystems::MountedDrive> &mounted);

// THE DISK A BLOCK DEVICE IS ON, as its folder under /sys: a partition's is the folder above it, and a
// device-mapper or md device's is under its first slave -- followed down until a real disk.
std::string disk_under(std::string device_in_sys);

// THE DISK'S WHOLE NAME, as lsblk shows it: udev's ID_MODEL_ENC, else /sys's first 16 characters. "" when
// neither has one.
std::string model_of(const std::string &disk_in_sys, const std::string &udev = "/run/udev/data");

} // namespace satellite004
