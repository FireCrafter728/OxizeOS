# Image configuration format specification

## Variables

Currently only 1 variable is supported: `DISK_IMAGE`. 
This variable specifies the output image to create, format
and perform File System operations on.

## Comments

To specify a comment, type in `#`, and everything from that symbol to the end of the line will be commented and skipped in parsing.

## Function format

- A function is specified like this: `EXAMPLE(key=value, ...)`.
- If value is quoted, it's a string, else it's a number.

## Supported functions

- `DISK_INIT(format="", size="")` - Creates a disk image with specified format and size, and additionally writes the GPT Tables onto it.

- `MKPART(index=, label="", code="", start=, sectors=, letter="")` - Creates a partition in the GPT table with specified parameters and also assigns a letter to that partition

- `MKFS(part=, label="")` - Formats a specified partition with FAT32

- `MKDIR(path="")` - Creates a directory on a disk specified by the letter in the path

- `COPY(src="", dst="")` - Copies a file from `src` in the host to `dst` in the disk. partition specified by the letter at the start of `dst`

## Drive letters

Drive letters are an addition to the image configuration format so instead of having to specify the partition index each function, you specify the drive letter in the path