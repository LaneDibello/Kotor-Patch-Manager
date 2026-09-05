# Code generator tests

Run them with `make test` from `src/KotorPatcher`.

The engine writes machine code while the game is running: the trampoline works
out jump displacements, the platform backend hands out memory to put stubs in,
and the two DETOUR wrapper generators emit the code that calls a patch
function. These tests cover that code.

They are not unit tests in the usual sense. Each one builds a fake hook site in
a page of stub memory, points it at a wrapper the generator has just produced,
jumps into it with a known value in every register, and checks what comes out
the other end. Running the bytes is the only way to tell whether they are
right. Reading them back only proves they are the bytes we meant to write, not
that those bytes do what we wanted.

## Why they run on the host

What a generator emits depends on the instruction set, not on the operating
system. An x86_64 wrapper built for macOS is byte for byte the one built here,
so running it on Linux tests that build too. That is worth having, because
testing the macOS engine any other way means a virtual machine with a copy of
the game in it.

You need an x86 host that can build 32-bit binaries (`glibc-devel.i686` on
Fedora, `gcc-multilib` on Debian and Ubuntu). There is nothing here to run on
other architectures.

## Layout

- `any_*` is built at both widths, once against each wrapper generator.
- `t32_*` and `t64_*` are built against the generator for their instruction
  set.

A test is a program that prints its checks and exits non-zero if any of them
failed. `check.h` is the whole framework.

## What is covered

- Jump displacements, including the wrap-around cases at 32 bits that a naive
  range check gets wrong, and the exact boundary at 64 bits.
- Stub memory: writable, then sealed, then callable, and placed close enough to
  a given address that a relative jump reaches it.
- Wrappers: passing arguments, reading a parameter out of a register or off the
  stack, stack alignment at the call, the saved state surviving a patch
  function that destroys everything it is allowed to, the stolen bytes running,
  flags surviving to the resume point, the red zone, excluded registers, and
  both consumed-exit paths.
- Floating point: x87, MMX, SSE and MXCSR surviving a patch function that calls
  `fninit` and zeroes the XMM registers.
