#include <Drivers/MMD/mmd.hpp>

using namespace SysKrnl64::MMD;

MMD::MMD(MMIO* mmio, KRNL* krnl)
{
    if(!Initialize(mmio, krnl)) HaltSystem();
}

bool MMD::Initialize(MMIO* mmio, KRNL* krnl)
{
    if(!mmio || !krnl) {
        printf("[SYSKRNL64] [MMD] [ERROR]: Invalid arguments\r\n");   
        return false;
    }

    this->mmio = mmio;
    this->krnl = krnl;

    return true;
}

void* MMD::malloc(size_t blocks, MemoryTypes mt, flags_t flags)
{
    switch(mt)
    {
        case MT_MMIO:
        {
            return this->mmio->Allocate(blocks);
        }
        case MT_KRNL:
        {
            if(flags != 0) return this->krnl->Allocate(blocks, flags);
            return this->krnl->Allocate(blocks);
        }
        default: break;
    }

    return nullptr;
}

void MMD::free(void* ptr, size_t blocks, MemoryTypes mt)
{
    switch(mt)
    {
        case MT_KRNL:
        {
            this->krnl->Free(ptr, blocks);
        }
        default: break;
    }
}

void MMD::freeBlocks(size_t blocks, MemoryTypes mt)
{
    switch(mt)
    {
        case MT_MMIO:
        {
            this->mmio->Free(blocks);
        }
        default: break;
    }
}