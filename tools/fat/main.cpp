#include <gpt.hpp>
#include <fat32.hpp>
#include <disk.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <parser.hpp>
#include <memory>

int main(int argc, char **argv)
{
    bool ScriptingMode = (argc > 1 && strcmp(argv[1], "-s") == 0);

    FAT32::Parser::Parser *parser = new FAT32::Parser::Parser;
    if (!parser)
    {
        fprintf(stderr, "[FAT32-MAIN] [ERROR]: Failed to allocate memory for Parser\n");
        return 1;
    }

    FAT32::Parser::Args args;
    if (ScriptingMode)
        args = parser->ParseArgsScriptingMode(argc, argv);
    else
        args = parser->ParseArgs(argc, argv);

    if (args.Version)
    {
        printf("Test FAT32 Driver Version 1.0\nSyntax: %s <Disk Image> [FLAGS]\n", argv[0]);
        delete (parser);
        return 0;
    }

    if (args.Help)
    {
        printf("Test FAT32 Driver Version 1.0\n"
               "Syntax: %s <Disk Image> [FLAGS]\n"
               "Supported disk image formats: RAW\n"
               "Operations:\n"
               "    ReadFile - Print specified file contents\n"
               "    CreateFile - Create a file\n"
               "    DeleteFile - Delete a file\n"
               "    RenameFile - Rename a file\n"
               "    MoveFile - Move a file to a specified location\n"
               "    CopyFileTo - Copy a file into the disk image\n"
               "    CopyFileFrom - Copy a file from the disk image to a specified location in the disk\n",
               argv[0]);
        delete (parser);
        return 0;
    }

    if (Empty(args.Disk))
    {
        fprintf(stderr, "[FAT32-MAIN] [ERROR]: Disk image has to be specified\n");
        delete (parser);
        return 2;
    }

    FAT32::Parser::OperationDesc operation;

    if (ScriptingMode)
    {
        int elementCount = 10;
        static const FAT32::Parser::ScriptArg scriptArgsDesc[] = {
            {'i', "input", true},
            {'o', "output", true},
            {'b', "bytespersector", true},
            {'c', "sectorspercluster", true},
            {'m', "mediadesctype", true},
            {'t', "sectorspertrack", true},
            {'h', "headcount", true},
            {'d', "drivenumber", true},
            {'v', "volumeid", true},
            {'l', "volumelabel", true}};
        operation = parser->getOperationScriptingMode(argc, argv, scriptArgsDesc, elementCount);
    }
    else
        operation = parser->getOperation();

    if (operation.operation == FAT32::Parser::Operations::NONE)
    {
        printf("[FAT32-MAIN] [ERROR]: Failed to retrieve operation\n");
        delete (parser);
        return -1;
    }

    FAT32::DISK *disk = new FAT32::DISK;
    if (!disk)
    {
        fprintf(stderr, "[FAT32-MAIN] [ERROR]: Failed to allocate memory for DISK\n");
        delete (parser);
        return 3;
    }

    if (!disk->Initialize(args.Disk))
    {
        fprintf(stderr, "[FAT32-MAIN] [ERROR]: Failed to initialize DISK\n");
        delete (parser);
        delete (disk);
        return 4;
    }

    FAT32::GPT::GPT* gpt = new FAT32::GPT::GPT;
    if (!gpt)
    {
        fprintf(stderr, "[FAT32-MAIN] [ERROR]: Failed to allocate memory for GPT\n");
        delete (parser);
        delete (disk);
        return 5;
    }

    FAT32::GPT::GPTDesc gptDesc = {};

    int partition = -1;
    if (args.PartitionIndex != 0)
        partition = args.PartitionIndex;
    if (!gpt->Initialize(disk, &gptDesc, partition))
    {
        fprintf(stderr, "[FAT32-MAIN] [ERROR]: Failed to initialize GPT\n");
        delete (parser);
        delete (disk);
        delete (gpt);
        return 6;
    }

    FAT32::FAT::FAT *fat = new FAT32::FAT::FAT;
    if (!fat)
    {
        fprintf(stderr, "[FAT32-MAIN] [ERROR]: Failed to allocate memory for FAT\n");
        delete (parser);
        delete (disk);
        delete (gpt);
        return 7;
    }
    if (operation.operation != FAT32::Parser::Operations::MakeFS)
    {
        if (!fat->Initialize(gpt))
        {
            fprintf(stderr, "[FAT32-MAIN] [ERROR]: Failed to initialize FAT\n");
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 8;
        }
    }
    else fat->InitializeMin(gpt);

    FAT32::FAT::File file;
    if (operation.operation == FAT32::Parser::Operations::ReadFile || operation.operation == FAT32::Parser::Operations::RenameFile || operation.operation == FAT32::Parser::Operations::RenameDirectory || operation.operation == FAT32::Parser::Operations::CopyFileFrom)
    {
        if (!fat->OpenFile(operation.args[0], &file))
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to open file %s\n", operation.args[0]);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 9;
        }
    }

    switch (operation.operation)
    {
    case FAT32::Parser::Operations::ReadFile:
    {
        m_uint8_t buffer[SECTOR_SIZE];
        m_uint32_t read;
        while ((read = fat->ReadFile(&file, &buffer, SECTOR_SIZE)))
            for (m_uint32_t i = 0; i < read; i++)
                printf("%c", buffer[i]);
        printf("\n");
        break;
    }
    case FAT32::Parser::Operations::ListContents:
    {
        printf("\n");
        FAT32::FAT::File current;
        if (!fat->OpenFile("/", &current))
        {
            fprintf(stderr, "[FAT32-MAIN] [ERROR]: Failed to open root directory\n");
            return false;
        }

        FAT32::FAT::LFNDirectoryEntry Volume;
        while (fat->ReadEntry(&current, &Volume) && !(Volume.SFNEntry.Attribs & FAT32::FAT::FileAttribs::VOLUMEID))
            ;
        printf("Volume in partition %d is %*s\n", gpt->GetPartitionIndex(), 11, Volume.SFNEntry.Name);
        printf("Directory of %s\n\n", operation.args[0]);

        if (!fat->OpenFile(operation.args[0], &current))
        {
            fprintf(stderr, "[FAT32-MAIN] [ERROR]: Failed to open directory %s", operation.args[0]);
            return false;
        }

        FAT32::FAT::LFNDirectoryEntry entry;
        uint16_t FileCount = 0, DirCount = 0;
        uint32_t TotalBytesUsed = 0;
        char buffer[20];

        while (fat->ReadEntry(&current, &entry))
        {
            if (!(entry.SFNEntry.Attribs & FAT32::FAT::FileAttribs::DIRECTORY) &&
                !(entry.SFNEntry.Attribs & FAT32::FAT::FileAttribs::ARCHIVE))
                continue;

            char TimeStr[9];
            char DateStr[11];

            fat->GetTimeFormatted(entry.SFNEntry.CreationTime, TimeStr);
            fat->GetDateFormatted(entry.SFNEntry.CreationDate, DateStr);

            printf("%s   %s    ", DateStr, TimeStr);
            if (entry.SFNEntry.Attribs & FAT32::FAT::FileAttribs::DIRECTORY)
            {
                printf("<DIR>  ");
                printf("                    ");
                DirCount++;
            }
            else
            {
                printf("       ");
                size_t len = NumberToFormattedStr(entry.SFNEntry.FileSize, buffer);
                printf("%s", buffer);
                for (size_t i = len; i < sizeof(buffer); i++)
                    printf(" ");
                FileCount++;
                TotalBytesUsed += entry.SFNEntry.FileSize;
            }

            if (entry.IsLFN)
                printf("  %s\n", entry.LFN);
            else
                printf("  %*s\n", 11, entry.SFNEntry.Name);
        }

        printf("                %d File", FileCount);
        if (FileCount != 1)
            printf("s");
        else
            printf(" ");

        NumberToFormattedStr(TotalBytesUsed, buffer);
        printf("         %s bytes\n", buffer);

        printf("                %d Dir", DirCount);
        if (DirCount != 1)
            printf("s");
        else
            printf(" ");

        NumberToFormattedStr(fat->GetBytesFree(), buffer);
        printf("          %s bytes free\n", buffer);

        break;
    }
    case FAT32::Parser::Operations::CreateFile:
    {
        if (!fat->CreateEntry(operation.args[0], false, &file))
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to create file %s\n", operation.args[0]);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 10;
        }
        break;
    }
    case FAT32::Parser::Operations::CreateDirectory:
    {
        if (!fat->CreateEntry(operation.args[0], true, &file))
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to create directory %s\n", operation.args[0]);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 11;
        }
        break;
    }
    case FAT32::Parser::Operations::DeleteFile:
    {
        if (!fat->DeleteEntry(operation.args[0], false))
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to delete file %s\n", operation.args[0]);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 12;
        }
        break;
    }
    case FAT32::Parser::Operations::DeleteDirectory:
    {
        if (!fat->DeleteDir(operation.args[0]))
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to delete directory %s\n", operation.args[0]);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 13;
        }
        break;
    }
    case FAT32::Parser::Operations::RenameFile:
    {
        if (!fat->RenameEntry(operation.args[0], operation.args[1], false, &file))
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to rename file %s to %s\n", operation.args[0], operation.args[1]);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 14;
        }
        break;
    }
    case FAT32::Parser::Operations::RenameDirectory:
    {
        if (!fat->RenameEntry(operation.args[0], operation.args[1], true, &file))
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to rename directory %s to %s\n", operation.args[0], operation.args[1]);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 15;
        }
        break;
    }
    case FAT32::Parser::Operations::CopyFile:
    {
        FAT32::FAT::File newFile;
        if (!fat->CopyEntry(operation.args[0], operation.args[1], false, &newFile))
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to copy file %s to %s\n", operation.args[0], operation.args[1]);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 16;
        }
        break;
    }
    case FAT32::Parser::Operations::CopyDirectory:
    {
        FAT32::FAT::File newDir;
        if (!fat->CopyEntry(operation.args[0], operation.args[1], true, &newDir))
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to copy dir %s to %s\n", operation.args[0], operation.args[1]);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 17;
        }
        break;
    }
    case FAT32::Parser::Operations::MoveFile:
    {
        FAT32::FAT::File movedFile;
        if (!fat->MoveEntry(operation.args[0], operation.args[1], false, &movedFile))
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to move file %s to %s\n", operation.args[0], operation.args[1]);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 18;
        }
        break;
    }
    case FAT32::Parser::Operations::MoveDirectory:
    {
        FAT32::FAT::File movedDir;
        if (!fat->MoveEntry(operation.args[0], operation.args[1], true, &movedDir))
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to move dir %s to %s\n", operation.args[0], operation.args[1]);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 19;
        }
        break;
    }
    case FAT32::Parser::Operations::CopyFileFrom:
    {
        FILE *hostFile = fopen(operation.args[1], "wb");
        m_uint8_t buffer[SECTOR_SIZE];
        m_uint32_t read;
        while ((read = fat->ReadFile(&file, &buffer, sizeof(buffer))))
        {
            if (!hostFile)
            {
                printf("[FAT32-MAIN] [ERROR]: Failed to create file %s\n", operation.args[1]);
                delete (parser);
                delete (disk);
                delete (gpt);
                delete (fat);
                return 20;
            }
            if (fwrite(&buffer, read, 1, hostFile) != 1)
            {
                printf("[FAT32-MAIN] [ERROR]: Failed to write to file %s\n", operation.args[1]);
                fclose(hostFile);
                delete (parser);
                delete (disk);
                delete (gpt);
                delete (fat);
                return 21;
            }
        }
        fclose(hostFile);
        break;
    }
    case FAT32::Parser::Operations::CopyFileTo:
    {
        if (!fat->CreateEntry(operation.args[1], false, &file))
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to create file %s\n", operation.args[1]);
            return false;
        }
        FILE *hostFile = fopen(operation.args[0], "rb");
        if (!hostFile)
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to open file %s\n", operation.args[0]);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 22;
        }
        if (fseek(hostFile, 0, SEEK_END) != 0)
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to seek in file %s\n", operation.args[0]);
            fclose(hostFile);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 23;
        }
        uint32_t fileSize = ftell(hostFile);
        if (fileSize == -1UL)
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to get file %s position", operation.args[0]);
            fclose(hostFile);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 24;
        }
        if (fseek(hostFile, 0, SEEK_SET) != 0)
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to seek in file %s\n", operation.args[0]);
            fclose(hostFile);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 25;
        }
        void *buffer = malloc(fileSize);
        if (!buffer)
        {
            printf("[FAT32-MAIN] [ERROR]: memory allocation failed\n");
            fclose(hostFile);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 26;
        }
        uint32_t read = fread(buffer, fileSize, 1, hostFile);
        if (read != 1)
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to read file %s\n", operation.args[0]);
            fclose(hostFile);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 27;
        }

        if (!fat->SetFileData(operation.args[1], buffer, fileSize))
        {
            printf("[FAT32-MAIN] [ERROR]: Failed to copy file %s\n", operation.args[0]);
            fclose(hostFile);
            delete (parser);
            delete (disk);
            delete (gpt);
            delete (fat);
            return 28;
        }
        fclose(hostFile);
        break;
    }
    case FAT32::Parser::Operations::MakeFS:
    {
        FAT32::FAT::MKFSDesc desc = {};
        if (strcmp(operation.args[2], "false") != 0)
        {
            m_uint16_t bps = strtoul(operation.args[2], nullptr, 10);
            if (bps != 0 && bps % SECTOR_SIZE == 0)
                desc.BytesPerSector = bps;
        }
        if (strcmp(operation.args[3], "false") != 0)
        {
            m_uint8_t driveNumber = strtoul(operation.args[3], nullptr, 10);
            if (driveNumber != 0)
                desc.DriveNumber = driveNumber;
        }
        if (strcmp(operation.args[4], "false") != 0)
        {
            m_uint16_t headCount = strtoul(operation.args[4], nullptr, 10);
            if (headCount != 0)
                desc.HeadCount = headCount;
        }
        if (strcmp(operation.args[5], "false") != 0)
        {
            m_uint8_t mediaDescType = strtoul(operation.args[5], nullptr, 10);
            if (mediaDescType != 0)
                desc.mediaDescType = mediaDescType;
        }
        if (strcmp(operation.args[6], "false") != 0)
        {
            m_uint16_t SectorsPerCluster = strtoul(operation.args[6], nullptr, 10);
            if (SectorsPerCluster != 0 && (SectorsPerCluster == 1 || SectorsPerCluster % 8 == 0))
                desc.SectorsPerCluster = SectorsPerCluster;
        }
        if (strcmp(operation.args[7], "false") != 0)
        {
            m_uint16_t SectorsPerTrack = strtoul(operation.args[7], nullptr, 10);
            if (SectorsPerTrack != 0)
                desc.SectorsPerTrack = SectorsPerTrack;
        }
        if (strcmp(operation.args[8], "false") != 0)
        {
            m_uint32_t VolumeID = strtoul(operation.args[8], nullptr, 10);
            if (VolumeID != 0)
                desc.VolumeID = VolumeID;
        }
        if (strcmp(operation.args[9], "false") != 0)
        {
            m_uint8_t VolumeLabel[11];
            size_t len = strlen(operation.args[9]);
            if (len > 11)
            {
                printf("[FAT32-MAIN] [ERROR]: Volume label cannot be longer than 11 characters\n");
                return 29;
            }
            memset(VolumeLabel, ' ', 11);
            memcpy(VolumeLabel, operation.args[9], len);
            memcpy(desc.VolumeLabel, VolumeLabel, 11);
        }
        if (!fat->mkfs(&desc))
            return 30;
        break;
    }
    default:
    {
        printf("[FAT32-MAIN] [WARN]: Operation is under developement\n");
        break;
    }
    }

    delete (parser);
    delete (disk);
    delete (gpt);
    delete (fat);

    return 0;
}