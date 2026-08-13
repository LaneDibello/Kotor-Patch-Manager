// Inherit the parent model's special-texture flag across Odyssey reference nodes.

#include <cstddef>
#include <cstdint>

namespace {

constexpr std::ptrdiff_t DependencyControllerOffset = 0x188;
constexpr std::ptrdiff_t ControllerParentModelOffset = 0x14;
constexpr std::ptrdiff_t ControllerTargetNodeOffset = 0x18;
constexpr std::ptrdiff_t NodeSourceOffset = 0x04;
constexpr std::uint8_t ReferenceNodeFlags = 0x11u;

constexpr std::ptrdiff_t SpecialTextureEnabledOffset = 0x17C;

void* ReadPointer(void* object, std::ptrdiff_t offset)
{
    return object
        ? *reinterpret_cast<void**>(reinterpret_cast<std::uint8_t*>(object) + offset)
        : nullptr;
}

bool IsReferenceNode(void* runtimeNode)
{
    void* sourceNode = ReadPointer(runtimeNode, NodeSourceOffset);
    if (!sourceNode) {
        return false;
    }

    std::uint8_t flags = *reinterpret_cast<std::uint8_t*>(sourceNode);
    return (flags & ReferenceNodeFlags) == ReferenceNodeFlags;
}

} // namespace

extern "C" void __cdecl InheritReferencedModelStealthTexture(void* model)
{
    void* controller = ReadPointer(model, DependencyControllerOffset);
    if (!controller) {
        return;
    }

    void* parentModel = ReadPointer(controller, ControllerParentModelOffset);
    void* targetNode = ReadPointer(controller, ControllerTargetNodeOffset);
    if (!parentModel || !IsReferenceNode(targetNode)) {
        return;
    }

    auto* childBytes = reinterpret_cast<std::uint8_t*>(model);
    auto* parentBytes = reinterpret_cast<std::uint8_t*>(parentModel);
    childBytes[SpecialTextureEnabledOffset] = parentBytes[SpecialTextureEnabledOffset];
}
