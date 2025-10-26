# Future Enhancements

This document tracks potential improvements I've been brainstorming for the KOTOR Patcher project.

---

## Other Enhancements

### Signature Scanning
- Find hook addresses by byte patterns instead of fixed addresses
- Makes patches more version-portable
- Useful when exact addresses differ between game versions

### Logging System
- Optional file logging (in addition to OutputDebugString)
- Configurable log levels (ERROR, WARN, INFO, DEBUG)
- Performance metrics (patch load time, etc.)

---

## Cross-Platform Support

### Steam Support
- For kotor 1, this is very easy, as most of the addresses are identical
- For kotor 2, it's a bit more complicated
- It ultimately boils down to different game manifest approaches

### macOS Support
- Adapt trampolines for x86_64/ARM64 macOS
- Use mach-o DLL injection
- Support KOTOR Mac versions

### Linux Support (via Wine/Proton)
- Test compatibility with Wine
- In my experience, Kotor runs really well with Wine, so this may be free

---

## Hook Features

- Support for return value modification
- Support for struct/array parameters
- Support for 64-bit parameters (probably not)
- Automatic parameter detection based on calling convention analysis
