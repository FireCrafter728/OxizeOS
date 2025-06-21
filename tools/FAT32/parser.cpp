#include <parser.hpp>
#include <stdio.h>
#include <stdlib.h>

using namespace FAT32::Parser;

Args Parser::ParseArgs(int argc, char **argv)
{
    Args args = {};
    LPCSTR NextArgValue = "";
    for (int i = 1; i < argc; i++)
    {
        LPCSTR arg = argv[i];
        if (!Empty(NextArgValue))
        {
            if (strcmp(NextArgValue, "partition") == 0)
            {
                args.PartitionIndex = (m_uint8_t)strtoul(arg, nullptr, 10);
                NextArgValue = "";
            }
            continue;
        }
        else
        {
            if (strncmp(arg, "--", 2) == 0)
            {
                arg += 2;
                if (strcasecmp(arg, "help") == 0)
                    args.Help = true;
                else if (strcasecmp(arg, "version") == 0)
                    args.Version = true;
                else if (strcasecmp(arg, "partition") == 0)
                    NextArgValue = "partition";
                else
                {
                    printf("[PARSER] [ERROR]: Unknown argument %s\n", argv[i]);
                    exit(-1);
                }
            }
            else if (strncmp(arg, "-", 1) == 0)
            {
                arg++;
                while (*arg != '\0')
                {
                    if (*arg == 'h')
                        args.Help = true;
                    else if (*arg == 'v')
                        args.Version = true;
                    else if (*arg == 'p')
                        NextArgValue = "partition";
                    else
                    {
                        printf("[PARSER] [ERROR]: Unknown argument -%c\n", *arg);
                        exit(-1);
                    }
                    arg++;
                }
            }
            else if (args.Disk == nullptr)
                args.Disk = arg;
            else
            {
                printf("[PARSER] [ERROR]: More than one disk file specified\n");
                exit(-2);
            }
        }
    }

    return args;
}

Args Parser::ParseArgsScriptingMode(int argc, char **argv)
{
    Args args;
    LPCSTR NextArgValue = "";
    for (int i = 2; i < argc; i++)
    {
        LPCSTR arg = argv[i];
        if (!Empty(NextArgValue))
        {
            if (strcmp(NextArgValue, "partition") == 0)
            {
                args.PartitionIndex = (m_uint8_t)strtoul(arg, nullptr, 10);
                NextArgValue = "";
            }
            continue;
        }
        else
        {
            if (strncmp(arg, "--", 2) == 0)
            {
                arg += 2;
                if (strcasecmp(arg, "help") == 0)
                    args.Help = true;
                else if (strcasecmp(arg, "version") == 0)
                    args.Version = true;
                else if (strcasecmp(arg, "partition") == 0)
                    NextArgValue = "partition";
            }
            else if (strncmp(arg, "-", 1) == 0)
            {
                arg++;
                while (*arg != '\0')
                {
                    if (*arg == 'h')
                        args.Help = true;
                    else if (*arg == 'v')
                        args.Version = true;
                    else if (*arg == 'p')
                        NextArgValue = "partition";
                    arg++;
                }
            }
            else if (args.Disk == nullptr)
                args.Disk = arg;
        }
    }

    return args;
}

void Parser::ReadArg(LPCSTR prompt, LPSTR dest, size_t size)
{
    printf("%s", prompt);
    if (fgets(dest, size, stdin) != nullptr)
        dest[strcspn(dest, "\n")] = 0;
    else
    {
        printf("[PARSER] [ERROR]: Failed to retrieve string from stdin\n");
        exit(-2);
    }
}

OperationDesc Parser::getOperation()
{
    OperationDesc desc = {};
    char buffer[MAX_ARG_SIZE];
    int argp = 0;

    ReadArg("Enter Operation: ", buffer, MAX_ARG_SIZE);
    ClearWhitespace(buffer);

    struct OpInfo
    {
        const char *name;
        Operations op;
        int argCount;
        const char *prompt[2];
    };

    static const OpInfo operations[] = {
        {"readfile", Operations::ReadFile, 1, {"Enter file path: ", nullptr}},

        {"listfiles", Operations::ListContents, 1, {"Enter directory path: ", nullptr}},

        {"createfile", Operations::CreateFile, 1, {"Enter new file path: ", nullptr}},

        {"createdir", Operations::CreateDirectory, 1, {"Enter new directory path: ", nullptr}},

        {"deletefile", Operations::DeleteFile, 1, {"Enter file to delete path: ", nullptr}},

        {"deletedir", Operations::DeleteDirectory, 1, {"Enter directory to delete path: ", nullptr}},

        {"renamefile", Operations::RenameFile, 2, {"Enter file to rename path: ", "Enter new name: "}},

        {"renamedir", Operations::RenameDirectory, 2, {"Enter directory to rename path: ", "Enter new name: "}},

        {"copyfile", Operations::CopyFile, 2, {"Enter file to copy path: ", "Enter new path: "}},

        {"copydir", Operations::CopyDirectory, 2, {"Enter directory to copy path: ", "Enter new path: "}},

        {"movefile", Operations::MoveFile, 2, {"Enter file to move path: ", "Enter new path: "}},

        {"movedir", Operations::MoveDirectory, 2, {"Enter directory to move path: ", "Enter new path: "}},

        {"copyfileto", Operations::CopyFileTo, 2, {"Enter file to copy path: ", "Enter path where to copy file: "}},

        {"copyfilefrom", Operations::CopyFileFrom, 2, {"Enter file to copy path: ", "Enter path where to copy file: "}},
    };

    for (const auto &op : operations)
    {
        if (strcasecmp(buffer, op.name) == 0)
        {
            desc.operation = op.op;
            for (int i = 0; i < op.argCount; i++)
            {
                ZeroOut(buffer, MAX_ARG_SIZE);
                ReadArg(op.prompt[i], buffer, MAX_ARG_SIZE);
                TrimTrailingSlash(buffer);
                strcpy(desc.args[argp++], buffer);
            }
            return desc;
        }
    }
    printf("[PARSER] [ERROR]: Unknown operation %s\n", buffer);
    return desc;
}

OperationDesc Parser::getOperationScriptingMode(int argc, char **argv, const ScriptArg argDesc[], size_t argDescCount)
{
    OperationDesc operation;
    operation.operation = Operations::NONE;

    for (size_t i = 0; i < argDescCount; i++)
        strncpy(operation.args[i], "false", MAX_ARG_SIZE - 1);

    int argDescIndex = -1;
    LPCSTR NextArgValue = nullptr;

    for (int i = 2; i < argc; i++)
    {
        LPCSTR arg = argv[i];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "-v") == 0 || strcmp(arg, "--help") == 0 || strcmp(arg, "--version") == 0)
            continue;
        if (strcmp(arg, "-p") == 0 || strcmp(arg, "--partition") == 0)
        {
            i++;
            continue;
        }

        if (NextArgValue)
        {
            if (strcmp(NextArgValue, "operation") == 0)
            {
                static const struct Ops
                {
                    LPCSTR OperationStr;
                    Operations operation;
                } ops[] = {
                    {"rdfile", Operations::ReadFile},

                    {"dir", Operations::ListContents},

                    {"createfile", Operations::CreateFile},

                    {"mkdir", Operations::CreateDirectory},

                    {"delfile", Operations::DeleteFile},

                    {"deldir", Operations::DeleteDirectory},

                    {"renfile", Operations::RenameFile},

                    {"rendir", Operations::RenameDirectory},

                    {"cpfile", Operations::CopyFile},

                    {"cpdir", Operations::CopyDirectory},

                    {"mvfile", Operations::MoveFile},

                    {"mvdir", Operations::MoveDirectory},

                    {"diskcpy", Operations::CopyFileTo},

                    {"hostcpy", Operations::CopyFileFrom},

                    {"mkfs", Operations::MakeFS},
                };

                bool found = false;
                for (const auto &op : ops)
                {
                    if (strcmp(op.OperationStr, arg) == 0)
                    {
                        operation.operation = op.operation;
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    printf("[PARSER] [ERROR]: Unknown operation %s\n", arg);
                    return operation;
                }

                NextArgValue = nullptr;
                continue;
            }
        }
        if (argDescIndex != -1)
        {
            strncpy(operation.args[argDescIndex], arg, MAX_ARG_SIZE - 1);
            operation.args[argDescIndex][MAX_ARG_SIZE - 1] = '\0';
            argDescIndex = -1;
            continue;
        }

        if (strncmp(arg, "--", 2) == 0)
        {
            arg += 2;
            if (strcmp(arg, "operation") == 0)
            {
                NextArgValue = "operation";
                continue;
            }
            bool matched = false;
            for (size_t j = 0; j < argDescCount; j++)
            {
                if (strcmp(arg, argDesc[j].longName) == 0)
                {
                    matched = true;
                    if (argDesc[j].NextArgValue)
                        argDescIndex = (int)j;
                    else
                        strncpy(operation.args[j], "true", MAX_ARG_SIZE - 1);
                    break;
                }
            }

            if (!matched)
            {
                printf("[PARSER] [ERROR]: Unknown argument --%s\n", arg);
                return operation;
            }
            continue;
        }
        if (*arg == '-')
        {
            arg++;
            while (*arg)
            {
                bool matched = false;
                for (size_t j = 0; j < argDescCount; j++)
                {
                    if (*arg == argDesc[j].shortName)
                    {
                        matched = true;
                        if (argDesc[j].NextArgValue)
                        {
                            if (*(arg + 1) != '\0')
                            {
                                printf("[PARSER] [ERROR]: argument -%c requires a value\n", *arg);
                                return operation;
                            }
                            argDescIndex = (int)j;
                            break;
                        }
                        else
                        {
                            strncpy(operation.args[j], "true", MAX_ARG_SIZE - 1);
                            break;
                        }
                    }
                }

                if (!matched)
                {
                    printf("[PARSER] [ERROR]: Unknown argument -%c\n", *arg);
                    return operation;
                }
                if (argDescIndex != -1)
                    break;
                arg++;
            }
            continue;
        }
        continue; // skip disk image
    }

    if (argDescIndex != -1)
    {
        printf("[PARSER] [ERROR]: Expected value after argument --%s\n", argDesc[argDescIndex].longName);
        operation.operation = Operations::NONE;
    }

    return operation;
}