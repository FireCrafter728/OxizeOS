#include <Drivers/MMD/mmio.hpp>

using namespace SysKrnl64::MMD;

MMIO::MMIO(uintptr_t VirtStart, size_t length)
{
    if(!Initialize(VirtStart, length)) HaltSystem(); 
}

bool MMIO::Initialize(uintptr_t VirtStart, size_t length)
{
    if(!VirtStart || !length) {
        printf("[SYSKRNL64] [MMD-MMIO] [ERROR]: Invalid args\r\n");
        return false;
    }

    this->ptr = reinterpret_cast<uint8_t*>(VirtStart);
    this->end = reinterpret_cast<uint8_t*>(VirtStart + length);

    return true;
}

void* MMIO::Allocate(size_t Blocks)
{
    if(this->ptr > this->end - Blocks * BLOCK_SIZE) return nullptr;
    uint8_t* tmp = this->ptr;
    this->ptr += Blocks * BLOCK_SIZE;
    return tmp;
}

void MMIO::Free(size_t Blocks)
{
    this->ptr -= Blocks * BLOCK_SIZE;
}