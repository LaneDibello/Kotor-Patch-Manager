#!/usr/bin/env bash
# The build definition now lives in the Makefile. This shim preserves the
# interface the release orchestrators (publish.sh, ../../build-mingw.sh) rely on:
# "build-mingw.sh <absolute-output-path>" links KotorPatcher.dll straight there.
set -euo pipefail

cd "$(dirname "$0")"
exec make --no-print-directory dll DLL_OUT="${1:-KotorPatcher.dll}"
