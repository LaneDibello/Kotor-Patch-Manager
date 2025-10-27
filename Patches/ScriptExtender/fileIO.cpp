#include "fileIO.h"

int __stdcall ExecuteCommandOpenFile(DWORD routine, int paramCount) {
	if (paramCount != 2) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandOpenFile. Expected 2, got %i", paramCount);
		virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return 0;
	}

	CExoString filename;
	virtualMachineStackPopString(*VIRTUAL_MACHINE_PTR, &filename);

	CExoString mode;
	virtualMachineStackPopString(*VIRTUAL_MACHINE_PTR, &mode);

	debugLog("[PATCH] Opening file '%s' with mode '%s'", filename.c_string, mode.c_string);

	FILE* f;
	errno_t err = fopen_s(&f, filename.c_string, mode.c_string);

	if (err) {
		debugLog("[PATCH] Failed to Open File '%s', with mode '%s, and error %i'", filename.c_string, mode.c_string, err);
		virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return 0;
	}

	virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, (int)f);

	return 0;
}

int __stdcall ExecuteCommandCloseFile(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandCloseFile. Expected 1, got %i", paramCount);
		virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return 0;
	}

	int file;
	virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	if (fclose(f)) {
		debugLog("[PATCH] Failed to Close File Stream at '%p'", f);
		virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return 0;
	}

	debugLog("[PATCH] Closing file with handle '%p'", f);

	virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, 1);

	return 0;
}

int __stdcall ExecuteCommandReadTextFile(DWORD routine, int paramCount) {
	if (paramCount != 2) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandReadTextFile. Expected 2, got %i", paramCount);
		virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return 0;
	}

	int file;
	virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	int charCount;
	virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &charCount);

	char buffer[4096];
	int itemsRead = (int)fread_s((void *)buffer, sizeof(buffer), 1, charCount, f);

	CExoString output;
	output.c_string = buffer;
	output.length = itemsRead;

	virtualMachineStackPushString(*VIRTUAL_MACHINE_PTR, &output);

	return 0;
}

int __stdcall ExecuteCommandWriteTextFile(DWORD routine, int paramCount) {
	if (paramCount != 2) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandWriteTextFile. Expected 2, got %i", paramCount);
		virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return 0;
	}

	int file;
	virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	CExoString text;
	virtualMachineStackPopString(*VIRTUAL_MACHINE_PTR, &text);

	int charsWritten = (int)fwrite((const void*)text.c_string, 1, text.length, f);
	virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, charsWritten);

	return 0;
}

int __stdcall ExecuteCommandPeakCharFile(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandPeakCharFile. Expected 1, got %i", paramCount);
		CExoString empty("", 0);
		virtualMachineStackPushString(*VIRTUAL_MACHINE_PTR, &empty);
		return 0;
	}

	int file;
	virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	int c = fgetc(f);
	if (c == EOF) {
		debugLog("[PATCH] PeakCharFile: EOF or error reading from file handle '%p'", f);
		CExoString empty("", 0);
		virtualMachineStackPushString(*VIRTUAL_MACHINE_PTR, &empty);
		return 0;
	}

	ungetc(c, f);

	char buffer[2] = { (char)c, '\0' };
	CExoString result(buffer, 1);
	virtualMachineStackPushString(*VIRTUAL_MACHINE_PTR, &result);

	return 0;
}

int __stdcall ExecuteCommandSeekFile(DWORD routine, int paramCount) {
	if (paramCount != 3) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandSeekFile. Expected 3, got %i", paramCount);
		virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return 0;
	}

	int file;
	virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	int offset;
	virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &offset);

	int origin;
	virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &origin);
	// Origin values: SEEK_SET = 0, SEEK_CUR = 1, SEEK_END = 2

	int result = fseek(f, offset, origin);

	if (result != 0) {
		debugLog("[PATCH] SeekFile: fseek failed on file handle '%p', offset %d, origin %d", f, offset, origin);
		virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return 0;
	}

	virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, 1);

	return 0;
}

int __stdcall ExecuteCommandTellFile(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandTellFile. Expected 1, got %i", paramCount);
		virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, -1);
		return 0;
	}

	int file;
	virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	long position = ftell(f);

	if (position == -1L) {
		debugLog("[PATCH] TellFile: ftell failed on file handle '%p'", f);
	}

	virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, (int)position);

	return 0;
}