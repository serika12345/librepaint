// SPDX-License-Identifier: GPL-2.0-or-later
#include "model.h"
#include <iostream>
#include <stdexcept>

void expect(uint32_t actual, uint32_t expected, const char *operation)
{
    if (actual != expected) {
        std::cerr << operation << ": expected " << std::hex << expected
                  << ", actual " << actual << '\n';
        throw std::runtime_error(operation);
    }
}

int main()
{
    try {
        // Encoded RGBA8, premultiplied, R in the least significant byte.
        expect(poc::over(0xffff0000, 0xff0000ff), 0xff0000ff, "opaque red replaces blue");
        expect(poc::over(0xffff0000, 0), 0xffff0000, "transparent paint preserves blue");
        expect(poc::over(0xffff0000, 0x80000080), 0xff7f0080, "half red over blue");
        expect(poc::shade(0, 64, 64, 64, 64, 16, 255, 0x000000ff),
               0xff0000ff, "brush center paints across tile boundary");
        expect(poc::shade(0xffff0000, 80, 64, 64, 64, 16, 255, 0x000000ff),
               0xffff0000, "brush boundary preserves destination");
        expect(poc::shade(0, 72, 64, 64, 64, 16, 255, 0x000000ff),
               0xbf0000bf, "radial coverage produces 191 alpha");
        std::cout << "pixel contract passed\n";
    } catch (const std::exception &) { return 1; }
}
