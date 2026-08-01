// SPDX-License-Identifier: GPL-3.0-or-later

#include <gpt.hpp>

int main(int argc, char** argv)
{
    // Syntax: gpt <Image File> <command> [flags]

    if(argc < 3) {
        fprintf(stderr, "Syntax: %s <Image File> <Command> [flags]\n", argv[0]);
        return 1;
    }

    FILE* Image = fopen(argv[1], "rb+");

    if(!Image) {
        fprintf(stderr, "Failed to open image file %s\n", argv[1]);
        return 2;
    }

    std::string cmd = argv[2];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c) { return std::toupper(c); });

    GPT gpt;
    
    if(cmd == "MKGPT") {
        return gpt.CreateGPT(Image) ? 0 : 4;
    } else if(cmd == "MKPART") {
        GPT_PartitionEntry partEntry = {};

        // Flag syntax: <Name> <16-byte hex part type ID> <start LBA> <end LBA> <part Index>
        if(argc < 8) {
            fprintf(stderr, "Syntax: %s %s MKPART <Part Name> <Part Type ID> <Part Start LBA> <Part End LBA> <part Index>\n", argv[0], argv[1]);
            return 5;
        }

        std::string PartName = argv[3];
        std::string PartID = argv[4];
        size_t PartStartLBA = std::stoull(argv[5], nullptr, 0), PartEndLBA = std::stoull(argv[6], nullptr, 0);

        auto PartGUIDPair = GPTTypeMap.find(PartID);
        if(PartGUIDPair == GPTTypeMap.end()) {
            fprintf(stderr, "Unknown part type code %s\n", PartID.c_str());
            return 6;
        }

        GUID PartGUID = PartGUIDPair->second;

        partEntry.PartTypeGUID = PartGUID;
        partEntry.UniqueGUID = gpt.GenerateGUID();
        partEntry.StartLBA = PartStartLBA;
        partEntry.EndLBA = PartEndLBA;

        size_t max = sizeof(partEntry.PartitionName) / sizeof(uint16_t);

        uint8_t i = 0;
        for(; i < max && i < PartName.length(); i++)
            partEntry.PartitionName[i] = (uint16_t)PartName[i];
        
        for(; i < max; i++) partEntry.PartitionName[i] = 0;

        return gpt.CreatePart(Image, partEntry, std::stoul(argv[7])) ? 0 : 7;        
    } else {
        fprintf(stderr, "Unknown command %s\n", argv[2]);
        return 3;
    }
}