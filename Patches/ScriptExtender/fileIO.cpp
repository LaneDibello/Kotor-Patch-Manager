#include "fileIO.h"

void __stdcall ExecuteCommandOpenFile(DWORD routine, int paramCount) {
	if (paramCount != 2) {
		DebugLog("[PATCH] Wrong number of params found in ExecuteCommandOpenFile. Expected 2, got %i", paramCount);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	CExoString filename;
	stackPopString(*VIRTUAL_MACHINE_PTR, &filename);

	CExoString mode;
	stackPopString(*VIRTUAL_MACHINE_PTR, &mode);

	DebugLog("[PATCH] Opening file '%s' with mode '%s'", filename.c_string, mode.c_string);

	FILE* f = fopen(filename.c_string, mode.c_string);

	if (f == NULL) {
		DebugLog("[PATCH] Failed to Open File '%s', with mode '%s'", filename.c_string, mode.c_string);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	stackPushInteger(*VIRTUAL_MACHINE_PTR, (int)f);
}

void __stdcall ExecuteCommandCloseFile(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		DebugLog("[PATCH] Wrong number of params found in ExecuteCommandCloseFile. Expected 1, got %i", paramCount);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	int file;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	if (fclose(f)) {
		DebugLog("[PATCH] Failed to Close File Stream at '%p'", f);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	DebugLog("[PATCH] Closing file with handle '%p'", f);

	stackPushInteger(*VIRTUAL_MACHINE_PTR, 1);
}

void __stdcall ExecuteCommandReadTextFile(DWORD routine, int paramCount) {
	if (paramCount != 2) {
		DebugLog("[PATCH] Wrong number of params found in ExecuteCommandReadTextFile. Expected 2, got %i", paramCount);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	int file;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	int charCount;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &charCount);

	char buffer[4096];
	int itemsRead = (int)fread_s((void *)buffer, sizeof(buffer), 1, charCount, f);

	CExoString output;
	output.c_string = buffer;
	output.length = itemsRead;

	stackPushString(*VIRTUAL_MACHINE_PTR, &output);
}

void __stdcall ExecuteCommandWriteTextFile(DWORD routine, int paramCount) {
	if (paramCount != 2) {
		DebugLog("[PATCH] Wrong number of params found in ExecuteCommandWriteTextFile. Expected 2, got %i", paramCount);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	int file;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	CExoString text;
	stackPopString(*VIRTUAL_MACHINE_PTR, &text);

	int charsWritten = (int)fwrite((const void*)text.c_string, 1, text.length, f);
	stackPushInteger(*VIRTUAL_MACHINE_PTR, charsWritten);
}