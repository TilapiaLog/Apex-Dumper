// Process memory dumper (uses Driver from MD.lib)
// Build: Open dumper.sln in Visual Studio and build
// Run: Run as Administrator
// Usage: dumper.exe <pid> [output_file] [size_bytes] [module_name]
//   pid          - target process ID (required)
//   output_file  - default "out.bin"
//   size_bytes   - bytes to dump (default: read from PE SizeOfImage = whole exe)
//   module_name  - optional, e.g. "r5apex_dx12.exe"; if omitted, use main module

#include "driver.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>

// drv is defined in MD.lib, use extern from driver.h

static const char* const DEMO_KEY = "AXRAESW35UG2UX38LPENW8";

#define CHUNK_SIZE     0x1000                // 4 KB per read

// Read SizeOfImage from PE header at module base (from target process via drv)
static size_t GetModuleSizeFromPe(u64 imagebase)
{
    char peBuf[0x1000];
    u64 nRead = 0;
    // moshi 3 = 切换CR3
    if (!drv.ReadProcessMemory(imagebase, (ptr)peBuf, sizeof(peBuf), 3, &nRead))
        return 0;
    if (sizeof(peBuf) < 0x40)
        return 0;
    DWORD e_lfanew = *(DWORD*)(peBuf + 0x3C);
    if (e_lfanew > sizeof(peBuf) - 100 || e_lfanew < 0x40)
        return 0;
    if (memcmp(peBuf + e_lfanew, "PE\0\0", 4) != 0)
        return 0;
    DWORD sizeOfImage = *(DWORD*)(peBuf + e_lfanew + 24 + 56);
    if (sizeOfImage == 0 || sizeOfImage > 0x10000000)  // sanity: 0 < size <= 256MB
        return 0;
    return (size_t)sizeOfImage;
}

int main(int argc, char* argv[])
{
    printf("=== Process Memory Dumper ===\n\n");

    if (argc < 2)
    {
        printf("Usage: %s <pid> [output_file] [size_bytes] [module_name]\n", argv[0]);
        printf("  pid         - target process ID (required)\n");
        printf("  output_file - default: out.bin\n");
        printf("  size_bytes  - default: from PE SizeOfImage (whole exe)\n");
        printf("  module_name - optional, e.g. r5apex_dx12.exe; default: main module\n\n");
        printf("Press Enter to exit...");
        getchar();
        return -1;
    }

    unsigned int pid = (unsigned int)atoi(argv[1]);
    const char* outFile = (argc >= 3) ? argv[2] : "out.bin";
    size_t imageSize = (argc >= 4) ? (size_t)strtoull(argv[3], NULL, 0) : 0;  // 0 = use PE SizeOfImage
    const char* moduleName = (argc >= 5) ? argv[4] : NULL;

    if (pid == 0)
    {
        printf("Invalid PID.\n");
        printf("Press Enter to exit...");
        getchar();
        return -1;
    }

    printf("[1] Loading driver...\n");
    if (!drv.Loaddriver(DEMO_KEY))
    {
        printf("    Load failed. Run as Administrator and ensure driver files are deployed.\n");
        printf("Press Enter to exit...");
        getchar();
        return -1;
    }
    printf("    Load OK.\n\n");

    printf("[2] Setting target process PID = %u\n", pid);
    if (!drv.proceint(pid))
    {
        printf("    proceint failed. Check if PID exists and you have permission.\n");
        printf("Press Enter to exit...");
        getchar();
        return -1;
    }
    printf("    OK.\n\n");

    printf("[3] Getting module base%s...\n", moduleName ? " (by name)" : " (main module)");
    u64 imagebase = moduleName ? drv.GetMoudleBaseEx(moduleName) : drv.GetMoudleBase();
    if (!imagebase)
    {
        printf("    %s failed.\n", moduleName ? "GetMoudleBaseEx" : "GetMoudleBase");
        printf("Press Enter to exit...");
        getchar();
        return -1;
    }
    printf("    Base = 0x%llX\n\n", (unsigned long long)imagebase);

    if (imageSize == 0)
    {
        printf("[4] Reading PE header for SizeOfImage...\n");
        imageSize = GetModuleSizeFromPe(imagebase);
        if (imageSize == 0)
        {
            printf("    Failed to get module size from PE header.\n");
            printf("Press Enter to exit...");
            getchar();
            return -1;
        }
        printf("    SizeOfImage = 0x%zX (%zu bytes)\n\n", imageSize, imageSize);
    }

    printf("[5] Allocating buffer (%zu bytes)...\n", imageSize);
    char* buffer = (char*)malloc(imageSize);
    if (!buffer)
    {
        printf("    malloc failed.\n");
        printf("Press Enter to exit...");
        getchar();
        return -1;
    }
    memset(buffer, 0, imageSize);

    printf("[6] Reading memory in 0x%X-byte chunks...\n", (unsigned)CHUNK_SIZE);
    size_t offset = 0;
    while (offset < imageSize)
    {
        u32 toRead = (u32)((imageSize - offset) > CHUNK_SIZE ? CHUNK_SIZE : (imageSize - offset));
        u64 nRead = 0;
        if (!drv.ReadProcessMemory((u64)(imagebase + offset), (ptr)(buffer + offset), toRead, 3, &nRead))
        {
            printf("    Read failed at offset 0x%zX (addr 0x%llX). Last error may be set.\n",
                offset, (unsigned long long)(imagebase + offset));
            break;
        }
        offset += toRead;
        if ((offset % (1024 * 1024)) == 0 && offset > 0)
            printf("    read 0x%zX / 0x%zX\n", offset, imageSize);
    }
    printf("    Read 0x%zX bytes total.\n\n", offset);

    printf("[7] Writing to file: %s\n", outFile);
    std::ofstream ofs(outFile, std::ios::binary);
    if (!ofs.is_open())
    {
        printf("    Failed to open file for writing.\n");
        free(buffer);
        printf("Press Enter to exit...");
        getchar();
        return -1;
    }
    ofs.write(buffer, offset);
    ofs.close();
    free(buffer);
    printf("    Wrote %zu bytes.\n\n", offset);

    printf("[8] Unloading driver...\n");
    if (!drv.Unloaddriver())
        printf("    Unload failed (non-fatal).\n");
    else
        printf("    Unload OK.\n");

    printf("\n=== Done. Output: %s ===\n", outFile);
    printf("Press Enter to exit...");
    getchar();
    return 0;
}
