// Track referenced-model attachments as the game creates and destroys them,
// then propagate the parent's distortion state through those known links.

#include <cstddef>
#include <cstdint>

namespace {

constexpr std::size_t MaxAttachments = 1024;
constexpr unsigned MaxAttachmentDepth = 32;

struct Attachment {
    void* parent;
    void* child;
};

Attachment attachments[MaxAttachments] = {};

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

void PropagateDistortion(
    void* parent,
    std::uint8_t enabled,
    unsigned depth,
    std::uintptr_t distortionOffset)
{
    if (!parent || depth >= MaxAttachmentDepth) {
        return;
    }

    for (const Attachment& attachment : attachments) {
        if (attachment.parent == parent && attachment.child) {
            auto* child = reinterpret_cast<std::uint8_t*>(attachment.child);
            child[distortionOffset] = enabled;
            PropagateDistortion(attachment.child, enabled, depth + 1, distortionOffset);
        }
    }
}

} // namespace

extern "C" void __cdecl InitializeReferencedModelStealth(
    void* childGob,
    void* parentGob,
    void* returnAddress,
    std::uintptr_t distortionOffset,
    std::uintptr_t loaderStart,
    std::uintptr_t loaderEnd)
{
    std::uintptr_t caller = reinterpret_cast<std::uintptr_t>(returnAddress);
    if (!childGob || !parentGob || caller < loaderStart || caller >= loaderEnd) {
        return;
    }

    RegisterAttachment(parentGob, childGob);

    auto* child = reinterpret_cast<std::uint8_t*>(childGob);
    auto* parent = reinterpret_cast<std::uint8_t*>(parentGob);
    std::uint8_t enabled = parent[distortionOffset];
    child[distortionOffset] = enabled;
    PropagateDistortion(childGob, enabled, 0, distortionOffset);
}

extern "C" void __cdecl EnableReferencedModelStealth(
    void* gob,
    std::uintptr_t distortionOffset)
{
    PropagateDistortion(gob, 1u, 0, distortionOffset);
}

extern "C" void __cdecl DisableReferencedModelStealth(
    void* gob,
    std::uintptr_t distortionOffset)
{
    PropagateDistortion(gob, 0u, 0, distortionOffset);
}

extern "C" void __cdecl CleanupReferencedModelStealth(void* gob)
{
    RemoveGobAttachments(gob);
}
