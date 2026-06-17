#include <Drivers/MMD/virt.hpp>

using namespace SysKrnl64::MMD;

MemoryAllocErrors VirtAlloc::Initialize(VirtAllocDesc* desc)
{
    if(!desc || !desc->System || !desc->physAlloc) 
    {
        printf("[SYSKRNL64] [VIRT ALLOC] [ERROR]: Invalid System table ptr specified\r\n");
        return MMD_INVALID_PARAMETER;
    }

    this->desc = *desc;

    // Allocate the table descriptor array with physical allocator

    size_t tableArrayBlocks = BLOCK_COUNT(VA_TOTAL_TABLES * sizeof(VA_TableEntry));
    uintptr_t tableArrayVirt = MapAddr + desc->System->memLayout.KrnlMemRegionSize;

    MemoryAllocErrors arrAllocRes = desc->physAlloc->AllocSparseBlocksToContiguousVirtualRange(tableArrayBlocks, tableArrayVirt, PTE_PRESENT | PTE_RW | PTE_NX);
    if(arrAllocRes != MMD_SUCCESS)
    {
        printf("[SYSKRNL64] [VIRT ALLOC] [ERROR]: Failed to allocate %llu blocks for VA Table descriptor array\r\n", tableArrayBlocks);
        return arrAllocRes;
    }

    tableArray = reinterpret_cast<VA_TableEntry*>(tableArrayVirt);

    // Allocate the initial table with physical allocator and set it up

    VA_TableEntry* initTableEntry = &tableArray[0];

    size_t tableBlocks = BLOCK_COUNT(sizeof(VA_Table));
    uintptr_t initTableVirt = reinterpret_cast<uintptr_t>(tableArray) + tableArrayBlocks * BLOCK_SIZE;

    MemoryAllocErrors initTableAllocRes = desc->physAlloc->AllocSparseBlocksToContiguousVirtualRange(tableBlocks, initTableVirt, PTE_PRESENT | PTE_RW | PTE_NX);
    if(initTableAllocRes != MMD_SUCCESS)
    {
        printf("[SYSKRNL64] [VIRT ALLOC] [ERROR]: Failed to allocate %llu blocks for initial VA Table\r\n", tableBlocks);
        return initTableAllocRes;
    }

    initTableEntry->tablePtr = reinterpret_cast<VA_Table*>(initTableVirt);
    initTableEntry->freeNodeCount = VA_NODES_IN_TABLE;
    
    SetupInitialFreeList(initTableEntry);

    // Mark the initial table, table desc array and kernel structures as used memory regions

    InsertNode(reinterpret_cast<uintptr_t>(initTableEntry->tablePtr), tableBlocks, VA_NODE_FLAG_USED | VA_NODE_FLAG_NO_EXECUTE_ACCESS | VA_NODE_FLAG_EXPANSION_TABLE, true);

    InsertNode(reinterpret_cast<uintptr_t>(tableArray), tableArrayBlocks, VA_NODE_FLAG_USED | VA_NODE_FLAG_NO_EXECUTE_ACCESS);

    // Mark the kernel structures as used

    for(size_t i = 0; i < sizeof(desc->System->kernelStructureRegions) / sizeof(desc->System->kernelStructureRegions[0]); i++)
    {
        KernelStructureRegion* region = &desc->System->kernelStructureRegions[i];

        if(!region->phys) break; // End of kernel structure region descriptors

        uint64_t flags = VA_NODE_FLAG_USED;
        if(region->writeProtected) flags |= VA_NODE_FLAG_NO_WRITE_ACCESS;
        if(region->execProtected) flags |= VA_NODE_FLAG_NO_EXECUTE_ACCESS;

        InsertNode(region->virt, region->totalPages, flags);

        if(region->guardPage) InsertNode(region->virt + region->totalPages * PAGE_SIZE, 1, VA_NODE_FLAG_USED | VA_NODE_FLAG_FENCE);
    }

    return MMD_SUCCESS;
}

MemoryAllocErrors VirtAlloc::InsertNode(uintptr_t base, size_t totalBlocks, uint64_t flags, bool skipTresholdCheck)
{
    if(!base || totalBlocks == 0) return MMD_INVALID_PARAMETER;

    VA_Node* node = nullptr;

    // Find free slot for the node

    for(size_t i = 0; i < VA_TOTAL_TABLES; i++)
    {
        VA_TableEntry* entry = &tableArray[i];
        if(!entry->tablePtr) 
        {
            // All tables checked, no free slots left
            return MMD_OUT_OF_MEMORY;
        }

        if(entry->freeNodeCount == 0 || !entry->nextFreeNodePtr) continue;

        // Use the free node described in the table descriptor and make the pointer inside the table descriptor point to the next free node, which's pointer is inside the node used

        VA_FreeNode* freeNode = entry->nextFreeNodePtr;

        entry->nextFreeNodePtr = freeNode->nextNode;
        entry->freeNodeCount--;

        node = reinterpret_cast<VA_Node*>(freeNode);

        node->base = base;
        node->totalBlocks = totalBlocks;
        node->flags = flags;
        node->tableDescIndex = i;

        break;
    }

    if(!node) return MMD_OUT_OF_MEMORY;

    // Manage Red-Black structures inside the node

    // 1: Insert Binary Search Tree structure

    VA_Node* y = nullptr;
    VA_Node* x = rootNode;

    while(x)
    {
        y = x;

        if(node->base < x->base) x = x->left;
        else x = x->right;
    }

    node->parent = y;

    if(!y) rootNode = node;
    else if(node->base < y->base) y->left = node;
    else y->right = node;

    node->left = nullptr;
    node->right = nullptr;

    // 2: Set the new node color initially as red

    SetRed(node);

    // Setup left & right and rotate the RB tree

    while(node != rootNode && IsRed(node->parent))
    {
        VA_Node* parent = node->parent;
        VA_Node* grandparent = parent->parent;

        if(!grandparent) break;

        if(parent == grandparent->left)
        {
            VA_Node* uncle = grandparent->right;

            if(IsRed(uncle))
            {
                SetBlack(parent);
                SetBlack(uncle);
                SetRed(grandparent);
                node = grandparent;
            }
            else
            {
                if(node == parent->right)
                {
                    node = parent;

                    VA_Node* y = node->right;
                    node->right = y->left;
                    if(y->left) y->left->parent = node;

                    y->parent = node->parent;
                    
                    if(!node->parent) rootNode = y;
                    else if(node == node->parent->left) node->parent->left = y;
                    else node->parent->right = y;

                    y->left = node;
                    node->parent = y;
                }

                SetBlack(parent);
                SetRed(grandparent);

                VA_Node* y = grandparent->left;

                grandparent->left = y->right;
                if(y->right) y->right->parent = grandparent;

                y->parent = grandparent->parent;

                if(!grandparent->parent) rootNode = y;
                else if(grandparent == grandparent->parent->left) grandparent->parent->left = y;
                else grandparent->parent->right = y;

                y->right = grandparent;
                grandparent->parent = y;
            }
        }
        else
        {
            VA_Node* uncle = grandparent->left;

            if(IsRed(uncle))
            {
                SetBlack(parent);
                SetBlack(uncle);
                SetRed(grandparent);
                node = grandparent;
            }
            else
            {
                if(node == parent->left)
                {
                    node = parent;
                    VA_Node* y = node->left;

                    node->left = y->right;
                    if(y->right) y->right->parent = node;

                    y->parent = node->parent;

                    if(!node->parent) rootNode = y;
                    else if(node == node->parent->right) node->parent->right = y;
                    else node->parent->left = y;

                    y->right = node;
                    node->parent = y;
                }

                SetBlack(parent);
                SetRed(grandparent);

                VA_Node* y = grandparent->right;

                grandparent->right = y->left;
                if(y->left) y->left->parent = grandparent;
                y->parent = grandparent->parent;

                if(!grandparent->parent) rootNode = y; 
                else if(grandparent == grandparent->parent->left) grandparent->parent->left = y;
                else grandparent->parent->right = y;

                y->left = grandparent;
                grandparent->parent = y;
            }
        }
    }

    SetBlack(rootNode); // Root must always be black


    if(skipTresholdCheck) return MMD_SUCCESS;


    // Check if total free nodes accross all tables is below a treshold

    uint64_t totalFreeNodes = 0;
    VA_TableEntry* nextFreeTableEntry = nullptr;

    for(size_t i = 0; i < VA_TOTAL_TABLES; i++)
    {
        VA_TableEntry* tableDesc = &tableArray[i];
        if(!tableDesc->tablePtr) {
            nextFreeTableEntry = &tableArray[i];
            break; // Iterated over all present tables
        }

        totalFreeNodes += tableDesc->freeNodeCount;
    }

    if(totalFreeNodes < VA_NODE_COUNT_TRESHOLD)
    {
        // Allocate a new table

        VA_TableEntry* newTableEntry = nextFreeTableEntry;

        size_t tableBlocks = BLOCK_COUNT(sizeof(VA_Table));

        auto virtSearchRes = FindFreeVirtualMemory(tableBlocks);
        if(!virtSearchRes)
        {
            printf("[SYSKRNL64] [VIRT ALLOC] [ERROR]: Failed to find a virtual address region to fit a new virtual range table\r\n");
            return virtSearchRes.error();
        }

        MemoryAllocErrors tableAllocRes = desc.physAlloc->AllocSparseBlocksToContiguousVirtualRange(tableBlocks, virtSearchRes.value(), PTE_PRESENT | PTE_RW | PTE_NX);
        if(tableAllocRes != MMD_SUCCESS)
        {
            printf("[SYSKRNL64] [VIRT ALLOC] [ERROR]: Failed to allocate %llu blocks for an expasion VA Table\r\n", tableBlocks);
            return tableAllocRes;
        }

        // Setup table desc and setup an initial free list in the table

        newTableEntry->tablePtr = reinterpret_cast<VA_Table*>(virtSearchRes.value());
        newTableEntry->freeNodeCount = VA_NODES_IN_TABLE;

        SetupInitialFreeList(newTableEntry);

        // Insert a node describing the new table
        // There should be a few free nodes left in the old tables, and because the system is first-fit, the descriptor node should be in the old tables. The only exception for a table descriptor being in the table itself is for the initial table

        InsertNode(reinterpret_cast<uintptr_t>(newTableEntry->tablePtr), tableBlocks, VA_NODE_FLAG_USED | VA_NODE_FLAG_NO_EXECUTE_ACCESS | VA_NODE_FLAG_EXPANSION_TABLE, true);
    }

    return MMD_SUCCESS;
}

void VirtAlloc::SetupInitialFreeList(VA_TableEntry* tableEntry)
{
    VA_Table* table = tableEntry->tablePtr;
    tableEntry->nextFreeNodePtr = reinterpret_cast<VA_FreeNode*>(&table->nodes[0]);

    for(size_t i = 0; i < VA_NODES_IN_TABLE - 1; i++)
    {
        VA_FreeNode* freeNode = reinterpret_cast<VA_FreeNode*>(&table->nodes[i]);
        freeNode->nextNode = reinterpret_cast<VA_FreeNode*>(&table->nodes[i + 1]);
    }

    VA_FreeNode* fNode = reinterpret_cast<VA_FreeNode*>(&table->nodes[VA_NODES_IN_TABLE - 1]);
    fNode->nextNode = nullptr;
}

std::expected<void*, MemoryAllocErrors> VirtAlloc::AllocateBlocks(size_t blockCount, uint64_t flags)
{
    if(blockCount == 0) return nullptr;

    // determine if allocating physical memory is needed
    // if the allocation is for MMIO, physical memory allocation is not needed
    // or if the allocation shouldn't backed by physical memory, physical memory allocation is not needed

    auto vmFindRes = FindFreeVirtualMemory(blockCount, flags & VA_NODE_FLAG_USER_ALLOC);
    if(!vmFindRes)
    {
        printf("[SYSKRNL64] [VIRT ALLOC] [ERROR]: Failed to find free virtual memory range for %llu blocks\r\n", blockCount);
        return std::unexpected<MemoryAllocErrors>(vmFindRes.error());
    }
    uintptr_t vaddr = vmFindRes.value();

    if(!(flags & VA_NODE_FLAG_MMIO) && !(flags & VA_NODE_FLAG_PHYSICALLY_NOT_BACKED))
    {
        // Allocate physical memory and map it to virtual

        uint64_t pageFlags = 0;
        if(flags & VA_NODE_FLAG_USED) pageFlags |= PTE_PRESENT;
        if(!(flags & VA_NODE_FLAG_NO_WRITE_ACCESS)) pageFlags |= PTE_RW;
        if(flags & VA_NODE_FLAG_NO_EXECUTE_ACCESS) pageFlags |= PTE_NX;
        if(flags & VA_NODE_FLAG_USER_ALLOC) pageFlags |= PTE_USER; 

        MemoryAllocErrors pAllocRes = desc.physAlloc->AllocSparseBlocksToContiguousVirtualRange(blockCount, vaddr, pageFlags);
        if(pAllocRes != MMD_SUCCESS)
        {
            printf("[SYSKRNL64] [VIRT ALLOC] [ERROR]: Failed to allocate physical memory\r\n");
            return std::unexpected<MemoryAllocErrors>(pAllocRes);
        }
    }

    // Insert node into region tables

    MemoryAllocErrors insertErr = InsertNode(vaddr, blockCount, flags);
    if(insertErr != MMD_SUCCESS) {
        printf("[SYSKRNL64] [VIRT ALLOC] [ERROR]: Failed to insert new virtual region descriptor node\r\n");
        return insertErr;
    }

    return reinterpret_cast<void*>(vaddr);
}

std::expected<uintptr_t, MemoryAllocErrors> VirtAlloc::FindFreeVirtualMemory(size_t blockCount, bool user)
{
    if(blockCount == 0) return std::unexpected<MemoryAllocErrors>(MMD_INVALID_PARAMETER);
    
    const size_t size = blockCount * BLOCK_SIZE;
    const uintptr_t rangeStart = user ? USERSPACE_VADDR_START : KERNEL_VADDR_START;
    const uintptr_t rangeEnd = user ? USERSPACE_VADDR_END : KERNEL_VADDR_END;

    if(!rootNode) return rangeStart;

    uintptr_t prevEnd = rangeStart;
    VA_Node* node = Min(rootNode);

    while(node)
    {
        uintptr_t nodeStart = node->base;
        uintptr_t nodeEnd = node->base + node->totalBlocks * BLOCK_SIZE;

        if(nodeEnd <= rangeStart)
        {
            node = Successor(node); // Skip nodes outside operative region
            continue;
        }

        if(nodeStart >= rangeEnd) break;

        if(nodeStart > prevEnd)
        {
            uintptr_t gap = node->base - prevEnd;

            if(gap >= size && prevEnd + size <= rangeEnd) return prevEnd;
        }

        if(nodeEnd > prevEnd) prevEnd = nodeEnd;

        node = Successor(node);
    }

    if(rangeEnd > prevEnd)
    {
        uintptr_t gap = rangeEnd - prevEnd;

        if(gap >= size) return prevEnd;
    }

    return std::unexpected<MemoryAllocErrors>(MMD_OUT_OF_MEMORY);
}

VA_Node* VirtAlloc::Min(VA_Node* node)
{
    if(!node) return nullptr;

    while(node->left) node = node->left;
    
    return node;
}

VA_Node* VirtAlloc::Successor(VA_Node* node)
{
    if(!node) return nullptr;

    if(node->right) return Min(node->right);

    VA_Node* parent = node->parent;

    while(parent && node == parent->right)
    {
        node = parent;
        parent = parent->parent;
    }

    return parent;
}

MemoryAllocErrors VirtAlloc::FreeBlocks(void* base)
{
    if(!base) return MMD_INVALID_PARAMETER;

    uintptr_t addr = reinterpret_cast<uintptr_t>(base);

    // Find node that describes the address using a BST traversal to get O(log n) lookup speed

    VA_Node* node = rootNode;

    while(node)
    {
        if(addr == node->base) break;

        if(addr < node->base) node = node->left;
        else node = node->right;
    }

    if(!node) return MMD_INVALID_PARAMETER;

    // Use the paging driver to get the physical addresses mapped to the region to free
    // And free physical blocks in runs, as physical blocks might not be contiguous
    // Only after that unmap the virtual memory
    // Only free if the region is physically backed in the flags and isn't MMIO
    if(!(node->flags & VA_NODE_FLAG_MMIO) && !(node->flags & VA_NODE_FLAG_PHYSICALLY_NOT_BACKED))
    {
        uintptr_t startVaddr = node->base;
        size_t blockCount = node->totalBlocks;
        
        uintptr_t prevPaddr = 0, runStartPaddr = 0;
        size_t runLen = 0;
        
        auto flushRun = [&]()
        {
            if(runLen == 0) return;
            desc.physAlloc->FreeBlocks(reinterpret_cast<void*>(runStartPaddr), runLen);
            runLen = 0;
        };
    
        for(size_t i = 0; i < blockCount; i++)
        {
            uintptr_t vaddr = startVaddr + i * BLOCK_SIZE;
            uintptr_t paddr = paging->GetPhys(vaddr);
            if(paddr == 0)
            {
                flushRun();
                continue;
            }
        
            if(runLen == 0)
            {
                runStartPaddr = paddr;
                runLen = 1;
            }
            else if(paddr == prevPaddr + BLOCK_SIZE) runLen++;
            else
            {
                flushRun();
                runStartPaddr = paddr;
                runLen = 1;
            }
        
            prevPaddr = paddr;
        }
    
        flushRun();
    
        paging->FreeArea(startVaddr, blockCount);
    }

    // Remove the node and update the red-black tree structure to match

    VA_Node* y = node;
    VA_Node* x = nullptr;

    bool yOriginalColorWasBlack = IsBlack(y);

    if(!node->left)
    {
        x = node->right;
        Transplant(node, node->right);
    }
    else if(!node->right)
    {
        x = node->left;
        Transplant(node, node->left);
    }
    else
    {
        y = Min(node->right);
        yOriginalColorWasBlack = IsBlack(y);

        x = y->right;

        if(y->parent == node) 
        {
            if(x) x->parent = y;
        }
        else
        {
            Transplant(y, y->right);
            y->right = node->right;
            y->right->parent = y;
        }

        Transplant(node, y);

        y->left = node->left;
        y->left->parent = y;

        if(IsBlack(node)) SetBlack(y);
        else SetRed(y);
    }

    if(yOriginalColorWasBlack) FixDelete(x);

    // Update the table descriptor and free list

    VA_TableEntry* entry = &tableArray[node->tableDescIndex];
    entry->freeNodeCount++;
    reinterpret_cast<VA_FreeNode*>(node)->nextNode = entry->nextFreeNodePtr;
    entry->nextFreeNodePtr = reinterpret_cast<VA_FreeNode*>(node);

    return MMD_SUCCESS;
}

void VirtAlloc::Transplant(VA_Node* u, VA_Node* v)
{
    if(!u->parent) rootNode = v;
    else if(u == u->parent->left) u->parent->left = v;
    else u->parent->right = v;

    if(v) v->parent = u->parent;
}

void VirtAlloc::FixDelete(VA_Node* x)
{
    while(x != rootNode && IsBlack(x))
    {
        VA_Node* parent = x ? x->parent : nullptr;
        if(!parent) break;

        if(x == parent->left)
        {
            VA_Node* w = parent->right;

            if(IsRed(w))
            {
                SetBlack(w);
                SetRed(parent);
                LeftRotate(parent);
                w = parent->right;
            }

            if(IsBlack(w->left) && IsBlack(w->right))
            {
                SetRed(w);
                x = parent;
            }
            else
            {
                if(IsBlack(w->right))
                {
                    if(w->left) SetBlack(w->left);
                    SetRed(w);
                    RightRotate(w);
                    w = parent->right;
                }

                if(w)
                {
                    if(IsRed(parent)) SetRed(w);
                    else SetBlack(w);

                    SetBlack(parent);
                    if(w->right) SetBlack(w->right);

                    LeftRotate(parent);
                }
                
                x = rootNode;
            }
        }
        else
        {
            VA_Node* w = parent->left;

            if(IsRed(w))
            {
                SetBlack(w);
                SetRed(parent);
                RightRotate(parent);
                w = parent->left;
            }

            if(IsBlack(w->left) && IsBlack(w->right))
            {
                SetRed(w);
                x = parent;
            }
            else
            {
                if(IsBlack(w->left))
                {
                    if(w->right) SetBlack(w->right);
                    SetRed(w);
                    LeftRotate(w);
                    w = parent->left;   
                }

                if(w)
                {
                    if(IsRed(parent)) SetRed(w);
                    else SetBlack(w);

                    SetBlack(parent);
                    if(w->left) SetBlack(w->left);

                    RightRotate(parent);
                }

                x = rootNode;
            }
        }
    }

    if(x) SetBlack(x);
}

void VirtAlloc::LeftRotate(VA_Node* x)
{
    VA_Node* y = x->right;
    if(!y) return;

    x->right = y->left; 

    if(y->left) y->left->parent = x;

    y->parent = x->parent;

    if(!x->parent) rootNode = y;
    else if(x == x->parent->left) x->parent->left = y;
    else x->parent->right = y;

    y->left = x;
    x->parent = y;
}

void VirtAlloc::RightRotate(VA_Node* y)
{
    VA_Node* x = y->left;
    if(!x) return;

    y->left = x->right;

    if(x->right) x->right->parent = y;

    x->parent = y->parent;

    if(!y->parent) rootNode = x;
    else if(y == y->parent->right) y->parent->right = x;
    else y->parent->left = x;

    x->right = y;
    y->parent = x;
}