#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |--------------------------------------------------------------------------------------------------------------------------------------| //
// | OxizeOS Kernel Implementation                                                                                                        | //
// | VIRTUAL MEMORY ALLOCATOR: A virtual address space allocator that uses a Red-Black tree to store nodes that describe allocated ranges | //
// |--------------------------------------------------------------------------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <SysTable.hpp>

#include <stdint.hpp>
#include <expected>

#include <Drivers/MMD/memdefs.hpp>
#include <Drivers/MMD/phys.hpp>
#include <Drivers/Paging/paging.hpp>


namespace SysKrnl64
{
    namespace MMD
    {
        constexpr uint64_t VA_NODES_IN_TABLE = 4096;
        constexpr uint64_t VA_TOTAL_TABLES = 512;
        constexpr uint64_t VA_NODE_COUNT_TRESHOLD = 16; // Amount of nodes total left needed to be under to add an extra table

        enum VA_NodeFlags : uint64_t
        {
            // Bit 0: Used or Free node? 0: free, 1: used
            VA_NODE_FLAG_USED = (1ULL << 0),

            // Bit 1: Generic Memory or MMIO? 0: generic, 1: MMIO
            VA_NODE_FLAG_MMIO = (1ULL << 1),

            // Bit 2: If generic memory, is it backed by physical memory or not? 0: backed, 1: not backed
            VA_NODE_FLAG_PHYSICALLY_NOT_BACKED = (1ULL << 2),

            // Bit 3: If generic memory, is it ring 3 or ring 0? 0: kernel, 1: user
            VA_NODE_FLAG_USER_ALLOC = (1ULL << 3),

            // Bit 4: Read allowed? 0: allowed, 1: not allowed
            VA_NODE_FLAG_NO_READ_ACCESS = (1ULL << 4),

            // Bit 5: Write allowed? 0: allowed, 1: not allowed
            VA_NODE_FLAG_NO_WRITE_ACCESS = (1ULL << 5),

            // Bit 6: Execute allowed? 0: allowed, 1: not allowed
            VA_NODE_FLAG_NO_EXECUTE_ACCESS = (1ULL << 6),
            
            // Bit 7: Is it an expansion table region desc? 0: no, 1: yes
            VA_NODE_FLAG_EXPANSION_TABLE = (1ULL << 7),

            // Bit 8: Is it a fence region(special region that forces #PF to protect from memory corruption)? 0: no, 1: yes
            VA_NODE_FLAG_FENCE = (1ULL << 8),

            // Bit 9(allocator-managed, not specified by higher levels): RB Color? 0: black, 1: red
            VA_NODE_FLAG_RB_COLOR_RED = (1ULL << 9),
        };
        
        struct PACK VA_Node
        {
            uintptr_t base;
            size_t totalBlocks;
            uint64_t flags;

            VA_Node* left;
            VA_Node* right;
            VA_Node* parent;

            uint16_t tableDescIndex;
        };

        struct PACK VA_FreeNode
        {
            VA_FreeNode* nextNode;
        };

        struct PACK VA_Table
        {
            VA_Node nodes[VA_NODES_IN_TABLE];
        };

        struct PACK VA_TableEntry
        {
            VA_Table* tablePtr;
            VA_FreeNode* nextFreeNodePtr;
            uint64_t freeNodeCount;
        };

        struct VirtAllocDesc
        {
            PhysAlloc* physAlloc;
            SystemTable* System;
        };

        class VirtAlloc
        {
        public:
            VirtAlloc() = default;
            MemoryAllocErrors Initialize(VirtAllocDesc* desc);
            std::expected<void*, MemoryAllocErrors> AllocateBlocks(size_t blockCount, uint64_t flags);
            MemoryAllocErrors FreeBlocks(void* base);
        private:
            VirtAllocDesc desc;
            VA_TableEntry* tableArray;
            VA_Node* rootNode = nullptr;
            MemoryAllocErrors InsertNode(uintptr_t base, size_t totalBlocks, uint64_t flags, bool skipTresholdCheck = false);
            std::expected<uintptr_t, MemoryAllocErrors> FindFreeVirtualMemory(size_t blockCount, bool user = false);
            void SetupInitialFreeList(VA_TableEntry* tableEntry);
            inline bool IsRed(VA_Node* node) { return node && (node->flags & VA_NODE_FLAG_RB_COLOR_RED);}
            inline bool IsBlack(VA_Node* node) { return !node && !(node->flags & VA_NODE_FLAG_RB_COLOR_RED);}
            inline void SetRed(VA_Node* node) { node->flags |= VA_NODE_FLAG_RB_COLOR_RED;}
            inline void SetBlack(VA_Node* node) { node->flags &= ~VA_NODE_FLAG_RB_COLOR_RED;}
            VA_Node* Min(VA_Node* node);
            VA_Node* Successor(VA_Node* node);
            void Transplant(VA_Node* u, VA_Node* v);
            void FixDelete(VA_Node* x);
            void LeftRotate(VA_Node* x);
            void RightRotate(VA_Node* y);
        };
    }
}