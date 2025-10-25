#include "fileIO.h"

void __stdcall ExecuteCommandOpenFile(DWORD routine, int paramCount) {
	if (paramCount != 2) {
		char buffer[128];
		sprintf_s(buffer, sizeof(buffer), "[PATCH] Wrong number of params found in ExecuteCommandOpenFile. Expected 2, got %i", paramCount);
		OutputDebugStringA(buffer);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}
	
	CExoString filename;
	stackPopString(*VIRTUAL_MACHINE_PTR, &filename);

	CExoString mode;
	stackPopString(*VIRTUAL_MACHINE_PTR, &mode);
	
	FILE* f = fopen(filename.c_string, mode.c_string);

	if (f == NULL) {
		char buffer[128];
		sprintf_s(buffer, sizeof(buffer), "[PATCH] Failed to Open File '%s', with mode '%s'", filename.c_string, mode.c_string);
		OutputDebugStringA(buffer);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	stackPushInteger(*VIRTUAL_MACHINE_PTR, (int)f);
}

void __stdcall ExecuteCommandCloseFile(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		char buffer[128];
		sprintf_s(buffer, sizeof(buffer), "[PATCH] Wrong number of params found in ExecuteCommandCloseFile. Expected 1, got %i", paramCount);
		OutputDebugStringA(buffer);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	int file;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	if (fclose(f)) {
		char buffer[128];
		sprintf_s(buffer, sizeof(buffer), "[PATCH] Failed to Close File Stream at '%p'", f);
		OutputDebugStringA(buffer);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	stackPushInteger(*VIRTUAL_MACHINE_PTR, 1);
}

void __stdcall ExecuteCommandReadTextFile(DWORD routine, int paramCount) {
	if (paramCount != 2) {
		char buffer[128];
		sprintf_s(buffer, sizeof(buffer), "[PATCH] Wrong number of params found in ExecuteCommandReadTextFile. Expected 2, got %i", paramCount);
		OutputDebugStringA(buffer);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	int file;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	int charCount;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &charCount);

	char buffer[4096];
	int itemsRead = fread_s(buffer, sizeof(buffer), 1, charCount, f);

	CExoString* output = new CExoString();
	constructCExoStringFromCStr(output, buffer, itemsRead);

	stackPushString(*VIRTUAL_MACHINE_PTR, output);

	destructCExoString(output);
	delete output;
}

void __stdcall ExecuteCommandWriteTextFile(DWORD routine, int paramCount) {
	if (paramCount != 2) {
		char buffer[128];
		sprintf_s(buffer, sizeof(buffer), "[PATCH] Wrong number of params found in ExecuteCommandWriteTextFile. Expected 2, got %i", paramCount);
		OutputDebugStringA(buffer);
		stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	int file;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &file);
	FILE* f = (FILE*)file;

	CExostring text;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, &text);


}