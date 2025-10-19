# Future Enhancements & Roadmap

This document tracks planned features and improvements for the KOTOR Patcher project.

---

## Other Enhancements

### Signature Scanning
- Find hook addresses by byte patterns instead of fixed addresses
- Makes patches more version-portable
- Useful when exact addresses differ between game versions

### Advanced Memory Protection
- Restore original code on DLL unload
- Implement patch enable/disable at runtime
- Create snapshots for rollback

### Logging System
- Optional file logging (in addition to OutputDebugString)
- Configurable log levels (ERROR, WARN, INFO, DEBUG)
- Performance metrics (patch load time, etc.)

### Multiple Hook Types
- ✅ INLINE hooks with parameter extraction (fully implemented)
- ✅ REPLACE hooks (fully implemented)
- ✅ WRAP hooks with calling convention support (fully implemented)
- ⬜ Import table hooks (future)
- ⬜ Virtual function table hooks (future)

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
- Potential native Linux KOTOR support

---

## Questions for Future Implementation
- Support for multiple hook types per patch? (Future consideration)
	- Yes, this is the hope
