#include "CResGFF.h"
#include "GameVersion.h"

// ===== STATIC MEMBER INITIALIZATION =====

// Static initialization flags
bool CResGFF::functionsInitialized = false;
bool CResGFF::offsetsInitialized = false;

// Constructors/Destructors
CResGFF::ConstructorFn CResGFF::constructor = nullptr;
CResGFF::Constructor2Fn CResGFF::constructor2 = nullptr;
CResGFF::DestructorFn CResGFF::destructor = nullptr;
CResGFF::Destructor2Fn CResGFF::destructor2 = nullptr;
CResGFF::Destructor3Fn CResGFF::destructor3 = nullptr;

// Data Management
CResGFF::AddDataFieldFn CResGFF::addDataField = nullptr;
CResGFF::AddDataLayoutFieldFn CResGFF::addDataLayoutField = nullptr;
CResGFF::AddDataLayoutListFn CResGFF::addDataLayoutList = nullptr;
CResGFF::GetDataFieldFn CResGFF::getDataField = nullptr;
CResGFF::GetDataLayoutListFn CResGFF::getDataLayoutList = nullptr;

// Structure/Field/Label Management
CResGFF::AddStructFn CResGFF::addStruct = nullptr;
CResGFF::AddFieldFn CResGFF::addField = nullptr;
CResGFF::AddLabelFn CResGFF::addLabel = nullptr;
CResGFF::AddListFn CResGFF::addList = nullptr;
CResGFF::AddListElementFn CResGFF::addListElement = nullptr;
CResGFF::AddStructToStructFn CResGFF::addStructToStruct = nullptr;
CResGFF::GetFieldFn CResGFF::getField = nullptr;
CResGFF::GetField2Fn CResGFF::getField2 = nullptr;
CResGFF::GetFieldByLabelFn CResGFF::getFieldByLabel = nullptr;
CResGFF::GetFieldCountFn CResGFF::getFieldCount = nullptr;
CResGFF::GetFieldTypeFn CResGFF::getFieldType = nullptr;
CResGFF::GetElementTypeFn CResGFF::getElementType = nullptr;
CResGFF::GetListFn CResGFF::getList = nullptr;
CResGFF::GetListCountFn CResGFF::getListCount = nullptr;
CResGFF::GetListElementFn CResGFF::getListElement = nullptr;
CResGFF::GetStructFromStructFn CResGFF::getStructFromStruct = nullptr;
CResGFF::GetTopLevelStructFn CResGFF::getTopLevelStruct = nullptr;

// Read Field Functions
CResGFF::ReadFieldBYTEFn CResGFF::readFieldBYTE = nullptr;
CResGFF::ReadFieldCHARFn CResGFF::readFieldCHAR = nullptr;
CResGFF::ReadFieldWORDFn CResGFF::readFieldWORD = nullptr;
CResGFF::ReadFieldSHORTFn CResGFF::readFieldSHORT = nullptr;
CResGFF::ReadFieldDWORDFn CResGFF::readFieldDWORD = nullptr;
CResGFF::ReadFieldDWORD64Fn CResGFF::readFieldDWORD64 = nullptr;
CResGFF::ReadFieldINTFn CResGFF::readFieldINT = nullptr;
CResGFF::ReadFieldFLOATFn CResGFF::readFieldFLOAT = nullptr;
CResGFF::ReadFieldCExoStringFn CResGFF::readFieldCExoString = nullptr;
CResGFF::ReadFieldCResRefFn CResGFF::readFieldCResRef = nullptr;
CResGFF::ReadFieldCExoLocStringFn CResGFF::readFieldCExoLocString = nullptr;
CResGFF::ReadFieldVOIDFn CResGFF::readFieldVOID = nullptr;
CResGFF::ReadFieldVectorFn CResGFF::readFieldVector = nullptr;
CResGFF::ReadFieldQuaternionFn CResGFF::readFieldQuaternion = nullptr;

// Write Field Functions
CResGFF::WriteFieldBYTEFn CResGFF::writeFieldBYTE = nullptr;
CResGFF::WriteFieldCHARFn CResGFF::writeFieldCHAR = nullptr;
CResGFF::WriteFieldWORDFn CResGFF::writeFieldWORD = nullptr;
CResGFF::WriteFieldSHORTFn CResGFF::writeFieldSHORT = nullptr;
CResGFF::WriteFieldINTFn CResGFF::writeFieldINT = nullptr;
CResGFF::WriteFieldDWORDFn CResGFF::writeFieldDWORD = nullptr;
CResGFF::WriteFieldDWORD64Fn CResGFF::writeFieldDWORD64 = nullptr;
CResGFF::WriteFieldFLOATFn CResGFF::writeFieldFLOAT = nullptr;
CResGFF::WriteFieldCExoStringFn CResGFF::writeFieldCExoString = nullptr;
CResGFF::WriteFieldCResRefFn CResGFF::writeFieldCResRef = nullptr;
CResGFF::WriteFieldCExoLocStringFn CResGFF::writeFieldCExoLocString = nullptr;
CResGFF::WriteFieldVOIDFn CResGFF::writeFieldVOID = nullptr;
CResGFF::WriteFieldVectorFn CResGFF::writeFieldVector = nullptr;
CResGFF::WriteFieldQuaternionFn CResGFF::writeFieldQuaternion = nullptr;

// File/Resource Operations
CResGFF::InitializeForWritingFn CResGFF::initializeForWriting = nullptr;
CResGFF::CreateGFFFileFn CResGFF::createGFFFile = nullptr;
CResGFF::WriteGFFDataFn CResGFF::writeGFFData = nullptr;
CResGFF::WriteGFFFileFn CResGFF::writeGFFFile = nullptr;
CResGFF::PackFn CResGFF::pack = nullptr;
CResGFF::OnResourceFreedFn CResGFF::onResourceFreed = nullptr;
CResGFF::OnResourceServicedFn CResGFF::onResourceServiced = nullptr;
CResGFF::ReleaseResourceFn CResGFF::releaseResource = nullptr;
CResGFF::GetTotalSizeFn CResGFF::getTotalSize = nullptr;

// Static offsets
int CResGFF::offsetHeader = -1;
int CResGFF::offsetStructs = -1;
int CResGFF::offsetFields = -1;
int CResGFF::offsetLabels = -1;
int CResGFF::offsetFieldType = -1;

// ===== INITIALIZATION FUNCTIONS =====

void CResGFF::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    // CRITICAL: Initialize parent class functions first
    CRes::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CResGFF] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Constructors/Destructors
        constructor = reinterpret_cast<ConstructorFn>(
            GameVersion::GetFunctionAddress("CResGFF", "Constructor")
        );
        constructor2 = reinterpret_cast<Constructor2Fn>(
            GameVersion::GetFunctionAddress("CResGFF", "Constructor_2")
        );
        destructor = reinterpret_cast<DestructorFn>(
            GameVersion::GetFunctionAddress("CResGFF", "Destructor")
        );
        destructor2 = reinterpret_cast<Destructor2Fn>(
            GameVersion::GetFunctionAddress("CResGFF", "Destructor_2")
        );
        destructor3 = reinterpret_cast<Destructor3Fn>(
            GameVersion::GetFunctionAddress("CResGFF", "Destructor_3")
        );

        // Data Management Functions
        addDataField = reinterpret_cast<AddDataFieldFn>(
            GameVersion::GetFunctionAddress("CResGFF", "AddDataField")
        );
        addDataLayoutField = reinterpret_cast<AddDataLayoutFieldFn>(
            GameVersion::GetFunctionAddress("CResGFF", "AddDataLayoutField")
        );
        addDataLayoutList = reinterpret_cast<AddDataLayoutListFn>(
            GameVersion::GetFunctionAddress("CResGFF", "AddDataLayoutList")
        );
        getDataField = reinterpret_cast<GetDataFieldFn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetDataField")
        );
        getDataLayoutList = reinterpret_cast<GetDataLayoutListFn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetDataLayoutList")
        );

        // Structure/Field/Label Management Functions
        addStruct = reinterpret_cast<AddStructFn>(
            GameVersion::GetFunctionAddress("CResGFF", "AddStruct")
        );
        addField = reinterpret_cast<AddFieldFn>(
            GameVersion::GetFunctionAddress("CResGFF", "AddField")
        );
        addLabel = reinterpret_cast<AddLabelFn>(
            GameVersion::GetFunctionAddress("CResGFF", "AddLabel")
        );
        addList = reinterpret_cast<AddListFn>(
            GameVersion::GetFunctionAddress("CResGFF", "AddList")
        );
        addListElement = reinterpret_cast<AddListElementFn>(
            GameVersion::GetFunctionAddress("CResGFF", "AddListElement")
        );
        addStructToStruct = reinterpret_cast<AddStructToStructFn>(
            GameVersion::GetFunctionAddress("CResGFF", "AddStructToStruct")
        );
        getField = reinterpret_cast<GetFieldFn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetField")
        );
        getField2 = reinterpret_cast<GetField2Fn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetField_2")
        );
        getFieldByLabel = reinterpret_cast<GetFieldByLabelFn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetFieldByLabel")
        );
        getFieldCount = reinterpret_cast<GetFieldCountFn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetFieldCount")
        );
        getFieldType = reinterpret_cast<GetFieldTypeFn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetFieldType")
        );
        getElementType = reinterpret_cast<GetElementTypeFn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetElementType")
        );
        getList = reinterpret_cast<GetListFn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetList")
        );
        getListCount = reinterpret_cast<GetListCountFn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetListCount")
        );
        getListElement = reinterpret_cast<GetListElementFn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetListElement")
        );
        getStructFromStruct = reinterpret_cast<GetStructFromStructFn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetStructFromStruct")
        );
        getTopLevelStruct = reinterpret_cast<GetTopLevelStructFn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetTopLevelStruct")
        );

        // Read Field Functions
        readFieldBYTE = reinterpret_cast<ReadFieldBYTEFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldBYTE")
        );
        readFieldCHAR = reinterpret_cast<ReadFieldCHARFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldCHAR")
        );
        readFieldWORD = reinterpret_cast<ReadFieldWORDFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldWORD")
        );
        readFieldSHORT = reinterpret_cast<ReadFieldSHORTFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldSHORT")
        );
        readFieldDWORD = reinterpret_cast<ReadFieldDWORDFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldDWORD")
        );
        readFieldDWORD64 = reinterpret_cast<ReadFieldDWORD64Fn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldDWORD64")
        );
        readFieldINT = reinterpret_cast<ReadFieldINTFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldINT")
        );
        readFieldFLOAT = reinterpret_cast<ReadFieldFLOATFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldFLOAT")
        );
        readFieldCExoString = reinterpret_cast<ReadFieldCExoStringFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldCExoString")
        );
        readFieldCResRef = reinterpret_cast<ReadFieldCResRefFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldCResRef")
        );
        readFieldCExoLocString = reinterpret_cast<ReadFieldCExoLocStringFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldCExoLocString")
        );
        readFieldVOID = reinterpret_cast<ReadFieldVOIDFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldVOID")
        );
        readFieldVector = reinterpret_cast<ReadFieldVectorFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldVector")
        );
        readFieldQuaternion = reinterpret_cast<ReadFieldQuaternionFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReadFieldQuaternion")
        );

        // Write Field Functions
        writeFieldBYTE = reinterpret_cast<WriteFieldBYTEFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldBYTE")
        );
        writeFieldCHAR = reinterpret_cast<WriteFieldCHARFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldCHAR")
        );
        writeFieldWORD = reinterpret_cast<WriteFieldWORDFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldWORD")
        );
        writeFieldSHORT = reinterpret_cast<WriteFieldSHORTFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldSHORT")
        );
        writeFieldINT = reinterpret_cast<WriteFieldINTFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldINT")
        );
        writeFieldDWORD = reinterpret_cast<WriteFieldDWORDFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldDWORD")
        );
        writeFieldDWORD64 = reinterpret_cast<WriteFieldDWORD64Fn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldDWORD64")
        );
        writeFieldFLOAT = reinterpret_cast<WriteFieldFLOATFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldFLOAT")
        );
        writeFieldCExoString = reinterpret_cast<WriteFieldCExoStringFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldCExoString")
        );
        writeFieldCResRef = reinterpret_cast<WriteFieldCResRefFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldCResRef")
        );
        writeFieldCExoLocString = reinterpret_cast<WriteFieldCExoLocStringFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldCExoLocString")
        );
        writeFieldVOID = reinterpret_cast<WriteFieldVOIDFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldVOID")
        );
        writeFieldVector = reinterpret_cast<WriteFieldVectorFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldVector")
        );
        writeFieldQuaternion = reinterpret_cast<WriteFieldQuaternionFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteFieldQuaternion")
        );

        // File/Resource Operations
        initializeForWriting = reinterpret_cast<InitializeForWritingFn>(
            GameVersion::GetFunctionAddress("CResGFF", "InitializeForWriting")
        );
        createGFFFile = reinterpret_cast<CreateGFFFileFn>(
            GameVersion::GetFunctionAddress("CResGFF", "CreateGFFFile")
        );
        writeGFFData = reinterpret_cast<WriteGFFDataFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteGFFData")
        );
        writeGFFFile = reinterpret_cast<WriteGFFFileFn>(
            GameVersion::GetFunctionAddress("CResGFF", "WriteGFFFile")
        );
        pack = reinterpret_cast<PackFn>(
            GameVersion::GetFunctionAddress("CResGFF", "Pack")
        );
        onResourceFreed = reinterpret_cast<OnResourceFreedFn>(
            GameVersion::GetFunctionAddress("CResGFF", "OnResourceFreed")
        );
        onResourceServiced = reinterpret_cast<OnResourceServicedFn>(
            GameVersion::GetFunctionAddress("CResGFF", "OnResourceServiced")
        );
        releaseResource = reinterpret_cast<ReleaseResourceFn>(
            GameVersion::GetFunctionAddress("CResGFF", "ReleaseResource")
        );
        getTotalSize = reinterpret_cast<GetTotalSizeFn>(
            GameVersion::GetFunctionAddress("CResGFF", "GetTotalSize")
        );

    } catch (const GameVersionException& e) {
        debugLog("[CResGFF] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CResGFF::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    // CRITICAL: Initialize parent class offsets first
    CRes::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CResGFF] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetHeader = GameVersion::GetOffset("CResGFF", "header");
        offsetStructs = GameVersion::GetOffset("CResGFF", "structs");
        offsetFields = GameVersion::GetOffset("CResGFF", "fields");
        offsetLabels = GameVersion::GetOffset("CResGFF", "labels");
        offsetFieldType = GameVersion::GetOffset("CResGFF", "field_type");

        offsetsInitialized = true;
    } catch (const GameVersionException& e) {
        debugLog("[CResGFF] ERROR: %s\n", e.what());
    }
}

// ===== CONSTRUCTORS/DESTRUCTOR =====

CResGFF::CResGFF(void* objectPtr)
    : CRes(objectPtr)  // Call parent wrapping constructor
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CResGFF::CResGFF()
    : CRes(nullptr)  // Use wrapping constructor variant
{
    shouldFree = true;  // Manually set flag

    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    objectPtr = malloc(OBJECT_SIZE);  // 0xa0, not 0x28!
    if (!objectPtr) {
        debugLog("[CResGFF] ERROR: Failed to allocate memory\n");
        shouldFree = false;
        return;
    }

    if (constructor) {
        constructor(objectPtr);
    } else {
        debugLog("[CResGFF] ERROR: Constructor not initialized\n");
        free(objectPtr);
        objectPtr = nullptr;
        shouldFree = false;
    }
}

CResGFF::CResGFF(ResourceType resourceType, char* GFFtype, CResRef* templateResRef)
    : CRes(nullptr)  // Use wrapping constructor variant
{
    shouldFree = true;

    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    objectPtr = malloc(OBJECT_SIZE);
    if (!objectPtr) {
        debugLog("[CResGFF] ERROR: Failed to allocate memory\n");
        shouldFree = false;
        return;
    }

    if (constructor2) {
        constructor2(objectPtr, resourceType, GFFtype, templateResRef ? templateResRef->GetPtr() : nullptr);
    } else {
        debugLog("[CResGFF] ERROR: Constructor_2 not initialized\n");
        free(objectPtr);
        objectPtr = nullptr;
        shouldFree = false;
    }
}

CResGFF::~CResGFF() {
    if (shouldFree && objectPtr) {
        if (destructor) {
            destructor(objectPtr);
        }
        free(objectPtr);
    }
    // Base class (CRes) destructor handles rest
}

// ===== OFFSET ACCESSORS =====

GFFHeaderInfo* CResGFF::GetHeader() {
    if (!objectPtr || offsetHeader < 0) {
        return nullptr;
    }
    return getObjectProperty<GFFHeaderInfo*>(objectPtr, offsetHeader);
}

GFFStructData* CResGFF::GetStructs() {
    if (!objectPtr || offsetStructs < 0) {
        return nullptr;
    }
    return getObjectProperty<GFFStructData*>(objectPtr, offsetStructs);
}

GFFFieldData* CResGFF::GetFields() {
    if (!objectPtr || offsetFields < 0) {
        return nullptr;
    }
    return getObjectProperty<GFFFieldData*>(objectPtr, offsetFields);
}

char* CResGFF::GetLabels() {
    if (!objectPtr || offsetLabels < 0) {
        return nullptr;
    }
    return getObjectProperty<char*>(objectPtr, offsetLabels);
}

char* CResGFF::GetFieldType() {
    if (!objectPtr || offsetFieldType < 0) {
        return nullptr;
    }
    return static_cast<char*>(objectPtr) + offsetFieldType;
}

// ===== DATA MANAGEMENT FUNCTIONS =====

int CResGFF::AddDataField(int existingBlockOffset, DWORD sizeOfExistingBlock, DWORD dataLength) {
    if (!objectPtr || !addDataField) {
        return 0;
    }
    return addDataField(objectPtr, existingBlockOffset, sizeOfExistingBlock, dataLength);
}

int CResGFF::AddDataLayoutField(int existingBlockOffset, DWORD sizeOfExistingBlock, DWORD dataLength) {
    if (!objectPtr || !addDataLayoutField) {
        return 0;
    }
    return addDataLayoutField(objectPtr, existingBlockOffset, sizeOfExistingBlock, dataLength);
}

int CResGFF::AddDataLayoutList(int existingBlockOffset, DWORD sizeOfExistingBlock, DWORD dataLength) {
    if (!objectPtr || !addDataLayoutList) {
        return 0;
    }
    return addDataLayoutList(objectPtr, existingBlockOffset, sizeOfExistingBlock, dataLength);
}

void* CResGFF::GetDataField(CResGFFField* field, DWORD* dataSize) {
    if (!objectPtr || !getDataField) {
        return nullptr;
    }
    return getDataField(objectPtr, field, dataSize);
}

void* CResGFF::GetDataLayoutList(CResGFFField* field, DWORD* dataLength) {
    if (!objectPtr || !getDataLayoutList) {
        return nullptr;
    }
    return getDataLayoutList(objectPtr, field, dataLength);
}

// ===== STRUCTURE/FIELD/LABEL MANAGEMENT =====

DWORD CResGFF::AddStruct(DWORD id) {
    if (!objectPtr || !addStruct) {
        return 0;
    }
    return addStruct(objectPtr, id);
}

CResGFFField* CResGFF::AddField(CResStruct* structPtr, char* labelText, GFFFieldTypes fieldType) {
    if (!objectPtr || !addField) {
        return nullptr;
    }
    return addField(objectPtr, structPtr, labelText, fieldType);
}

DWORD CResGFF::AddLabel(char* text) {
    if (!objectPtr || !addLabel) {
        return 0;
    }
    return addLabel(objectPtr, text);
}

int CResGFF::AddList(CResList* list, CResStruct* structPtr, char* labelText) {
    if (!objectPtr || !addList) {
        return 0;
    }
    return addList(objectPtr, list, structPtr, labelText);
}

int CResGFF::AddListElement(CResStruct* structPtr, CResList* list, DWORD structId) {
    if (!objectPtr || !addListElement) {
        return 0;
    }
    return addListElement(objectPtr, structPtr, list, structId);
}

int CResGFF::AddStructToStruct(CResStruct* parentStruct, CResStruct* childStruct, char* labelText, DWORD structId) {
    if (!objectPtr || !addStructToStruct) {
        return 0;
    }
    return addStructToStruct(objectPtr, parentStruct, childStruct, labelText, structId);
}

CResGFFField* CResGFF::GetField(CResGFFStruct* structData, DWORD index) {
    if (!objectPtr || !getField) {
        return nullptr;
    }
    return getField(objectPtr, structData, index);
}

CResGFFField* CResGFF::GetField2(CResStruct* structPtr, DWORD fieldIndex) {
    if (!objectPtr || !getField2) {
        return nullptr;
    }
    return getField2(objectPtr, structPtr, fieldIndex);
}

CResGFFField* CResGFF::GetFieldByLabel(CResStruct* structPtr, char* label) {
    if (!objectPtr || !getFieldByLabel) {
        return nullptr;
    }
    return getFieldByLabel(objectPtr, structPtr, label);
}

DWORD CResGFF::GetFieldCount(CResStruct* structPtr) {
    if (!objectPtr || !getFieldCount) {
        return 0;
    }
    return getFieldCount(objectPtr, structPtr);
}

GFFFieldTypes CResGFF::GetFieldType(CResStruct* structPtr, char* label, DWORD fieldIndex) {
    if (!objectPtr || !getFieldType) {
        return GFF_NONE;  // Default to first enum value
    }
    return getFieldType(objectPtr, structPtr, label, fieldIndex);
}

DWORD CResGFF::GetElementType(CResStruct* structPtr) {
    if (!objectPtr || !getElementType) {
        return 0;
    }
    return getElementType(objectPtr, structPtr);
}

int CResGFF::GetList(CResList* outList, CResStruct* structPtr, char* label) {
    if (!objectPtr || !getList) {
        return 0;
    }
    return getList(objectPtr, outList, structPtr, label);
}

DWORD CResGFF::GetListCount(CResList* list) {
    if (!objectPtr || !getListCount) {
        return 0;
    }
    return getListCount(objectPtr, list);
}

int CResGFF::GetListElement(CResStruct* structPtr, CResList* list, DWORD index) {
    if (!objectPtr || !getListElement) {
        return 0;
    }
    return getListElement(objectPtr, structPtr, list, index);
}

int CResGFF::GetStructFromStruct(CResStruct* outStruct, CResStruct* inStruct, char* label) {
    if (!objectPtr || !getStructFromStruct) {
        return 0;
    }
    return getStructFromStruct(objectPtr, outStruct, inStruct, label);
}

void CResGFF::GetTopLevelStruct(CResStruct* outStruct) {
    if (!objectPtr || !getTopLevelStruct) {
        return;
    }
    getTopLevelStruct(objectPtr, outStruct);
}

// ===== READ FIELD FUNCTIONS =====

BYTE CResGFF::ReadFieldBYTE(CResStruct* structPtr, char* label, int* success, BYTE defaultValue) {
    if (!objectPtr || !readFieldBYTE) {
        if (success) *success = 0;
        return defaultValue;
    }
    return readFieldBYTE(objectPtr, structPtr, label, success, defaultValue);
}

char CResGFF::ReadFieldCHAR(CResStruct* structPtr, char* label, int* success, char defaultValue) {
    if (!objectPtr || !readFieldCHAR) {
        if (success) *success = 0;
        return defaultValue;
    }
    return readFieldCHAR(objectPtr, structPtr, label, success, defaultValue);
}

WORD CResGFF::ReadFieldWORD(CResStruct* structPtr, char* label, int* success, WORD defaultValue) {
    if (!objectPtr || !readFieldWORD) {
        if (success) *success = 0;
        return defaultValue;
    }
    return readFieldWORD(objectPtr, structPtr, label, success, defaultValue);
}

short CResGFF::ReadFieldSHORT(CResStruct* structPtr, char* label, int* success, short defaultValue) {
    if (!objectPtr || !readFieldSHORT) {
        if (success) *success = 0;
        return defaultValue;
    }
    return readFieldSHORT(objectPtr, structPtr, label, success, defaultValue);
}

DWORD CResGFF::ReadFieldDWORD(CResStruct* structPtr, char* label, int* success, DWORD defaultValue) {
    if (!objectPtr || !readFieldDWORD) {
        if (success) *success = 0;
        return defaultValue;
    }
    return readFieldDWORD(objectPtr, structPtr, label, success, defaultValue);
}

DWORD64 CResGFF::ReadFieldDWORD64(CResStruct* structPtr, char* label, int* success, DWORD64 defaultValue) {
    if (!objectPtr || !readFieldDWORD64) {
        if (success) *success = 0;
        return defaultValue;
    }
    return readFieldDWORD64(objectPtr, structPtr, label, success, defaultValue);
}

int CResGFF::ReadFieldINT(CResStruct* structPtr, char* label, int* success, int defaultValue) {
    if (!objectPtr || !readFieldINT) {
        if (success) *success = 0;
        return defaultValue;
    }
    return readFieldINT(objectPtr, structPtr, label, success, defaultValue);
}

float CResGFF::ReadFieldFLOAT(CResStruct* structPtr, char* label, int* success, float defaultValue) {
    if (!objectPtr || !readFieldFLOAT) {
        if (success) *success = 0;
        return defaultValue;
    }
    return readFieldFLOAT(objectPtr, structPtr, label, success, defaultValue);
}

CExoString* CResGFF::ReadFieldCExoString(CExoString* out, CResStruct* structPtr, char* label, int* success, CExoString* defaultValue) {
    if (!objectPtr || !readFieldCExoString) {
        if (success) *success = 0;
        return defaultValue;
    }
    return readFieldCExoString(objectPtr, out, structPtr, label, success, defaultValue);
}

CResRef* CResGFF::ReadFieldCResRef(CResRef* out, CResStruct* structPtr, char* label, int* success, CResRef* defaultValue) {
    if (!objectPtr || !readFieldCResRef) {
        if (success) *success = 0;
        return defaultValue;
    }
    return readFieldCResRef(objectPtr, out, structPtr, label, success, defaultValue);
}

void* CResGFF::ReadFieldCExoLocString(void* out, CResStruct* structPtr, char* label, int* success, void* defaultValue) {
    if (!objectPtr || !readFieldCExoLocString) {
        if (success) *success = 0;
        return defaultValue;
    }
    // TODO: Replace void* with CExoLocString* once that class is implemented
    return readFieldCExoLocString(objectPtr, out, structPtr, label, success, defaultValue);
}

void CResGFF::ReadFieldVOID(CResStruct* structPtr, void* buffer, DWORD size, char* label, int* success, void* defaultValue) {
    if (!objectPtr || !readFieldVOID || !success) {
        if (success) *success = 0;
        return;
    }
    readFieldVOID(objectPtr, structPtr, buffer, size, label, success, defaultValue);
}

Vector* CResGFF::ReadFieldVector(Vector* out, CResStruct* structPtr, char* label, int* success, Vector* defaultValue) {
    if (!objectPtr || !readFieldVector) {
        if (success) *success = 0;
        return defaultValue;
    }
    return readFieldVector(objectPtr, out, structPtr, label, success, defaultValue);
}

Quaternion* CResGFF::ReadFieldQuaternion(Quaternion* out, CResStruct* structPtr, char* label, int* success, Quaternion* defaultValue) {
    if (!objectPtr || !readFieldQuaternion) {
        if (success) *success = 0;
        return defaultValue;
    }
    return readFieldQuaternion(objectPtr, out, structPtr, label, success, defaultValue);
}

// ===== WRITE FIELD FUNCTIONS =====

int CResGFF::WriteFieldBYTE(CResStruct* structPtr, BYTE value, char* label) {
    if (!objectPtr || !writeFieldBYTE) {
        return 0;
    }
    return writeFieldBYTE(objectPtr, structPtr, value, label);
}

int CResGFF::WriteFieldCHAR(CResStruct* structPtr, char value, char* label) {
    if (!objectPtr || !writeFieldCHAR) {
        return 0;
    }
    return writeFieldCHAR(objectPtr, structPtr, value, label);
}

int CResGFF::WriteFieldWORD(CResStruct* structPtr, WORD value, char* label) {
    if (!objectPtr || !writeFieldWORD) {
        return 0;
    }
    return writeFieldWORD(objectPtr, structPtr, value, label);
}

int CResGFF::WriteFieldSHORT(CResStruct* structPtr, short value, char* label) {
    if (!objectPtr || !writeFieldSHORT) {
        return 0;
    }
    return writeFieldSHORT(objectPtr, structPtr, value, label);
}

int CResGFF::WriteFieldINT(CResStruct* structPtr, int value, char* label) {
    if (!objectPtr || !writeFieldINT) {
        return 0;
    }
    return writeFieldINT(objectPtr, structPtr, value, label);
}

int CResGFF::WriteFieldDWORD(CResStruct* structPtr, DWORD value, char* label) {
    if (!objectPtr || !writeFieldDWORD) {
        return 0;
    }
    return writeFieldDWORD(objectPtr, structPtr, value, label);
}

int CResGFF::WriteFieldDWORD64(CResStruct* structPtr, DWORD64 value, char* label) {
    if (!objectPtr || !writeFieldDWORD64) {
        return 0;
    }
    return writeFieldDWORD64(objectPtr, structPtr, value, label);
}

int CResGFF::WriteFieldFLOAT(CResStruct* structPtr, float value, char* label) {
    if (!objectPtr || !writeFieldFLOAT) {
        return 0;
    }
    return writeFieldFLOAT(objectPtr, structPtr, value, label);
}

int CResGFF::WriteFieldCExoString(CResStruct* structPtr, CExoString* value, char* label) {
    if (!objectPtr || !writeFieldCExoString) {
        return 0;
    }
    return writeFieldCExoString(objectPtr, structPtr, value, label);
}

int CResGFF::WriteFieldCResRef(CResStruct* structPtr, CResRef* value, char* label) {
    if (!objectPtr || !writeFieldCResRef) {
        return 0;
    }
    return writeFieldCResRef(objectPtr, structPtr, value, label);
}

int CResGFF::WriteFieldCExoLocString(CResStruct* structPtr, void* value, char* label) {
    if (!objectPtr || !writeFieldCExoLocString) {
        return 0;
    }
    // TODO: Replace void* with CExoLocString* once that class is implemented
    return writeFieldCExoLocString(objectPtr, structPtr, value, label);
}

int CResGFF::WriteFieldVOID(CResStruct* structPtr, void* data, DWORD size, char* label) {
    if (!objectPtr || !writeFieldVOID) {
        return 0;
    }
    return writeFieldVOID(objectPtr, structPtr, data, size, label);
}

int CResGFF::WriteFieldVector(CResStruct* structPtr, Vector* value, char* label) {
    if (!objectPtr || !writeFieldVector) {
        return 0;
    }
    return writeFieldVector(objectPtr, structPtr, value, label);
}

int CResGFF::WriteFieldQuaternion(CResStruct* structPtr, Quaternion* value, char* label) {
    if (!objectPtr || !writeFieldQuaternion) {
        return 0;
    }
    return writeFieldQuaternion(objectPtr, structPtr, value, label);
}

// ===== FILE/RESOURCE OPERATIONS =====

void CResGFF::InitializeForWriting() {
    if (!objectPtr || !initializeForWriting) {
        return;
    }
    initializeForWriting(objectPtr);
}

void CResGFF::CreateGFFFile(CResStruct* structPtr, CExoString* fileType, CExoString* version) {
    if (!objectPtr || !createGFFFile) {
        return;
    }
    createGFFFile(objectPtr, structPtr, fileType, version);
}

void CResGFF::WriteGFFData(CExoFile* file, DWORD* totalBytes) {
    if (!objectPtr || !writeGFFData) {
        return;
    }
    writeGFFData(objectPtr, file, totalBytes);
}

void CResGFF::WriteGFFFile(CExoString* name, ResourceType type) {
    if (!objectPtr || !writeGFFFile) {
        return;
    }
    writeGFFFile(objectPtr, name, type);
}

void CResGFF::Pack(byte alwaysZero1, DWORD alwaysZero2) {
    if (!objectPtr || !pack) {
        return;
    }
    pack(objectPtr, alwaysZero1, alwaysZero2);
}

void CResGFF::OnResourceFreed() {
    if (!objectPtr || !onResourceFreed) {
        return;
    }
    onResourceFreed(objectPtr);
}

void CResGFF::OnResourceServiced() {
    if (!objectPtr || !onResourceServiced) {
        return;
    }
    onResourceServiced(objectPtr);
}

void CResGFF::ReleaseResource() {
    if (!objectPtr || !releaseResource) {
        return;
    }
    releaseResource(objectPtr);
}

DWORD CResGFF::GetTotalSize() {
    if (!objectPtr || !getTotalSize) {
        return 0;
    }
    return getTotalSize(objectPtr);
}
