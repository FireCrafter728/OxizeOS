#include <print.h>
#include <io.h>

#include <stdarg.h>
#include <stdbool.h>

const unsigned SCREEN_WIDTH = 80;
const unsigned SCREEN_HEIGHT = 25;
const uint8_t DEFAULT_COLOR = 0x7;

uint8_t* g_ScreenBuffer = (uint8_t*)0xB8000;
unsigned g_ScreenX = 0, g_ScreenY = 0;

void putchr(int x, int y, char c)
{
    g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)] = c;
}

void putcolor(int x, int y, uint8_t color)
{
    g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x) + 1] = color;
}

char getchr(int x, int y)
{
    return g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)];
}

uint8_t getcolor(int x, int y)
{
    return g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x) + 1];
}

void setcursor(int x, int y)
{
    int pos = y * SCREEN_WIDTH + x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void clrscr()
{
    for (unsigned y = 0; y < SCREEN_HEIGHT; y++)
        for (unsigned x = 0; x < SCREEN_WIDTH; x++)
        {
            putchr(x, y, '\0');
            putcolor(x, y, DEFAULT_COLOR);
        }

    g_ScreenX = 0;
    g_ScreenY = 0;
    setcursor(g_ScreenX, g_ScreenY);
}

void scrollback(unsigned lines)
{
    for (unsigned y = lines; y < SCREEN_HEIGHT; y++)
        for (unsigned x = 0; x < SCREEN_WIDTH; x++)
        {
            putchr(x, y - lines, getchr(x, y));
            putcolor(x, y - lines, getcolor(x, y));
        }

    for (unsigned y = SCREEN_HEIGHT - lines; y < SCREEN_HEIGHT; y++)
        for (unsigned x = 0; x < SCREEN_WIDTH; x++)
        {
            putchr(x, y, '\0');
            putcolor(x, y, DEFAULT_COLOR);
        }

    g_ScreenY -= lines;
}

void print_putc(char c)
{
    switch (c)
    {
        case '\n':
            g_ScreenY++;
            break;
    
        case '\t':
            for (unsigned i = 0; i < 4 - (g_ScreenX % 4); i++)
                print_putc(' ');
            break;

        case '\r':
            g_ScreenX = 0;
            break;

        default:
            putchr(g_ScreenX, g_ScreenY, c);
            g_ScreenX++;
            break;
    }

    if (g_ScreenX >= SCREEN_WIDTH)
    {
        g_ScreenY++;
        g_ScreenX = 0;
    }
    if (g_ScreenY >= SCREEN_HEIGHT)
        scrollback(1);

    setcursor(g_ScreenX, g_ScreenY);
}

void print_puts(const char* str)
{
    while(*str)
    {
        print_putc(*str);
        str++;
    }
}