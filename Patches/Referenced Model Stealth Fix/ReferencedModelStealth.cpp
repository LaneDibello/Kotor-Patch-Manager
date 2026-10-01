#include <cstddef>
#include <cstdint>

namespace {

constexpr std::ptrdiff_t Kotor1GobDistortionOffset = 0x17C;
constexpr std::ptrdiff_t Kotor2GobDistortionOffset = 0x1A8;
constexpr std::size_t MaxAttachments = 1024;
constexpr unsigned MaxAttachmentDepth = 32;
constexpr std::uintptr_t Kotor1CreateReferenceObjectsStart = 0x004448C0;
constexpr std::uintptr_t Kotor1CreateReferenceObjectsEnd = 0x00444D90;
constexpr std::uintptr_t Kotor2CreateReferenceObjectsStart = 0x00854480;
constexpr std::uintptr_t Kotor2CreateReferenceObjectsEnd = 0x008545A5;

struct Attachment {
    void* parent;
    void* child;
};

Attachment attachments[MaxAttachments] = {};
std::ptrdiff_t gobDistortionOffset = Kotor1GobDistortionOffset;

void RegisterAttachment(void* parent, void* child)
{
    Attachment* empty = nullptr;
    for (Attachment& attachment : attachments) {
        if (attachment.parent == parent && attachment.child == child) {
            return;
        }
        if (!empty && !attachment.parent) {
            empty = &attachment;
        }
    }

    if (empty) {
        empty->parent = parent;
        empty->child = child;
    }
}

void RemoveGobAttachments(void* gob)
{
    for (Attachment& attachment : attachments) {
        if (attachment.parent == gob || attachment.child == gob) {
            attachment = {};
        }
    }
}

void PropagateDistortion(void* parent, std::uint8_t enabled, unsigned depth)
{
    if (!parent || depth >= MaxAttachmentDepth) {
        return;
    }

    for (const Attachment& attachment : attachments) {
        if (attachment.parent == parent && attachment.child) {
            auto* child = reinterpret_cast<std::uint8_t*>(attachment.child);
            child[gobDistortionOffset] = enabled;
            PropagateDistortion(attachment.child, enabled, depth + 1);
        }
    }
}

void InitializeReferencedModelStealthForVersion(
    void* childGob,
    void** parentGobArgument,
    void** returnAddressSlot,
    std::uintptr_t loaderStart,
    std::uintptr_t loaderEnd,
    std::ptrdiff_t distortionOffset)
{
    void* parentGob = parentGobArgument ? *parentGobArgument : nullptr;
    std::uintptr_t caller = returnAddressSlot
        ? reinterpret_cast<std::uintptr_t>(*returnAddressSlot)
        : 0;
    if (!childGob || !parentGob || caller < loaderStart || caller >= loaderEnd) {
        return;
    }

    gobDistortionOffset = distortionOffset;
    RegisterAttachment(parentGob, childGob);

    auto* child = reinterpret_cast<std::uint8_t*>(childGob);
    auto* parent = reinterpret_cast<std::uint8_t*>(parentGob);
    std::uint8_t enabled = parent[gobDistortionOffset];
    child[gobDistortionOffset] = enabled;
    PropagateDistortion(childGob, enabled, 0);
}

} // namespace

extern "C" void __cdecl InitializeReferencedModelStealth(
    void* childGob,
    void** parentGobArgument,
    void** returnAddressSlot)
{
    InitializeReferencedModelStealthForVersion(
        childGob,
        parentGobArgument,
        returnAddressSlot,
        Kotor1CreateReferenceObjectsStart,
        Kotor1CreateReferenceObjectsEnd,
        Kotor1GobDistortionOffset);
}

extern "C" void __cdecl EnableReferencedModelStealth(void* gob)
{
    gobDistortionOffset = Kotor1GobDistortionOffset;
    PropagateDistortion(gob, 1u, 0);
}

extern "C" void __cdecl DisableReferencedModelStealth(void* gob)
{
    gobDistortionOffset = Kotor1GobDistortionOffset;
    PropagateDistortion(gob, 0u, 0);
}

extern "C" void __cdecl InitializeReferencedModelStealthK2(
    void* childGob,
    void** parentGobArgument,
    void** returnAddressSlot)
{
    InitializeReferencedModelStealthForVersion(
        childGob,
        parentGobArgument,
        returnAddressSlot,
        Kotor2CreateReferenceObjectsStart,
        Kotor2CreateReferenceObjectsEnd,
        Kotor2GobDistortionOffset);
}

extern "C" void __cdecl EnableReferencedModelStealthK2(void* gob)
{
    gobDistortionOffset = Kotor2GobDistortionOffset;
    PropagateDistortion(gob, 1u, 0);
}

extern "C" void __cdecl DisableReferencedModelStealthK2(void* gob)
{
    gobDistortionOffset = Kotor2GobDistortionOffset;
    PropagateDistortion(gob, 0u, 0);
}

extern "C" void __cdecl CleanupReferencedModelStealth(void* gob)
{
    RemoveGobAttachments(gob);
}
