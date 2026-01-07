#pragma once

#define MAX_ARGS                32

namespace FAT32
{
    namespace Parser
    {
        enum Operations {
            NONE = 0,
            ReadFile,
            ListContents,
            CreateFile,
            CreateDirectory,
            DeleteFile,
            DeleteDirectory,
            RenameFile,
            RenameDirectory,
            CopyFile,
            CopyDirectory,
            MoveFile,
            MoveDirectory,
            CopyFileTo,
            CopyFileFrom,
            MakeFS,
        };

        struct Args {
            bool Help, Version;
            LPCSTR Disk = nullptr;
            int PartitionIndex;
        };

        struct ScriptArg {
            char shortName;
            LPCSTR longName;
            bool NextArgValue;
        };

        struct OperationDesc {
            Operations operation;
            char args[MAX_ARGS][MAX_ARG_SIZE];
        };

        class Parser
        {
        public:
            Args ParseArgs(int argc, char** argv);
            Args ParseArgsScriptingMode(int argc, char** argv);
            OperationDesc getOperation();
            OperationDesc getOperationScriptingMode(int argc, char** argv, const ScriptArg argDesc[], size_t argDescCount);
            void ReadArg(LPCSTR prompt, LPSTR dest, size_t size);
        };
    }
}