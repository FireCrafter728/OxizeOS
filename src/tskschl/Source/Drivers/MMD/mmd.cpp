#include <Drivers/MMD/mmd.hpp>

using namespace TskSchl::MMD;

MMD::MMD(MMIO* mmio, KRNL* krnl)
{
    if(!Initialize(mmio, krnl)) HaltSystem();
}

bool MMD::Initialize(MMIO* mmio, KRNL* krnl)
{
    if(!mmio || !krnl) {
        printf("[TSKSCHL] [MMD] [ERROR]: Invalid arguments\r\n");   
        return false;
    }

    this->mmio = mmio;
    this->krnl = krnl;

    return true;
}

void* MMD::malloc(size_t blocks, MemoryTypes mt)
{
    switch(mt)
    {
        case MT_MMIO:
        {
            return this->mmio->Allocate(blocks);
        }
        case MT_KRNL:
        {
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