#include "Hooks.h"
#include "stl.h"
#define HOOK1 stl::ms_write_prologue_hook
#define HOOK2 stl::ms_write_prologue_hook
#define HOOK3 stl::ms_write_prologue_hook
struct Hook1 {
    static void thunk(uint32_t a1, uint64_t a2, uint64_t a3) { 
        logger::info("begunning of thunk 1");
        originalFunction(a1, a2, a3);
        logger::info("end of thunk 1");
    }
    static inline REL::Relocation<decltype(thunk)> originalFunction;
    static void install() { 
        originalFunction = HOOK1(REL::ID(107329).address(), thunk);
    }
};
struct Hook2 {
    static void thunk(uint32_t a1, uint64_t a2, uint64_t a3) { 
        logger::info("begunning of thunk 2");
        originalFunction(a1, a2, a3);
        logger::info("end of thunk 2");
    }
    static inline REL::Relocation<decltype(thunk)> originalFunction;
    static void install() { 
        originalFunction = HOOK2(REL::ID(107329).address(), thunk);
    }
};
struct Hook3 {
    static void thunk(uint32_t a1, uint64_t a2, uint64_t a3) { 
        logger::info("begunning of thunk 3");
        originalFunction(a1, a2, a3);
        logger::info("end of thunk 3");
    }
    static inline REL::Relocation<decltype(thunk)> originalFunction;
    static void install() { 
        originalFunction = HOOK3(REL::ID(107329).address(), thunk);
    }
};


void Hooks::Install() {
    SKSE::AllocTrampoline(512);
    stl::log_assembly(REL::ID(107329).address(), 13, "original");
    Hook1::install();
    stl::log_assembly(REL::ID(107329).address(), 13, "after hook");
    Hook2::install();
    stl::log_assembly(REL::ID(107329).address(), 13, "after hook 2");
    Hook3::install();
    stl::log_assembly(REL::ID(107329).address(), 13, "after hook 3");
}