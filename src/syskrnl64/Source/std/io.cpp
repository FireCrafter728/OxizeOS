#include <io.hpp>
#include <stdio.hpp>

void HaltSystem()
{
    printf("System Halted\r\n");
    HaltSystemImpl();
}