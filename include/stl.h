#define ZYDIS_STATIC_DEFINE
#include "Zydis/Zydis.h"
#include "xbyak/xbyak.h"
#include <detours/detours.h>

namespace stl {
    enum class InstructionType { None, Call, Jump };

    inline size_t get_function_size(uintptr_t addr, size_t max_bytes = 1024) {
        ZydisDecoder decoder;
        ZydisDecodedInstruction instr;
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

        size_t offset = 0;
        const uint8_t* code = reinterpret_cast<const uint8_t*>(addr);

        while (offset < max_bytes) {
            if (ZydisDecoderDecodeInstruction(&decoder, nullptr, code + offset, max_bytes - offset, &instr) != ZYAN_STATUS_SUCCESS) break;

            offset += instr.length;

            if (instr.mnemonic == ZYDIS_MNEMONIC_RET || instr.mnemonic == ZYDIS_MNEMONIC_JMP) break;
        }

        return offset;
    }

    void log_assembly(uintptr_t runtime_address, size_t length, std::string title) {
        logger::info("{}", title);
        ZyanUSize offset = 0;
        ZydisDisassembledInstruction instruction;
        const uint8_t* code = reinterpret_cast<const uint8_t*>(runtime_address);
        while (offset < length && ZYAN_SUCCESS(ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LONG_64, runtime_address, code + offset, length - offset, &instruction))) {
            // logger::info("{:x}  {}", runtime_address, instruction.text);
            logger::info("{}", instruction.text);
            offset += instruction.info.length;
            runtime_address += instruction.info.length;
        }
        logger::info("");
    }

    std::string type_to_string(InstructionType type) {
        switch (type) {
            case stl::InstructionType::None:
                return "None";
            case stl::InstructionType::Call:
                return "Call";
            case stl::InstructionType::Jump:
                return "Jump";
            default:
                return "Unknown";
        }
    }

    struct InstructionInfo {
        size_t size;
        InstructionType type;
    };

    inline InstructionInfo get_instruction_info(uintptr_t addr) {
        ZydisDecoder decoder;
        ZydisDecodedInstruction instr;

        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

        InstructionInfo info{};
        info.size = 0;
        info.type = InstructionType::None;

        if (ZydisDecoderDecodeInstruction(&decoder, nullptr, reinterpret_cast<const void*>(addr), 16, &instr) == ZYAN_STATUS_SUCCESS) {
            info.size = instr.length;

            switch (instr.mnemonic) {
                case ZYDIS_MNEMONIC_CALL:
                    info.type = InstructionType::Call;
                    break;
                case ZYDIS_MNEMONIC_JMP:
                    info.type = InstructionType::Jump;
                    break;
                default:
                    break;
            }
        }

        return info;
    }

    template <class Func>
    uintptr_t ms_write_prologue_hook(uintptr_t a_src, Func* a_dest) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)a_src, a_dest);
        DetourTransactionCommit();
        return a_src;
    }

    template <class Func>
    auto write_prologue_hook(uintptr_t a_src, Func a_dest) {
        auto& trampoline = SKSE::GetTrampoline();

        const auto opcode = *reinterpret_cast<uint8_t*>(a_src);

        if (opcode == 0xE8) {
            logger::info("found call instruction");
            return trampoline.write_call<5>(a_src, a_dest);
        } else if (opcode == 0xE9) {
            logger::info("found jump instruction");
            return trampoline.write_branch<5>(a_src, a_dest);
        } else {
            size_t totalSize = 0;
            uintptr_t addr = a_src;

            do {
                auto info = stl::get_instruction_info(addr);
                if (info.size == 0) break;
                addr += info.size;
                totalSize += info.size;
            } while (totalSize < 5);

            logger::info("found other instructions, total size: {}", totalSize);

            struct Patch : Xbyak::CodeGenerator {
                explicit Patch(uintptr_t OriginalFuncAddr, size_t OriginalByteLength) {
                    // Hook returns here. Execute the restored bytes and jump back to the original function.
                    for (size_t i = 0; i < OriginalByteLength; i++) db(*reinterpret_cast<uint8_t*>(OriginalFuncAddr + i));
                    jmp(ptr[rip]);
                    dq(OriginalFuncAddr + OriginalByteLength);
                }
            };

            Patch p(a_src, totalSize);
            p.ready();

            trampoline.write_branch<5>(a_src, a_dest);

            auto alloc = trampoline.allocate(p.getSize());
            memcpy(alloc, p.getCode(), p.getSize());

            return reinterpret_cast<uintptr_t>(alloc);
        }
    }
}
