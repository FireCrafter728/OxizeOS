extern char **environ;

struct CfgArg
{
    std::string key, value;
};

bool GetArgs(std::vector<Token> tokens, std::vector<CfgArg>& out);
bool isSeparator(const std::string& str);
bool ExecuteProcess(const std::string& exec, const std::vector<std::string>& args);


std::string ToLowerStr(const std::string& s)
{
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return out;
}

int main(int argc, char** argv)
{
    if(argc < 2) {
        fprintf(stderr, "Syntax: %s <image.conf>\n", argv[0]);
        return -1;
    }

    printf("\x1b[1m\x1b[91m---------------------------------------------------------------------\x1b[0m\n");
    printf("\x1b[1m\x1b[33m[IMAGE]      \x1b[91m[Creating Image]:\x1b[0m\n");

    std::ifstream configFile(argv[1]);
    if(!configFile.is_open()) {
        fprintf(stderr, "Failed to open configuration file %s\n", argv[1]);
        return -2;
    }

    std::string line;

    Tokenizer token;

    std::filesystem::path imagePath = "";

    enum OperationTypes
    {
        DISK_INIT,
        MKGPT,
        MKPART,
        MKFS,
        MKDIR,
        COPY
    };

    struct Operation
    {
        OperationTypes opType;
        std::vector<std::string> args;
        std::string printfString;
    };

    struct Letter
    {
        std::string letter;
        uint32_t partIndex;
    };

    std::vector<Letter> letters;
    Letter letter;

    std::vector<Operation> ops;
    Operation op;

    size_t lineIdx = 0;

    while(std::getline(configFile, line))
    {
        lineIdx++;
        if(line.empty()) continue;

        std::vector<Token> tokens;
        if(!token.Tokenize(line, tokens)) {
            fprintf(stderr, "Failed to tokenize line %s at index %lu\n", line.c_str(), lineIdx);
            return -3;
        }

        if(tokens.empty()) continue;

        Token Command = tokens[0];

        if(Command.type != TokenTypes::String) {
            fprintf(stderr, "Command in config file must be a string value(%lu)\n", lineIdx);
            return -4;
        }

        if(Command.value == "DISK_IMAGE") {
            if(tokens.size() < 3 || tokens[1].value != "=") {
                fprintf(stderr, "Invalid disk image specification in config file at line %lu\n", lineIdx);
                return -5;
            }

            imagePath = tokens[2].value;

            printf("\x1b[1m\x1b[33m[IMAGE]      \x1b[96mOutput image: %s\x1b[0m\n", imagePath.c_str());

            continue;
        }

        else if(Command.value == "DISK_INIT") {
            if(tokens.size() < 10 || tokens[1].value != "(") {
                fprintf(stderr, "Invalid DISK_INIT operation at line %lu\n", lineIdx);
                return -6;
            }

            std::vector<CfgArg> args;
            if(!GetArgs(tokens, args)) {
                fprintf(stderr, "Failed to get command %s args at line %lu\n", Command.value.c_str(), lineIdx);
                return -7;
            }

            op.opType = DISK_INIT;
            op.args.clear();
            op.args.resize(5);
            op.args[0] = "create";
            op.args[1] = "-f";
            op.args[3] = imagePath;

            for(CfgArg& arg : args)
            {
                if(arg.key == "format") op.args[2] = ToLowerStr(arg.value);
                else if(arg.key == "size") op.args[4] = arg.value;
                else {
                    fprintf(stderr, "Invalid DISK_INIT argument key %s at line %lu", arg.value.c_str(), lineIdx);
                    return -11;
                }
            }

            op.printfString = "\x1b[1m\x1b[33m[IMAGE]      \x1b[96mCreating disk with format: " + op.args[2] + ", size: " + op.args[4] + "\x1b[0m\n";

            ops.push_back(op);

            op.opType = MKGPT;
            op.args.clear();
            op.args.push_back(imagePath);
            op.args.push_back("mkgpt");

            op.printfString = "\x1b[1m\x1b[33m[IMAGE]      \x1b[96mFormatting disk with GPT\x1b[0m\n";

            ops.push_back(op);
            continue;
        }
        else if(Command.value == "MKPART") {
            if(tokens.size() < 26) {
                fprintf(stderr, "Invalid MKPART operation at line %lu\n", lineIdx);
                return -8;
            }

            std::vector<CfgArg> args;
            if(!GetArgs(tokens, args)) {
                fprintf(stderr, "Failed to get command MKPART args at line %lu\n", lineIdx);
                return -9;
            }

            op.opType = MKPART;
            op.args.clear();
            op.args.resize(7);

            op.args[0] = imagePath;
            op.args[1] = "MKPART";

            for(CfgArg& arg : args)
            {
                if(arg.key == "index") op.args[6] = arg.value;
                else if(arg.key == "label") op.args[2] = arg.value;
                else if(arg.key == "code") op.args[3] = arg.value;
                else if(arg.key == "start") op.args[4] = arg.value;
                else if(arg.key == "sectors") op.args[5] = arg.value;
                else if(arg.key == "letter") {
                    letter.letter = arg.value;
                    letter.partIndex = std::stoul(op.args[6]);
                    letters.push_back(letter);
                } else {
                    fprintf(stderr, "Invalid MKPART argument key %s at line %lu\n", arg.key.c_str(), lineIdx);
                    return -10;
                }
            }

            op.printfString = "\x1b[1m\x1b[33m[IMAGE]      \x1b[96mCreating partition on disk with label: \"" + op.args[2] + "\", code: " + op.args[3] + ", part index: " + op.args[6] + ", start LBA: " + op.args[4] + ", sector count: " + op.args[5] + "\x1b[0m\n";

            ops.push_back(op);
            continue;
        }
        else if(Command.value == "MKFS") {
            if(tokens.size() < 10) {
                fprintf(stderr, "Invalid MKFS operation at line %lu\n", lineIdx);
                return -12;
            }

            std::vector<CfgArg> args;
            if(!GetArgs(tokens, args)) {
                fprintf(stderr, "Failed to get command MKFS args at line %lu\n", lineIdx);
                return -13;
            }

            op.opType = MKFS;
            op.args.clear();
            op.args.resize(8);

            op.args[0] = "-s";
            op.args[1] = imagePath;
            op.args[4] = "--operation";
            op.args[5] = "mkfs";

            for(CfgArg& arg : args)
            {
                if(arg.key == "part") {
                    op.args[2] = "-p";
                    op.args[3] = arg.value;
                } else if(arg.key == "label") {
                    op.args[6] = "-l";
                    op.args[7] = arg.value;
                } else {
                    fprintf(stderr, "Invalid MKFS argument key %s at line %lu\n", arg.key.c_str(), lineIdx);
                    return -14;
                }
            }

            op.printfString = "\x1b[1m\x1b[33m[IMAGE]      \x1b[96mFormatting partition at index: " + op.args[3] + " with FAT32 and label: \"" + op.args[7] + "\"\x1b[0m\n"; 

            ops.push_back(op);
            continue;
        }
        else if(Command.value == "MKDIR") {
            if(tokens.size() < 6) {
                fprintf(stderr, "Invalid MKDIR operation at line %lu\n", lineIdx);
                return -15;
            }

            std::vector<CfgArg> args;
            if(!GetArgs(tokens, args)) {
                fprintf(stderr, "Failed to get command MKDIR args at line %lu\n", lineIdx);
                return -16;
            }

            op.opType = MKDIR;
            op.args.clear();
            op.args.resize(8);

            op.args[0] = "-s";
            op.args[1] = imagePath;
            op.args[2] = "-p";
            op.args[4] = "--operation";
            op.args[5] = "mkdir";
            op.args[6] = "-i";

            std::string driveLetter;

            for(CfgArg& arg : args)
            {
                if(arg.key == "path") {
                    if(!isupper(static_cast<unsigned char>(arg.value[0])) || arg.value[1] != ':' || (arg.value[2] != '/' && arg.value[2] != '\\')) {
                        fprintf(stderr, "Invalid MKDIR path at line %lu. Path must start with a drive letter, for example: C:\\testdir\n", lineIdx);
                        return -18;
                    }

                    driveLetter = arg.value.substr(0, 2);
                    Letter* ltr = nullptr;
                    for(size_t i = 0; i < letters.size(); i++)
                    {
                        Letter* l = &letters[i];
                        if(l->letter == driveLetter) {
                            ltr = l;
                            break;
                        }
                    }

                    if(!ltr) {
                        fprintf(stderr, "Unregistered drive letter %s at line %lu. Make sure you have registered the letter inside the MKPART command using arg `letter=\"<letter>:\"`\n", driveLetter.c_str(), lineIdx);
                        return -19;
                    }

                    op.args[3] = std::to_string(ltr->partIndex);

                    op.args[7] = arg.value.substr(2);
                } else {
                    fprintf(stderr, "Invalid MKDIR argument key %s at line %lu\n", arg.key.c_str(), lineIdx);
                    return -17;
                }
            }

            op.printfString = "\x1b[1m\x1b[33m[IMAGE]      \x1b[96mCreating directory with path: \"" + driveLetter + op.args[7] + "\"\x1b[0m\n";

            ops.push_back(op);
            continue;
        }
        else if(Command.value == "COPY") {
            if(tokens.size() < 10) {
                fprintf(stderr, "Invalid COPY operation at line %lu\n", lineIdx);
                return -20;
            }

            std::vector<CfgArg> args;
            if(!GetArgs(tokens, args)) {
                fprintf(stderr, "Failed to get command COPY args at line %lu\n", lineIdx);
                return -21;
            }

            op.opType = COPY;
            op.args.clear();
            op.args.resize(10);

            op.args[0] = "-s";
            op.args[1] = imagePath;
            op.args[2] = "-p";
            op.args[4] = "--operation";
            op.args[5] = "diskcpy";
            op.args[6] = "-i";
            op.args[8] = "-o";

            std::string driveLetter;

            for(CfgArg& arg : args)
            {
                if(arg.key == "src") op.args[7] = arg.value;
                else if(arg.key == "dst") {
                    if(!isupper(static_cast<unsigned char>(arg.value[0])) || arg.value[1] != ':' || (arg.value[2] != '/' && arg.value[2] != '\\')) {
                        fprintf(stderr, "Invalid COPY dest path at line %lu. Path must start with a drive letter, for example: C:\\test.txt\n", lineIdx);
                        return -22;
                    }

                    driveLetter = arg.value.substr(0, 2);
                    Letter* ltr = nullptr;
                    for(size_t i = 0; i < letters.size(); i++)
                    {
                        Letter* l = &letters[i];
                        if(l->letter == driveLetter) {
                            ltr = l;
                            break;
                        }
                    }

                    if(!ltr) {
                        fprintf(stderr, "Unregistered drive letter %s at line %lu. Make sure you have registered the letter inside the MKPART command using arg `letter=\"<letter>:\"`\n", driveLetter.c_str(), lineIdx);
                        return -23;
                    }

                    op.args[3] = std::to_string(ltr->partIndex);

                    op.args[9] = arg.value.substr(2);
                } else {
                    fprintf(stderr, "Invalid COPY argument key %s at line %lu\n", arg.key.c_str(), lineIdx);
                    return -24;
                }
            }

            op.printfString = "\x1b[1m\x1b[33m[IMAGE]      \x1b[96mCopying file from host at path: \"" + op.args[7] + "\" to disk at path: \"" + driveLetter + op.args[9] + "\"\x1b[0m\n";

            ops.push_back(op);
            continue;
        }
        else {
            fprintf(stderr, "Invalid operation %s at line %lu\n", Command.value.c_str(), lineIdx);
            return -25;
        }
    }

    for(Operation& op : ops)
    {
        std::string operation;

        // Convert type to operation string
        switch(op.opType)
        {
            case DISK_INIT: {
                operation = "/usr/bin/qemu-img";
                break;
            }
            case MKGPT:
            case MKPART: {
                operation = "output/gpt";
                break;
            }
            case MKDIR:
            case COPY:
            case MKFS: {
                operation = "output/fat";
                break;
            }
        }

        printf(op.printfString.c_str());
        
        if(!ExecuteProcess(operation, op.args)) {
            fprintf(stderr, "Failed to execute operation %s\n", operation.c_str());
            return -26;
        }
    }

    return 0;
}

bool GetArgs(std::vector<Token> tokens, std::vector<CfgArg>& out)
{
    bool start = false;
    bool key = false;
    bool separator = false;
    CfgArg arg;
    for(Token& token : tokens)
    {
        if(!start) {
            if(token.value == "(") {
                start = true;
                key = true;
            }
            continue;
        }

        if(key) {
            if(isSeparator(token.value)) {
                fprintf(stderr, "An argument key was expected, but got separator\n");
                return false;
            }
            arg.key = token.value;
            key = false;
            separator = true;
            continue;
        } else if(separator) {
            if(!isSeparator(token.value)) {
                fprintf(stderr, "A separator was expected, but something else was received\n");
                return false;
            }
            separator = false;
            if(token.value == ")") return true;
            else if(token.value == ",") key = true;
            continue;
        } else {
            if(isSeparator(token.value)) {
                fprintf(stderr, "An argument value was expected, but got separator\n");
                return false;
            }
            arg.value = token.value;
            out.push_back(arg);
            separator = true;
        }
    }

    fprintf(stderr, "No closing parenthesis found to close command args\n");
    return false;
}

bool isSeparator(const std::string& str)
{
    if(str == "," || str == "=" || str == "(" || str == ")") return true;
    return false;
}

bool ExecuteProcess(const std::string& exec, const std::vector<std::string>& args)
{
    std::vector<char*> argv;
    argv.reserve(args.size() + 2);

    argv.push_back(const_cast<char*>(exec.c_str()));

    for(const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));

    argv.push_back(nullptr);

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    int fd = open("/dev/null", O_WRONLY);
    if(fd < 0) {
        fprintf(stderr, "Failed to open /dev/null for stdout redirection\n");
        return false;
    }

    posix_spawn_file_actions_adddup2(&actions, fd, STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, fd);

    pid_t pid;

    int result = posix_spawn(&pid, exec.c_str(), &actions, nullptr, argv.data(), environ);
    if(result != 0) {
        fprintf(stderr, "Failed to call posix_spawn() for command %s: %d\n", exec.c_str(), result);
        return false;
    }

    int status;
    while(true) {
        pid_t w = waitpid(pid, &status, 0);

        if(w == pid) break;

        if(w == -1) {
            if(errno == EINTR) continue;
            fprintf(stderr, "Fault: Executed Program signaled\n");
            return false;
        }
    }
    
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}