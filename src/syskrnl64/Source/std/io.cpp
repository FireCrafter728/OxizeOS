#include <io.hpp>
#include <stdio.hpp>

void HaltSystem()
{
    puts("System Halted\r\n");
    HaltSystemImpl();
}