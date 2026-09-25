#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>
#include <cstdio>

namespace erase {

    __forceinline void erase_function(std::uint8_t* function) {
        if (!function) return;

        if (function[0] == 0xE9)
        {
            std::int32_t relative_offset = *reinterpret_cast<std::int32_t*>(function + 1);
            function = function + relative_offset + 5;
        }
        else if (function[0] == 0xEB)
        {
            std::int8_t relative_offset = *reinterpret_cast<std::int8_t*>(function + 1);
            function = function + relative_offset + 2;
        }
        else if (function[0] == 0xFF && function[1] == 0x25)
        {
            std::int32_t relative_offset = *reinterpret_cast<std::int32_t*>(function + 2);
            function = *reinterpret_cast<std::uint8_t**>(function + 6 + relative_offset);
        }

        std::uint8_t* start_addr = function;
        std::size_t bytes = 0;

        DWORD64 image_base = 0;
        PRUNTIME_FUNCTION rt_fn = RtlLookupFunctionEntry(
            reinterpret_cast<DWORD64>(function),
            &image_base,
            nullptr
        );

        if (rt_fn) {
            start_addr = reinterpret_cast<std::uint8_t*>(image_base + rt_fn->BeginAddress);
            bytes = rt_fn->EndAddress - rt_fn->BeginAddress;
        }
        else {
            for (std::size_t i = 0; i < 0x500; ++i) {
                if (function[i] == 0xC3) {
                    bytes = i + 1;
                    break;
                }
            }
        }

        if (bytes == 0) {
            printf("[-] Error: No se pudo calcular el tamano de la funcion.\r\n");
            return;
        }
      
        DWORD old_protect;
        if (VirtualProtect(start_addr, bytes, PAGE_EXECUTE_READWRITE, &old_protect)) {
            std::memset(start_addr, 0x00, bytes);
            VirtualProtect(start_addr, bytes, old_protect, &old_protect);
        }
    }
}

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define erase_fn(a)                                                            \
  constexpr auto CONCAT(w, __LINE__) = &a;                                     \
  erase::erase_function(                                                \
      reinterpret_cast<std::uint8_t *>((void *&)CONCAT(w, __LINE__)))


#define erase_end
