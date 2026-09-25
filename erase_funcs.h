#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>
#include <cstdio>

namespace erase_my_dih {

    __forceinline void erase_function(std::uint8_t* function) {
        if (!function) return;

        // 1. Resolver Saltos/Jumps en x64 (Thunks de compilador, hooks o Incremental Linking)
        if (function[0] == 0xE9) // JMP rel32
        {
            std::int32_t relative_offset = *reinterpret_cast<std::int32_t*>(function + 1);
            function = function + relative_offset + 5;
        }
        else if (function[0] == 0xEB) // JMP rel8
        {
            std::int8_t relative_offset = *reinterpret_cast<std::int8_t*>(function + 1);
            function = function + relative_offset + 2;
        }
        else if (function[0] == 0xFF && function[1] == 0x25) // JMP [RIP + offset] (Muy común en x64)
        {
            std::int32_t relative_offset = *reinterpret_cast<std::int32_t*>(function + 2);
            function = *reinterpret_cast<std::uint8_t**>(function + 6 + relative_offset);
        }

        std::uint8_t* start_addr = function;
        std::size_t bytes = 0;

        // 2. Método Principal para x64: Consultar la tabla de funciones de Windows (.pdata)
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
            // 3. Método de Respaldo para "Leaf Functions" (Funciones simples sin unwind data en x64)
            // Escanea hasta encontrar una instrucción RET (0xC3)
            for (std::size_t i = 0; i < 0x500; ++i) {
                if (function[i] == 0xC3) { // 0xC3 = RET en x86/x64
                    bytes = i + 1;
                    break;
                }
            }
        }

        if (bytes == 0) {
            printf("[-] Error: No se pudo calcular el tamano de la funcion.\r\n");
            return;
        }

        // 4. Sobrescribir la función en memoria
        DWORD old_protect;
        if (VirtualProtect(start_addr, bytes, PAGE_EXECUTE_READWRITE, &old_protect)) {
            // Llenamos con ceros (0x00) o NOPs (0x90)
            std::memset(start_addr, 0x00, bytes);
            VirtualProtect(start_addr, bytes, old_protect, &old_protect);
        }
    }
} // namespace erase_my_dih

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define erase_fn(a)                                                            \
  constexpr auto CONCAT(w, __LINE__) = &a;                                     \
  erase_my_dih::erase_function(                                                \
      reinterpret_cast<std::uint8_t *>((void *&)CONCAT(w, __LINE__)))

// En x64 ya NO se requiere ensamblador en línea.
// Se deja la macro vacía para no romper el código donde ya pusiste 'erase_end;'
#define erase_end