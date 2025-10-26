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

void __stdcall ExecuteCommandPeakCharFile(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		DebugLog("[PATCH] Wrong number of params found in ExecuteCommandPeakCharFile. Expected 1, got %i", paramCount);
		CExoString empty("", 0);
		stackPushString(*VIRTUAL_MACHINE_PTR, &empty);
		return;
	}

	int file;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	int c = fgetc(f);
	if (c == EOF) {
		DebugLog("[PATCH] PeakCharFile: EOF or error reading from file handle '%p'", f);
		CExoString empty("", 0);
		stackPushString(*VIRTUAL_MACHINE_PTR, &empty);
		return;
	}

	ungetc(c, f);

	char buffer[2] = { (char)c, '\0' };
	CExoString result(buffer, 1);
	stackPushString(*VIRTUAL_MACHINE_PTR, &result);
}

void __stdcall ExecuteCommandSeekFile(DWORD routine, int paramCount) {
	if (paramCount != 3) {
		DebugLog("[PATCH] Wrong number of params found in ExecuteCommandSeekFile. Expected 3, got %i", paramCount);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	int file;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	int offset;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &offset);

	int origin;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &origin);
	// Origin values: SEEK_SET = 0, SEEK_CUR = 1, SEEK_END = 2

	int result = fseek(f, offset, origin);

	if (result != 0) {
		DebugLog("[PATCH] SeekFile: fseek failed on file handle '%p', offset %d, origin %d", f, offset, origin);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	stackPushInteger(*VIRTUAL_MACHINE_PTR, 1);
}

void __stdcall ExecuteCommandTellFile(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		DebugLog("[PATCH] Wrong number of params found in ExecuteCommandTellFile. Expected 1, got %i", paramCount);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, -1);
		return;
	}

	int file;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	long position = ftell(f);

	if (position == -1L) {
		DebugLog("[PATCH] TellFile: ftell failed on file handle '%p'", f);
	}

	stackPushInteger(*VIRTUAL_MACHINE_PTR, (int)position);
}