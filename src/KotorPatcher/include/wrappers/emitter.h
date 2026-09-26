#pragma once
#include <cstddef>
#include <cstdint>

namespace KotorPatcher {
    namespace Wrappers {

        // Emits into a fixed buffer and refuses to run past the end, so a bad size
        // estimate shows up as a failed hook rather than a corrupted heap.
        //
        // Both generators write through this. A wrapper's size is estimated before its
        // memory is allocated, term by term from what the generator is about to emit, and
        // an estimate that comes out short is a mistake in arithmetic nobody would see
        // otherwise: the writes land in whatever follows the allocation.
        class Emitter {
        public:
            Emitter(uint8_t* buffer, std::size_t capacity)
                : m_begin(buffer), m_cursor(buffer), m_end(buffer + capacity) {}

            void Byte(uint8_t value) {
                if (m_cursor >= m_end) { m_overflowed = true; return; }
                *m_cursor++ = value;
            }

            void Bytes(const uint8_t* bytes, std::size_t count) {
                for (std::size_t i = 0; i < count; ++i) Byte(bytes[i]);
            }

            // Little-endian, which is the only order either target encodes.
            void Dword(uint32_t value) {
                for (int i = 0; i < 4; ++i) Byte(static_cast<uint8_t>(value >> (i * 8)));
            }

            // True once a write has been refused. The caller checks this before handing the
            // buffer out, because every write after the first refusal is also dropped and
            // the code left behind is a truncated instruction stream.
            bool Overflowed() const { return m_overflowed; }

            // Where the next byte would go. Relative operands are computed from this, so it
            // is read mid-emission rather than only at the end.
            uint8_t* Cursor() const { return m_cursor; }

            std::size_t Written() const { return static_cast<std::size_t>(m_cursor - m_begin); }

        private:
            uint8_t* m_begin;
            uint8_t* m_cursor;
            uint8_t* m_end;
            bool m_overflowed = false;
        };

    } // namespace Wrappers
} // namespace KotorPatcher
