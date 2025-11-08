#include "fileIO.h"

int __stdcall ExecuteCommandOpenFile(DWORD routine, int paramCount) {
	if (paramCount != 2) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandOpenFile. Expected 2, got %i", paramCount);
		CVirtualMachine* vm = CVirtualMachine::GetInstance();
		if (vm) {
			vm->StackPushInteger(0);
			delete vm;
		}
		return 0;
	}

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return -2001;

	CExoString filename;
	if (!vm->StackPopString(&filename)) {
		delete vm;
		return -2001;
	}

	CExoString mode;
	if (!vm->StackPopString(&mode)) {
		delete vm;
		return -2001;
	}

	debugLog("[PATCH] Opening file '%s' with mode '%s'", filename.GetCStr(), mode.GetCStr());

	FILE* f;
	errno_t err = fopen_s(&f, filename.GetCStr(), mode.GetCStr());

	if (err) {
		debugLog("[PATCH] Failed to Open File '%s', with mode '%s, and error %i'", filename.GetCStr(), mode.GetCStr(), err);
		vm->StackPushInteger(0);
		delete vm;
		return 0;
	}

	vm->StackPushInteger((int)f);
	delete vm;
	return 0;
}

int __stdcall ExecuteCommandCloseFile(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandCloseFile. Expected 1, got %i", paramCount);
		CVirtualMachine* vm = CVirtualMachine::GetInstance();
		if (vm) {
			vm->StackPushInteger(0);
			delete vm;
		}
		return 0;
	}

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return -2001;

	int file;
	if (!vm->StackPopInteger(&file)) {
		delete vm;
		return -2001;
	}

	FILE* f = (FILE*)file;

	if (fclose(f)) {
		debugLog("[PATCH] Failed to Close File Stream at '%p'", f);
		vm->StackPushInteger(0);
		delete vm;
		return 0;
	}

	debugLog("[PATCH] Closing file with handle '%p'", f);

	vm->StackPushInteger(1);
	delete vm;
	return 0;
}

int __stdcall ExecuteCommandReadTextFile(DWORD routine, int paramCount) {
	if (paramCount != 2) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandReadTextFile. Expected 2, got %i", paramCount);
		CVirtualMachine* vm = CVirtualMachine::GetInstance();
		if (vm) {
			vm->StackPushInteger(0);
			delete vm;
		}
		return 0;
	}

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return -2001;

	int file;
	if (!vm->StackPopInteger(&file)) {
		delete vm;
		return -2001;
	}

	FILE* f = (FILE*)file;

	int charCount;
	if (!vm->StackPopInteger(&charCount)) {
		delete vm;
		return -2001;
	}

	char buffer[4096];
	int itemsRead = (int)fread_s((void *)buffer, sizeof(buffer), 1, charCount, f);

	CExoString output(buffer, itemsRead);

	if (!vm->StackPushString(&output)) {
		delete vm;
		return -2000;
	}

	delete vm;
	return 0;
}

int __stdcall ExecuteCommandWriteTextFile(DWORD routine, int paramCount) {
	if (paramCount != 2) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandWriteTextFile. Expected 2, got %i", paramCount);
		CVirtualMachine* vm = CVirtualMachine::GetInstance();
		if (vm) {
			vm->StackPushInteger(0);
			delete vm;
		}
		return 0;
	}

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return -2001;

	int file;
	if (!vm->StackPopInteger(&file)) {
		delete vm;
		return -2001;
	}

	FILE* f = (FILE*)file;

	CExoString text;
	if (!vm->StackPopString(&text)) {
		delete vm;
		return -2001;
	}

	int charsWritten = (int)fwrite((const void*)text.GetCStr(), 1, text.GetLength(), f);

	if (!vm->StackPushInteger(charsWritten)) {
		delete vm;
		return -2000;
	}

	delete vm;
	return 0;
}

int __stdcall ExecuteCommandPeakCharFile(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandPeakCharFile. Expected 1, got %i", paramCount);
		CVirtualMachine* vm = CVirtualMachine::GetInstance();
		if (vm) {
			CExoString empty("", 0);
			vm->StackPushString(&empty);
			delete vm;
		}
		return 0;
	}

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return -2001;

	int file;
	if (!vm->StackPopInteger(&file)) {
		delete vm;
		return -2001;
	}

	FILE* f = (FILE*)file;

	int c = fgetc(f);
	if (c == EOF) {
		debugLog("[PATCH] PeakCharFile: EOF or error reading from file handle '%p'", f);
		CExoString empty("", 0);
		vm->StackPushString(&empty);
		delete vm;
		return 0;
	}

	ungetc(c, f);

	char buffer[2] = { (char)c, '\0' };
	CExoString result(buffer, 1);

	if (!vm->StackPushString(&result)) {
		delete vm;
		return -2000;
	}

	delete vm;
	return 0;
}

int __stdcall ExecuteCommandSeekFile(DWORD routine, int paramCount) {
	if (paramCount != 3) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandSeekFile. Expected 3, got %i", paramCount);
		CVirtualMachine* vm = CVirtualMachine::GetInstance();
		if (vm) {
			vm->StackPushInteger(0);
			delete vm;
		}
		return 0;
	}

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return -2001;

	int file;
	if (!vm->StackPopInteger(&file)) {
		delete vm;
		return -2001;
	}

	FILE* f = (FILE*)file;

	int offset;
	if (!vm->StackPopInteger(&offset)) {
		delete vm;
		return -2001;
	}

	int origin;
	if (!vm->StackPopInteger(&origin)) {
		delete vm;
		return -2001;
	}
	// Origin values: SEEK_SET = 0, SEEK_CUR = 1, SEEK_END = 2

	int result = fseek(f, offset, origin);

	if (result != 0) {
		debugLog("[PATCH] SeekFile: fseek failed on file handle '%p', offset %d, origin %d", f, offset, origin);
		vm->StackPushInteger(0);
		delete vm;
		return 0;
	}

	vm->StackPushInteger(1);
	delete vm;
	return 0;
}

int __stdcall ExecuteCommandTellFile(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		debugLog("[PATCH] Wrong number of params found in ExecuteCommandTellFile. Expected 1, got %i", paramCount);
		CVirtualMachine* vm = CVirtualMachine::GetInstance();
		if (vm) {
			vm->StackPushInteger(-1);
			delete vm;
		}
		return 0;
	}

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return -2001;

	int file;
	if (!vm->StackPopInteger(&file)) {
		delete vm;
		return -2001;
	}

	FILE* f = (FILE*)file;

	long position = ftell(f);

	if (position == -1L) {
		debugLog("[PATCH] TellFile: ftell failed on file handle '%p'", f);
	}

	if (!vm->StackPushInteger((int)position)) {
		delete vm;
		return -2000;
	}

	delete vm;
	return 0;
}