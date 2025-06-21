# OxizeOS ChangeLog

## June 21, 2025: Disk 2.0 Release

### Changes

- Implemented new **FAT32** driver inside the Host with full Read / Write operations, not fully tested and may contain bugs
- Implemented new Disk image creation tool in **Rust** that generates disk images based on a specified `image.json` configuration file, integrating operations via dd, parted & the new **FAT32** host driver
- Started implementing logfile logging mechanizm in x86Kernel, currently under developement
- Incereased the disk size to 1 GiB
- Resized the boot partition to 128 MiB
- Created a new primary partition for user-land & system applications with a size of 512 MiB
- Started creating new TTY driver, currently under developement
