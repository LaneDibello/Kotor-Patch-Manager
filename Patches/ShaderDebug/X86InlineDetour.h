#pragma once

#include <cstddef>

namespace x86hook {
    class InlineDetour {
    public:
        bool Install(void* entryPoint, void* replacement);

        bool IsInstalled() const { return m_target != nullptr; }
        bool Covers(void* entryPoint) const;
        void* Original() const { return m_trampoline; }

    private:
        static constexpr std::size_t kMaxStolenBytes = 32;

        void* m_target = nullptr;
        void* m_replacement = nullptr;
        void* m_trampoline = nullptr;
    };
}


