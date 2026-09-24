// Startup crash once a controller is visible to SDL
//
// Two subsystems own the same controller: the DirectInput joystick device, which
// opens one in Initialize and closes it in its destructor, and ASL's rumble
// manager, which opens every joystick index into a four slot table and closes
// those on its own schedule. The joystick ends up closed twice.
//
// SDL_JoystickClose treats any reference count below 1 as the last close, so a
// count arriving at -1 runs the teardown a second time and re-frees the joystick
// and its arrays. It faults before those frees: SDL_SYS_JoystickClose re-reads
// joystick->hwdata after its close(fd) call without re-testing it for null, and
// freed memory reused by another thread during that syscall changes underneath
// it.
//
// Nulling the argument slot is enough, because the function's first act is to
// test that argument against null and return, which skips the teardown and the
// double free together.
//
// The guard belongs here rather than on the DirectInput destructor, which closes
// a live joystick with a count of 1; the fatal close arrives by another route,
// and guarding the destructor would still leave the double free to abort.

#include <cstddef>

#if !defined(_WIN32)
// Already how a free function is called on i386 System V. The keyword is MSVC's.
#define __cdecl
#endif

namespace {

// SDL_Joystick's reference count, from SDL_JoystickClose's own decrement and seen
// at -1 in two core dumps of this crash.
constexpr std::size_t kJoystickRefCount = 0x2C;

} // namespace

// Entry of SDL_JoystickClose. A genuine last close arrives with 1.
extern "C" void __cdecl SkipOverReleasedJoystickClose(void** joystickSlot)
{
    const unsigned char* joystick =
        static_cast<const unsigned char*>(*joystickSlot);
    if (joystick == nullptr) {
        return;
    }

    if (*reinterpret_cast<const int*>(joystick + kJoystickRefCount) <= 0) {
        *joystickSlot = nullptr;
    }
}
