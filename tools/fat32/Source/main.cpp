// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <filesystem>

#include <defs.hpp>
#include <utils.hpp>

#include <disk.hpp>
#include <part.hpp>
#include <fat.hpp>

int main(int argc, char** argv)
{
	// Syntax: fat32 <[Override Flags] | <Disk Image> <Partition Index / Partition GUID> <Operation> [Flags]>
	if(argc < 2)
	{
		fprintf(stderr, "[FAT32] [ERROR]: Syntax: %s <[Override Flags] | <Disk Image> <Partition Index / Partition GUID> <Operation> [Flags]>\n", argv[0]);
		fprintf(stderr, "[FAT32] [INFO]: Use `%s -h` or `%s --help` for more info\n", argv[0], argv[0]);
		return FAT32_INVALID_PARAMETER;
	}

	if(argv[1][0] == '-')
	{
		// Override flag specified, handle it, then return
		// Override flag means a special operation that doesn't involve the other arguments and returns early
		std::string of = argv[1];
		if(of == "-h" || of == "--help")
		{
			printf("[FAT32] [INFO]: FAT(File Allocation Table) filesystem tool\n");
			printf("[FAT32] [INFO]: Syntax: %s <[Override Flags] | <Disk Image> <Partition Index / Partition GUID> <Operation> [Flags]>\n", argv[0]);
			printf("[FAT32] [INFO]: Use `%s <Disk Image> help` for more info about operations and their flags\n", argv[0]);
			printf("[FAT32] [INFO]: Disk Image must be a RAW format disk image file, with an MBR / GPT partition table\n");
			return FAT32_SUCCESS;
		}
		else
		{
			fprintf(stderr, "[FAT32] [INFO]: Unknown override flag %s\n", of.c_str());
			return FAT32_INVALID_PARAMETER;
		}
	}

	if(argc < 4)
	{
		fprintf(stderr, "[FAT32] [ERROR]: Syntax: %s <[Override Flags] | <Disk Image> <Partition Index / Partition GUID> <Operation> [Flags]>\n", argv[0]);
		fprintf(stderr, "[FAT32] [INFO]: Use `%s -h` or `%s --help` for more info\n", argv[0], argv[0]);
		return FAT32_INVALID_PARAMETER;
	}

	// Extract arguments

	const char* diskImage = argv[1];
	const char* partIdent = argv[2];
	const char* operation = argv[3];
	char** flags = &argv[4];
	int flagc = argc - 4;

	// Initialize DISK

	FAT32::DISK disk;
	FAT32_STATUS status = disk.Initialize(diskImage);
	if(FAT32_ERROR(status))
	{
		printf("[FAT32] [ERROR]: Failed to initialize DISK\n");
		return status;
	}

	// Initialize Partition Manager

	FAT32::PartMgr::PartMgr partMgr;
	status = partMgr.Initialize(&disk);
	if(FAT32_ERROR(status) && status != FAT32_PARTITION_TABLE_RECOVERY_SUCCEEDED)
	{
		printf("[FAT32] [ERROR]: Failed to initialize Partition Manager\n");
		return status;
	}

	FAT32::PartMgr::PartDesc* partition;

	FAT32_GUID partGUID = {};
	uint32_t partIndex;
	bool guidSpecified = false;
	if(strncasecmp(partIdent, "GUID:", 5) == 0)
	{
		// Parse the string GUID into a FAT32_GUID. String GUID is in format: "12345678-9ABC-DEF0-1234-56789ABCDEF0"
		const char* ptr = partIdent + 5;
		char* endptr = nullptr;
		guidSpecified = true;

		partGUID.Data1 = static_cast<uint32_t>(strtoul(ptr, &endptr, 16));
		if(*endptr != '-')
		{
			fprintf(stderr, "[FAT32] [ERROR]: Invalid partition GUID specified: separator after part 1 not found\n");
			return FAT32_INVALID_PARAMETER;
		}

		ptr = endptr + 1;
		partGUID.Data2 = static_cast<uint16_t>(strtoul(ptr, &endptr, 16));
		if(*endptr != '-')
		{
			fprintf(stderr, "[FAT32] [ERROR]: Invalid partition GUID specified: separator after part 2 not found\n");
			return FAT32_INVALID_PARAMETER;
		}

		ptr = endptr + 1;
		partGUID.Data3 = static_cast<uint16_t>(strtoul(ptr, &endptr, 16));
		if(*endptr != '-')
		{
			fprintf(stderr, "[FAT32] [ERROR]: Invalid partition GUID specified: separator after part 3 not found\n");
			return FAT32_INVALID_PARAMETER;
		}

		// Data4 in text representation is split into 2 parts: a 16-bit and a 48-bit part, both are still big endian, but we need to combine it into a 64-bit integer
		ptr = endptr + 1;
		uint16_t Data4_1 = static_cast<uint16_t>(strtoul(ptr, &endptr, 16));
		if(*endptr != '-')
		{
			fprintf(stderr, "[FAT32] [ERROR]: Invalid partition GUID specified: separator after part 4 not found\n");
			return FAT32_INVALID_PARAMETER;
		}

		ptr = endptr + 1;
		uint64_t Data4_2 = static_cast<uint64_t>(strtoull(ptr, &endptr, 16));

		if(*endptr != '\0')
		{
			fprintf(stderr, "[FAT32] [ERROR]: Invalid partition GUID specified: end of argument after part 5 not found\n");
			return FAT32_INVALID_PARAMETER;
		}

		uint64_t Data4Combined = (static_cast<uint64_t>(Data4_1) << 48) | Data4_2;

		for(uint8_t i = 0; i < 8; i++) partGUID.Data4[i] = (Data4Combined >> ((7 - i) * 8)) & 0xFF;

		// Now that the GUID has been parsed from a string to a structure, open the partition using the GUID

		auto openRes = partMgr.OpenPartitionByGUID(partGUID);
		if(!openRes)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to open partition with GUID " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(partGUID));
			return openRes.error();
		}
		partition = openRes.value();
	}
	else
	{
		guidSpecified = false;
		char* endptr;
		partIndex = static_cast<uint64_t>(strtoul(partIdent, &endptr, 10));
		if(endptr == partIdent || *endptr != '\0')
		{
			fprintf(stderr, "[FAT32] [ERROR]: Invalid partition index %s\n", partIdent);
			return FAT32_INVALID_PARAMETER;
		}
		auto openRes = partMgr.OpenPartitionByIndex(partIndex);
		if(!openRes)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to open partition with index %u\n", partIndex);
			return openRes.error();
		}
		partition = openRes.value();
	}

	// Initialize FAT
	FAT32::FAT::FAT fat;
	status = fat.Initialize(&partMgr, partition);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [ERROR]: Failed to initialize FAT driver\n");
		return status;
	}

	// Handle operations
	std::string op = operation;
	
	if(op == "help")
	{
		printf("[FAT32] [INFO]: Operation list:\n");
		printf("[FAT32] [INFO]:     help: prints this menu and exits\n");
		printf("[FAT32] [INFO]:     read <file path> [count] [offset]: prints the contents of a file in the image starting at the offset(default is 0) for n amount of bytes(default is until the end of file)\n");
		printf("[FAT32] [INFO]:     dir <directory path>: indexes and prints the contents of a directory\n");
		printf("[FAT32] [INFO]:     mkdir <directory path>: creates a new directory at the specified path\n");
		printf("[FAT32] [INFO]:     create <file path>: creates a new file at the specified path\n");
		printf("[FAT32] [INFO]:     import <host path> <target path>: imports a file from the host to the image at target path\n");
		printf("[FAT32] [INFO]:     debug-cmd: goes to a special debugger command prompt to debug the tool and the image\n");
		return FAT32_SUCCESS;
	}

	if(op == "read")
	{
		// Gather the arguments
		if(flagc < 1)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Operation `read` requires a file to read to be specified\n");
			return FAT32_INVALID_PARAMETER;
		}
		std::string filePath = flags[0];
		std::wstring wFilePath = utf8StringToWideString(filePath);

		uint64_t bufferSize = 0;
		if(flagc > 1 && strcasecmp(flags[1], "DEFAULT") != 0)
		{
			char* endptr;
			bufferSize = strtoul(flags[1], &endptr, 0);
			if(*endptr == flags[1][0] || *endptr != '\0')
			{
				fprintf(stderr, "[FAT32] [ERROR]: Invalid byte count specified for the read operation\n");
				return FAT32_INVALID_PARAMETER;
			}
			if(bufferSize == 0) return FAT32_SUCCESS; // Nothing to read, no need to open the file
		}

		uint64_t offsetInFile = 0;
		if(flagc > 2)
		{
			char* endptr;
			offsetInFile = strtoul(flags[2], &endptr, 0);
			if(*endptr == flags[2][0] || *endptr != '\0')
			{
				fprintf(stderr, "[FAT32] [ERROR]: Invalid file offset specified for the read operation\n");
				return FAT32_INVALID_PARAMETER;
			}
		}

		if(flagc > 3)
		{
			fprintf(stderr, "[FAT32] [ERROR]: too much arguments for the read operation\n");
			return FAT32_INVALID_PARAMETER;
		}

		// Open the file
		auto openRes = fat.OpenFile(wFilePath);
		if(!openRes)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to open file %s for reading\n", filePath.c_str());
			return openRes.error();
		}
		FAT32::FAT::FAT_File file = openRes.value();

		// Seek to the requested position
		if(offsetInFile != 0)
		{
			status = fat.Seek(&file, offsetInFile, FAT32::FAT::FAT_SeekBase::SeekFromStart);
			if(FAT32_ERROR(status))
			{
				fprintf(stderr, "[FAT32] [ERROR]: Failed to seek in file %s to position %lu for reading\n", filePath.c_str(), offsetInFile);
				return status;
			}
		}

		// Get the buffer size
		if(bufferSize == 0)
		{
			auto getSizeRes = fat.GetFileSize(&file);
			if(!getSizeRes)
			{
				fprintf(stderr, "[FAT32] [ERROR]: Failed to get file size in file %s for the read operation\n", filePath.c_str());
				return getSizeRes.error();
			}
			bufferSize = getSizeRes.value();
		}

		// Allocate a buffer of the requested size
		uint8_t* fileBuffer = reinterpret_cast<uint8_t*>(malloc(bufferSize));
		if(bufferSize && !fileBuffer)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to allocate a buffer to read the file data into\n");
			return FAT32_MEMORY_ALLOCATION_FAILED;
		}

		// Read the file into the buffer
		auto readRes = fat.ReadFile(&file, bufferSize, fileBuffer);
		if(!readRes)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to read the file %s\n", filePath.c_str());
			return readRes.error();
		}
		uint64_t read = readRes.value();
		printf("[FAT32] [INFO]: Read %lu bytes out of %lu requested\n", read, bufferSize);
		printf("[FAT32] [INFO]: Buffer contents:\n");
		
		for(size_t i = 0; i < read; i++)
		{
			char c = fileBuffer[i];

			if(c == '\r') continue;
			if(c == '\n')
			{
				puts("");
				continue;
			}

			if(isprint(c)) putc(c, stdout);
			else printf("<%02X>", c);
		}
		puts("");
		
		return FAT32_SUCCESS;
	}

	if(op == "dir")
	{
		// Gather arguments
		if(flagc < 1)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Operation `dir` requires a directory to index to be specified\n");
			return FAT32_INVALID_PARAMETER;
		}
		std::string dirPath = flags[0];
		std::wstring wDirPath = utf8StringToWideString(dirPath);

		if(flagc > 1)
		{
			fprintf(stderr, "[FAT32] [ERROR]: too much arguments for the dir operation\n");
			return FAT32_INVALID_PARAMETER;
		}

		// Open the directory
		auto openRes = fat.OpenFile(wDirPath, true);
		if(!openRes)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to open directory %s for listing contents\n", dirPath.c_str());
			return openRes.error();
		}
		FAT32::FAT::FAT_File dir = openRes.value();

		// Open a memory stream to log output to, so if an error occurs it wouldn't be in the middle of the entry text
		// After the stream is finished, flush it to stdout
		char* buffer = nullptr;
		size_t bufferSize = 0;

		FILE* memstream = open_memstream(&buffer, &bufferSize);
		
		// Print pre-entry info
		std::string volumeLabel = "is ";

		status = fat.GetVolumeLabel(volumeLabel);
		if(status == FAT32_FAT_END_OF_DIRECTORY) volumeLabel = "has no label";
		else if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to get volume label\n");
			fclose(memstream);
			free(buffer);
			return status;
		}

		if(guidSpecified) fprintf(memstream, "Volume in partition " GUID_PRINT_FORMAT " %s\n", DECODE_GUID_TO_ARG(partGUID), volumeLabel.c_str());
		else fprintf(memstream, "Volume in partition %u %s\n", partIndex, volumeLabel.c_str());

		fprintf(memstream, "Directory of %s\n\n", dirPath.c_str());

		// Index the directory and print the contents
		struct entryInfo
		{
			std::wstring name;
			std::string formattedSize;
			FAT32::FAT::FAT_TimeDate timeDate;
			bool directory;
		};
		std::vector<entryInfo> infoVec;
		size_t longestFNNumber = 0;
		size_t dirCount = 0, fileCount = 0;
		size_t totalUsedSize = 0;
		while(true)
		{
			auto indexRes = fat.GetNextDirectoryEntry(&dir);
			if(!indexRes)
			{
				if(indexRes.error() == FAT32_FAT_END_OF_DIRECTORY) break;
				fprintf(stderr, "[FAT32] [ERROR]: Failed to index directory %s for listing contents\n", dirPath.c_str());
				fclose(memstream);
				free(buffer);
				return indexRes.error();
			}
			FAT32::FAT::FAT_DirectoryEntryInfo info = indexRes.value();

			entryInfo eInfo = {};
			eInfo.name = info.name;
			eInfo.formattedSize = FormatNumber(info.size);
			eInfo.timeDate = info.timeDate;
			eInfo.directory = FLAG_GET_BOOLEAN(info.flags, FAT32::FAT::FAT_FILE_FLAG_DIRECTORY);
			
			if(eInfo.directory) dirCount++;
			else
			{
				fileCount++;
				totalUsedSize += info.size;
			}
			
			size_t formatLength = eInfo.formattedSize.length();
			if(longestFNNumber < formatLength) longestFNNumber = formatLength;

			infoVec.push_back(eInfo);
		}

		for(size_t i = 0; i < infoVec.size(); i++)
		{
			entryInfo info = infoVec[i];
			fprintf(memstream, "%04u-%02u-%02u   %02u:%02u %s    %s  ", info.timeDate.year, info.timeDate.month, info.timeDate.day, info.timeDate.hour % 12, info.timeDate.minute, info.timeDate.hour / 12 == 1 ? "PM" : "AM", info.directory ? "<DIR>" : "     ");
			fprintf(memstream, "%-*s %ls\n", static_cast<int>(longestFNNumber), info.directory ? "" : info.formattedSize.c_str(), info.name.c_str());
		}
		fprintf(memstream, "\n");
		fprintf(memstream, "                %s File(s)       %s bytes\n", FormatNumber(fileCount).c_str(), FormatNumber(totalUsedSize).c_str());
		fprintf(memstream, "                %s Dir(s)        %s bytes free\n", FormatNumber(dirCount).c_str(), FormatNumber(fat.GetFreeByteCount()).c_str());

		fclose(memstream);

		fwrite(buffer, 1, bufferSize, stdout);
		free(buffer);

		return FAT32_SUCCESS;
	}

	if(op == "mkdir")
	{
		// Gather arguments
		if(flagc < 1)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Operation `mkdir` requires directory to create path to be specified\n");
			return FAT32_INVALID_PARAMETER;
		}
		std::string dirPath = flags[0];
		std::wstring wDirPath = utf8StringToWideString(dirPath);

		if(flagc > 1)
		{
			fprintf(stderr, "[FAT32] [ERROR]: too much arguments for the mkdir operation\n");
			return FAT32_INVALID_PARAMETER;
		}

		// Create the directory
		auto createRes = fat.CreateFile(wDirPath, FAT32::FAT::FAT_FILE_FLAG_DIRECTORY, true);
		if(!createRes)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to create directory %s\n", dirPath.c_str());
			return createRes.error();
		}

		return FAT32_SUCCESS;
	}

	if(op == "create")
	{
		// Gather arguments
		if(flagc < 1)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Operation `create` requires file to create path to be specified\n");
			return FAT32_INVALID_PARAMETER;
		}
		std::string filePath = flags[0];
		std::wstring wFilePath = utf8StringToWideString(filePath);

		if(flagc > 1)
		{
			fprintf(stderr, "[FAT32] [ERROR]: too much arguments for the create operation\n");
			return FAT32_INVALID_PARAMETER;
		}

		// Create the file
		auto createRes = fat.CreateFile(wFilePath, FAT32::FAT::FAT_FILE_FLAG_FILE, false);
		if(!createRes)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to create file %s\n", filePath.c_str());
			return createRes.error();
		}

		return FAT32_SUCCESS;
	}
	
	if(op == "import")
	{
		// Gather arguments
		if(flagc < 2)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Operation `import` requires host path and target path to be specified\n");
			return FAT32_INVALID_PARAMETER;
		}
		std::string hostPath = flags[0];
		std::string targetPath = flags[1];
		std::wstring wTargetPath = utf8StringToWideString(targetPath);

		if(flagc > 2)
		{
			fprintf(stderr, "[FAT32] [ERROR]: too much arguments for the import operation\n");
			return FAT32_INVALID_PARAMETER;
		}

		// Create the file
		auto openRes = fat.CreateFile(wTargetPath, FAT32::FAT::FAT_FILE_FLAG_FILE, false);
		if(!openRes)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to create file %s for writing\n", targetPath.c_str());
			return openRes.error();
		}
		FAT32::FAT::FAT_File file = openRes.value();

		// Read the host file into a buffer

		size_t fileSize = std::filesystem::file_size(hostPath);
		FILE* hostFile = fopen(hostPath.c_str(), "rb");
		if(!hostFile)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to open the host file %s\n", hostPath.c_str());
			return FAT32_HOST_FILESYSTEM_ERROR;
		}

		void* hostBuffer = malloc(fileSize);
		if(!hostBuffer)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to allocate a buffer for the host file %s\n", hostPath.c_str());
			return FAT32_MEMORY_ALLOCATION_FAILED;
		}

		if(fread(hostBuffer, 1, fileSize, hostFile) != fileSize)
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to read host file %s to buffer\n", hostPath.c_str());
			return FAT32_HOST_FILESYSTEM_ERROR;
		}

		// Write buffer to the FAT32 filesystem

		status = fat.WriteFile(&file, fileSize, hostBuffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [ERROR]: Failed to write buffer to the target file %s\n", targetPath.c_str());
			return status;
		}

		return FAT32_SUCCESS;
	}	

	if(op == "debug-cmd")
	{
		printf("[FAT32] [INFO]: Starting built-in debug command prompt...\n");
		printf("[DEBUG-CMD] [BUILT-IN]: Use `help` for more info\n");

		uint64_t partitionStartLba = partMgr.GetPartitionStartLBA(partition);

		while(true)
		{
			// command format: <command> [flags]
			// Supported commands:
			//
			// help: prints info about this builtin command prompt
			// dump: flags: <sector>, <count>: dumps the disk contents at the sector inside the partition for the amount of sectors specified
			// readhex: flags: <path>: prints a file contents in a hex dump
			// exit: exits the program
			// clear: clears the screen
			printf("BUILTIN-DEBUG-CMD>");
			char command[256];
			if(!fgets(command, sizeof(command), stdin))
			{
				fprintf(stderr, "[FAT32] [ERROR]: Failed to get string\n");
				return FAT32_INVALID_PARAMETER;
			}
			command[strcspn(command, "\n")] = '\0';

			char* token = strtok(command, " ");
			if(strcasecmp(token, "help") == 0)
			{
				printf("Built-in debugger command prompt\n");
				printf("Command format: <command> [flags]\n");
				printf("Supported commands:\n\n");
				printf("help: prints info about this builtin command prompt\n");
				printf("dump: flags: <sector>, <count>: dumps the disk contents at the sector inside the partition for the amount of sectors specified\n");
				printf("exit: exits the program\n");
				printf("clear: clears the screen\n");
				continue;
			}

			if(strcasecmp(token, "exit") == 0)
			{
				printf("[DEBUG-CMD] [BUILT-IN]: Exiting the program\n");
				return FAT32_SUCCESS;
			}

			if(strcasecmp(token, "clear") == 0)
			{
				printf("\033[2J\033[H");
				fflush(stdout);
				continue;
			}

			if(strcasecmp(token, "dump") == 0)
			{
				char* sectorStr = strtok(nullptr, " ");
				char* countStr = strtok(nullptr, " ");

				uint64_t sector = static_cast<uint64_t>(strtoull(sectorStr, nullptr, 10));
				uint64_t count = static_cast<uint64_t>(strtoull(countStr, nullptr, 10));

				uint8_t* buffer = reinterpret_cast<uint8_t*>(malloc(count * SECTOR_SIZE));
				if(!buffer)
				{
					fprintf(stderr, "[DEBUG-CMD] [BUILT-IN]: Failed to allocate memory for the data buffer\n");
					return FAT32_MEMORY_ALLOCATION_FAILED;
				}

				status = partMgr.ReadSectors(partition, sector, count, buffer);
				if(FAT32_ERROR(status))
				{
					fprintf(stderr, "[DEBUG-CMD] [BUILT-IN]: Failed to read data from disk\n");
					return status;
				}

				DumpFormattedHex(buffer, count * SECTOR_SIZE, (partitionStartLba + sector) * SECTOR_SIZE);
				continue;
			}

			if(strcasecmp(token, "readhex") == 0)
			{
				char* pathStr = strtok(nullptr, " ");
				std::wstring wFilePath = utf8StringToWideString(pathStr);
			
				// Open the file
				auto openRes = fat.OpenFile(wFilePath);
				if(!openRes)
				{
					fprintf(stderr, "[FAT32] [ERROR]: Failed to open file %s for reading\n", pathStr);
					return openRes.error();
				}
				FAT32::FAT::FAT_File file = openRes.value();
			
				auto getSizeRes = fat.GetFileSize(&file);
				if(!getSizeRes)
				{
					fprintf(stderr, "[FAT32] [ERROR]: Failed to get file size in file %s for the read operation\n", pathStr);
					return getSizeRes.error();
				}
				size_t bufferSize = (getSizeRes.value() + 15) & ~15ULL;
			
				// Allocate the buffer
				uint8_t* fileBuffer = reinterpret_cast<uint8_t*>(malloc(bufferSize));
				if(bufferSize && !fileBuffer)
				{
					fprintf(stderr, "[FAT32] [ERROR]: Failed to allocate a buffer to read the file data into\n");
					return FAT32_MEMORY_ALLOCATION_FAILED;
				}
			
				// Read the file into the buffer
				auto readRes = fat.ReadFile(&file, bufferSize, fileBuffer);
				if(!readRes)
				{
					fprintf(stderr, "[FAT32] [ERROR]: Failed to read the file %s\n", pathStr);
					return readRes.error();
				}

				printf("bufferSize: 0x%lX\n", getSizeRes.value());
				DumpFormattedHex(fileBuffer, bufferSize, 0);
				continue;
			}

			fprintf(stderr, "[DEBUG-CMD] [BUILT-IN]: Unknown command \"%s\"\n", token);
		}
	}

	fprintf(stderr, "[FAT32] [ERROR]: Unknown operation %s\n", op.c_str());
	return FAT32_INVALID_PARAMETER;
}