#pragma once
// satellite/satl/drives.hpp -- the table a person sees when `satellite.directory.system()`
// is a whole typed line (the author, 2026-09-25: "satellite.directory.system() will
// report the file space on all attached drives ... space attached to the machine").
//
// TWO LINES, THEN A ROW A DRIVE. The lines are the space on every mounted drive added
// up, and how much of it is free -- written as the listing's free space is, always mb
// with commas, and white at a terminal (the session does the colour). A row is one
// mounted drive (filesystems.hpp says which mounts are drives): where it is mounted,
// its filesystem, its size, used and free space -- df's Size, Used and Avail -- the
// kernel's name for its device, and the disk it is on, by the model the disk gives
// and with "usb" when it hangs off a USB port. A logical volume or RAID device is
// followed down /sys/block/<device>/slaves to the disk under it.
//
// ONLY WHAT IS MOUNTED. A partition that is not mounted has no free space anyone can
// ask about without mounting it, and swap holds no files; neither is a row.

#include <string>
#include <vector>

namespace satellite004 {

// "SPACE ON ALL DRIVES: ..." and "FREE SPACE ON ALL DRIVES: ...", with no colour.
std::vector<std::string> drives_lines(const std::vector<std::string> &points);

// The table for the drives mounted at `points`, in the order given.
std::string drives_table(const std::vector<std::string> &points);

} // namespace satellite004
