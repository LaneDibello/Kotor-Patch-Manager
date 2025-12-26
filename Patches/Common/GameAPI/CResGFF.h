#pragma once
#include <windows.h>
#include "../Common.h"
#include "GameVersion.h"
#include "CResRef.h"
#include "CRes.h"
#include "CExoString.h"

typedef enum GFFFieldTypes {
    GFF_BYTE = 0,
    GFF_CHAR = 1,
    GFF_WORD = 2,
    GFF_SHORT = 3,
    GFF_DWORD = 4,
    GFF_DWORD64 = 5,
    GFF_INT = 6,
    GFF_INT64 = 7,
    GFF_FLOAT = 8,
    GFF_DOUBLE = 9,
    GFF_CExoString = 10,
    GFF_ResRef = 11,
    GFF_CExoLocString = 12,
    GFF_VOID = 13,
    GFF_Struct = 14,
    GFF_List = 15,
    GFF_Orientation = 16,
    GFF_Vector = 17,
    GFF_StrRef = 18,
    GFF_NONE = 0xFFFFFFFF
} GFFFieldTypes;

struct GFFStructData {
    DWORD id;
    DWORD data_or_data_offset;
    DWORD field_count;
};

struct GFFFieldData {
    enum GFFFieldTypes field_type;
    DWORD label_index;
    DWORD data_or_data_offset;
};

struct GFFHeaderInfo {
    char file_type[4];
    char file_version[4];
    DWORD struct_offset;
    DWORD struct_count;
    DWORD field_offset;
    DWORD field_count;
    DWORD label_offset;
    DWORD label_count;
    DWORD field_data_offset;
    DWORD field_data_count;
    DWORD field_indices_offset;
    DWORD field_indices_count;
    DWORD list_indices_offset;
    DWORD list_indices_count;
};

struct CResStruct {
    DWORD index;
};

struct CResList {
    CResStruct resStruct;
    char label[16];
};

struct CResGFFField {
    enum GFFFieldTypes field_type;
    DWORD label_index;
    DWORD data_or_data_offset;
};

struct CResGFFStruct {
    DWORD id;
    DWORD data_or_data_offset;
    DWORD field_count;
};

// These will later be replaced by an actual class
// But for now we'll just use a struct
// TODO: Convert CExoFile into a GameAPI class
struct CExoFile {
    struct CExoFileInternal* internal;
};

struct CExoFileInternal {
    FILE* fp;
    CExoString file_name;
    CExoString mode;
};

class CResGFF : public CRes {
private:
    static constexpr size_t OBJECT_SIZE = 0xa0;  // 160 bytes

    // Static initialization flags
    static bool functionsInitialized;
    static bool offsetsInitialized;

    // ===== FUNCTION POINTER TYPEDEFS =====

    // Constructors/Destructors
    typedef void(__thiscall* ConstructorFn)(void* thisPtr);
    typedef void(__thiscall* Constructor2Fn)(void* thisPtr, ResourceType resourceType, char* GFFtype, void* templateResRef);
    typedef void(__thiscall* DestructorFn)(void* thisPtr);
    typedef void(__thiscall* Destructor2Fn)(void* thisPtr, byte shouldFree);
    typedef void(__thiscall* Destructor3Fn)(void* thisPtr);

    // Data Management Functions
    typedef int(__thiscall* AddDataFieldFn)(void* thisPtr, int existingBlockOffset, DWORD sizeOfExistingBlock, DWORD dataLength);
    typedef int(__thiscall* AddDataLayoutFieldFn)(void* thisPtr, int existingBlockOffset, DWORD sizeOfExistingBlock, DWORD dataLength);
    typedef int(__thiscall* AddDataLayoutListFn)(void* thisPtr, int existingBlockOffset, DWORD sizeOfExistingBlock, DWORD dataLength);
    typedef void*(__thiscall* GetDataFieldFn)(void* thisPtr, CResGFFField* field, DWORD* dataSize);
    typedef void*(__thiscall* GetDataLayoutListFn)(void* thisPtr, CResGFFField* field, DWORD* dataLength);

    // Structure/Field/Label Management Functions
    typedef DWORD(__thiscall* AddStructFn)(void* thisPtr, DWORD id);
    typedef CResGFFField*(__thiscall* AddFieldFn)(void* thisPtr, CResStruct* structPtr, char* labelText, GFFFieldTypes fieldType);
    typedef DWORD(__thiscall* AddLabelFn)(void* thisPtr, char* text);
    typedef int(__thiscall* AddListFn)(void* thisPtr, CResList* list, CResStruct* structPtr, char* labelText);
    typedef int(__thiscall* AddListElementFn)(void* thisPtr, CResStruct* structPtr, CResList* list, DWORD structId);
    typedef int(__thiscall* AddStructToStructFn)(void* thisPtr, CResStruct* parentStruct, CResStruct* childStruct, char* labelText, DWORD structId);
    typedef CResGFFField*(__thiscall* GetFieldFn)(void* thisPtr, CResGFFStruct* structData, DWORD index);
    typedef CResGFFField*(__thiscall* GetField2Fn)(void* thisPtr, CResStruct* structPtr, DWORD fieldIndex);
    typedef CResGFFField*(__thiscall* GetFieldByLabelFn)(void* thisPtr, CResStruct* structPtr, char* label);
    typedef DWORD(__thiscall* GetFieldCountFn)(void* thisPtr, CResStruct* structPtr);
    typedef GFFFieldTypes(__thiscall* GetFieldTypeFn)(void* thisPtr, CResStruct* structPtr, char* label, DWORD fieldIndex);
    typedef DWORD(__thiscall* GetElementTypeFn)(void* thisPtr, CResStruct* structPtr);
    typedef int(__thiscall* GetListFn)(void* thisPtr, CResList* outList, CResStruct* structPtr, char* label);
    typedef DWORD(__thiscall* GetListCountFn)(void* thisPtr, CResList* list);
    typedef int(__thiscall* GetListElementFn)(void* thisPtr, CResStruct* structPtr, CResList* list, DWORD index);
    typedef int(__thiscall* GetStructFromStructFn)(void* thisPtr, CResStruct* outStruct, CResStruct* inStruct, char* label);
    typedef void(__thiscall* GetTopLevelStructFn)(void* thisPtr, CResStruct* outStruct);

    // Read Field Functions
    typedef BYTE(__thiscall* ReadFieldBYTEFn)(void* thisPtr, CResStruct* structPtr, char* label, int* success, BYTE defaultValue);
    typedef char(__thiscall* ReadFieldCHARFn)(void* thisPtr, CResStruct* structPtr, char* label, int* success, char defaultValue);
    typedef WORD(__thiscall* ReadFieldWORDFn)(void* thisPtr, CResStruct* structPtr, char* label, int* success, WORD defaultValue);
    typedef short(__thiscall* ReadFieldSHORTFn)(void* thisPtr, CResStruct* structPtr, char* label, int* success, short defaultValue);
    typedef DWORD(__thiscall* ReadFieldDWORDFn)(void* thisPtr, CResStruct* structPtr, char* label, int* success, DWORD defaultValue);
    typedef DWORD64(__thiscall* ReadFieldDWORD64Fn)(void* thisPtr, CResStruct* structPtr, char* label, int* success, DWORD64 defaultValue);
    typedef int(__thiscall* ReadFieldINTFn)(void* thisPtr, CResStruct* structPtr, char* label, int* success, int defaultValue);
    typedef float(__thiscall* ReadFieldFLOATFn)(void* thisPtr, CResStruct* structPtr, char* label, int* success, float defaultValue);
    typedef CExoString*(__thiscall* ReadFieldCExoStringFn)(void* thisPtr, CExoString* out, CResStruct* structPtr, char* label, int* success, CExoString* defaultValue);
    typedef CResRef*(__thiscall* ReadFieldCResRefFn)(void* thisPtr, CResRef* out, CResStruct* structPtr, char* label, int* success, CResRef* defaultValue);
    typedef void*(__thiscall* ReadFieldCExoLocStringFn)(void* thisPtr, void* out, CResStruct* structPtr, char* label, int* success, void* defaultValue);  // TODO: CExoLocString
    typedef void(__thiscall* ReadFieldVOIDFn)(void* thisPtr, CResStruct* structPtr, void* buffer, DWORD size, char* label, int* success, void* defaultValue);
    typedef Vector*(__thiscall* ReadFieldVectorFn)(void* thisPtr, Vector* out, CResStruct* structPtr, char* label, int* success, Vector* defaultValue);
    typedef Quaternion*(__thiscall* ReadFieldQuaternionFn)(void* thisPtr, Quaternion* out, CResStruct* structPtr, char* label, int* success, Quaternion* defaultValue);

    // Write Field Functions
    typedef int(__thiscall* WriteFieldBYTEFn)(void* thisPtr, CResStruct* structPtr, BYTE value, char* label);
    typedef int(__thiscall* WriteFieldCHARFn)(void* thisPtr, CResStruct* structPtr, char value, char* label);
    typedef int(__thiscall* WriteFieldWORDFn)(void* thisPtr, CResStruct* structPtr, WORD value, char* label);
    typedef int(__thiscall* WriteFieldSHORTFn)(void* thisPtr, CResStruct* structPtr, short value, char* label);
    typedef int(__thiscall* WriteFieldINTFn)(void* thisPtr, CResStruct* structPtr, int value, char* label);
    typedef int(__thiscall* WriteFieldDWORDFn)(void* thisPtr, CResStruct* structPtr, DWORD value, char* label);
    typedef int(__thiscall* WriteFieldDWORD64Fn)(void* thisPtr, CResStruct* structPtr, DWORD64 value, char* label);
    typedef int(__thiscall* WriteFieldFLOATFn)(void* thisPtr, CResStruct* structPtr, float value, char* label);
    typedef int(__thiscall* WriteFieldCExoStringFn)(void* thisPtr, CResStruct* structPtr, CExoString* value, char* label);
    typedef int(__thiscall* WriteFieldCResRefFn)(void* thisPtr, CResStruct* structPtr, CResRef* value, char* label);
    typedef int(__thiscall* WriteFieldCExoLocStringFn)(void* thisPtr, CResStruct* structPtr, void* value, char* label);  // TODO: CExoLocString
    typedef int(__thiscall* WriteFieldVOIDFn)(void* thisPtr, CResStruct* structPtr, void* data, DWORD size, char* label);
    typedef int(__thiscall* WriteFieldVectorFn)(void* thisPtr, CResStruct* structPtr, Vector* value, char* label);
    typedef int(__thiscall* WriteFieldQuaternionFn)(void* thisPtr, CResStruct* structPtr, Quaternion* value, char* label);

    // File/Resource Operations
    typedef void(__thiscall* InitializeForWritingFn)(void* thisPtr);
    typedef void(__thiscall* CreateGFFFileFn)(void* thisPtr, CResStruct* structPtr, CExoString* fileType, CExoString* version);
    typedef void(__thiscall* WriteGFFDataFn)(void* thisPtr, CExoFile* file, DWORD* totalBytes);
    typedef void(__thiscall* WriteGFFFileFn)(void* thisPtr, CExoString* name, ResourceType type);
    typedef void(__thiscall* PackFn)(void* thisPtr, byte alwaysZero1, DWORD alwaysZero2);
    typedef void(__thiscall* OnResourceFreedFn)(void* thisPtr);
    typedef void(__thiscall* OnResourceServicedFn)(void* thisPtr);
    typedef void(__thiscall* ReleaseResourceFn)(void* thisPtr);
    typedef DWORD(__thiscall* GetTotalSizeFn)(void* thisPtr);

    // ===== STATIC FUNCTION POINTERS =====

    // Constructors/Destructors
    static ConstructorFn constructor;
    static Constructor2Fn constructor2;
    static DestructorFn destructor;
    static Destructor2Fn destructor2;
    static Destructor3Fn destructor3;

    // Data Management
    static AddDataFieldFn addDataField;
    static AddDataLayoutFieldFn addDataLayoutField;
    static AddDataLayoutListFn addDataLayoutList;
    static GetDataFieldFn getDataField;
    static GetDataLayoutListFn getDataLayoutList;

    // Structure/Field/Label Management
    static AddStructFn addStruct;
    static AddFieldFn addField;
    static AddLabelFn addLabel;
    static AddListFn addList;
    static AddListElementFn addListElement;
    static AddStructToStructFn addStructToStruct;
    static GetFieldFn getField;
    static GetField2Fn getField2;
    static GetFieldByLabelFn getFieldByLabel;
    static GetFieldCountFn getFieldCount;
    static GetFieldTypeFn getFieldType;
    static GetElementTypeFn getElementType;
    static GetListFn getList;
    static GetListCountFn getListCount;
    static GetListElementFn getListElement;
    static GetStructFromStructFn getStructFromStruct;
    static GetTopLevelStructFn getTopLevelStruct;

    // Read Field Functions
    static ReadFieldBYTEFn readFieldBYTE;
    static ReadFieldCHARFn readFieldCHAR;
    static ReadFieldWORDFn readFieldWORD;
    static ReadFieldSHORTFn readFieldSHORT;
    static ReadFieldDWORDFn readFieldDWORD;
    static ReadFieldDWORD64Fn readFieldDWORD64;
    static ReadFieldINTFn readFieldINT;
    static ReadFieldFLOATFn readFieldFLOAT;
    static ReadFieldCExoStringFn readFieldCExoString;
    static ReadFieldCResRefFn readFieldCResRef;
    static ReadFieldCExoLocStringFn readFieldCExoLocString;
    static ReadFieldVOIDFn readFieldVOID;
    static ReadFieldVectorFn readFieldVector;
    static ReadFieldQuaternionFn readFieldQuaternion;

    // Write Field Functions
    static WriteFieldBYTEFn writeFieldBYTE;
    static WriteFieldCHARFn writeFieldCHAR;
    static WriteFieldWORDFn writeFieldWORD;
    static WriteFieldSHORTFn writeFieldSHORT;
    static WriteFieldINTFn writeFieldINT;
    static WriteFieldDWORDFn writeFieldDWORD;
    static WriteFieldDWORD64Fn writeFieldDWORD64;
    static WriteFieldFLOATFn writeFieldFLOAT;
    static WriteFieldCExoStringFn writeFieldCExoString;
    static WriteFieldCResRefFn writeFieldCResRef;
    static WriteFieldCExoLocStringFn writeFieldCExoLocString;
    static WriteFieldVOIDFn writeFieldVOID;
    static WriteFieldVectorFn writeFieldVector;
    static WriteFieldQuaternionFn writeFieldQuaternion;

    // File/Resource Operations
    static InitializeForWritingFn initializeForWriting;
    static CreateGFFFileFn createGFFFile;
    static WriteGFFDataFn writeGFFData;
    static WriteGFFFileFn writeGFFFile;
    static PackFn pack;
    static OnResourceFreedFn onResourceFreed;
    static OnResourceServicedFn onResourceServiced;
    static ReleaseResourceFn releaseResource;
    static GetTotalSizeFn getTotalSize;

    // ===== STATIC OFFSETS =====
    static int offsetHeader;
    static int offsetStructs;
    static int offsetFields;
    static int offsetLabels;
    static int offsetFieldType;

public:
    // ===== CONSTRUCTORS/DESTRUCTOR =====
    explicit CResGFF(void* objectPtr);  // Wrapping constructor
    CResGFF();  // Allocating default constructor
    CResGFF(ResourceType resourceType, char* GFFtype, CResRef* templateResRef);  // Allocating with args
    virtual ~CResGFF();

    // ===== OFFSET ACCESSORS =====
    GFFHeaderInfo* GetHeader();
    GFFStructData* GetStructs();
    GFFFieldData* GetFields();
    char* GetLabels();  // Returns pointer to array of char[16]
    char* GetFieldType();  // Returns char[4]

    // ===== DATA MANAGEMENT FUNCTIONS =====
    int AddDataField(int existingBlockOffset, DWORD sizeOfExistingBlock, DWORD dataLength);
    int AddDataLayoutField(int existingBlockOffset, DWORD sizeOfExistingBlock, DWORD dataLength);
    int AddDataLayoutList(int existingBlockOffset, DWORD sizeOfExistingBlock, DWORD dataLength);
    void* GetDataField(CResGFFField* field, DWORD* dataSize);
    void* GetDataLayoutList(CResGFFField* field, DWORD* dataLength);

    // ===== STRUCTURE/FIELD/LABEL MANAGEMENT =====
    DWORD AddStruct(DWORD id);
    CResGFFField* AddField(CResStruct* structPtr, char* labelText, GFFFieldTypes fieldType);
    DWORD AddLabel(char* text);
    int AddList(CResList* list, CResStruct* structPtr, char* labelText);
    int AddListElement(CResStruct* structPtr, CResList* list, DWORD structId);
    int AddStructToStruct(CResStruct* parentStruct, CResStruct* childStruct, char* labelText, DWORD structId);
    CResGFFField* GetField(CResGFFStruct* structData, DWORD index);
    CResGFFField* GetField2(CResStruct* structPtr, DWORD fieldIndex);
    CResGFFField* GetFieldByLabel(CResStruct* structPtr, char* label);
    DWORD GetFieldCount(CResStruct* structPtr);
    GFFFieldTypes GetFieldType(CResStruct* structPtr, char* label, DWORD fieldIndex);
    DWORD GetElementType(CResStruct* structPtr);
    int GetList(CResList* outList, CResStruct* structPtr, char* label);
    DWORD GetListCount(CResList* list);
    int GetListElement(CResStruct* structPtr, CResList* list, DWORD index);
    int GetStructFromStruct(CResStruct* outStruct, CResStruct* inStruct, char* label);
    void GetTopLevelStruct(CResStruct* outStruct);

    // ===== READ FIELD FUNCTIONS =====
    BYTE ReadFieldBYTE(CResStruct* structPtr, char* label, int* success, BYTE defaultValue);
    char ReadFieldCHAR(CResStruct* structPtr, char* label, int* success, char defaultValue);
    WORD ReadFieldWORD(CResStruct* structPtr, char* label, int* success, WORD defaultValue);
    short ReadFieldSHORT(CResStruct* structPtr, char* label, int* success, short defaultValue);
    DWORD ReadFieldDWORD(CResStruct* structPtr, char* label, int* success, DWORD defaultValue);
    DWORD64 ReadFieldDWORD64(CResStruct* structPtr, char* label, int* success, DWORD64 defaultValue);
    int ReadFieldINT(CResStruct* structPtr, char* label, int* success, int defaultValue);
    float ReadFieldFLOAT(CResStruct* structPtr, char* label, int* success, float defaultValue);
    CExoString* ReadFieldCExoString(CExoString* out, CResStruct* structPtr, char* label, int* success, CExoString* defaultValue);
    CResRef* ReadFieldCResRef(CResRef* out, CResStruct* structPtr, char* label, int* success, CResRef* defaultValue);
    void* ReadFieldCExoLocString(void* out, CResStruct* structPtr, char* label, int* success, void* defaultValue);  // TODO: Replace void* with CExoLocString* once implemented
    void ReadFieldVOID(CResStruct* structPtr, void* buffer, DWORD size, char* label, int* success, void* defaultValue);
    Vector* ReadFieldVector(Vector* out, CResStruct* structPtr, char* label, int* success, Vector* defaultValue);
    Quaternion* ReadFieldQuaternion(Quaternion* out, CResStruct* structPtr, char* label, int* success, Quaternion* defaultValue);

    // ===== WRITE FIELD FUNCTIONS =====
    int WriteFieldBYTE(CResStruct* structPtr, BYTE value, char* label);
    int WriteFieldCHAR(CResStruct* structPtr, char value, char* label);
    int WriteFieldWORD(CResStruct* structPtr, WORD value, char* label);
    int WriteFieldSHORT(CResStruct* structPtr, short value, char* label);
    int WriteFieldINT(CResStruct* structPtr, int value, char* label);
    int WriteFieldDWORD(CResStruct* structPtr, DWORD value, char* label);
    int WriteFieldDWORD64(CResStruct* structPtr, DWORD64 value, char* label);
    int WriteFieldFLOAT(CResStruct* structPtr, float value, char* label);
    int WriteFieldCExoString(CResStruct* structPtr, CExoString* value, char* label);
    int WriteFieldCResRef(CResStruct* structPtr, CResRef* value, char* label);
    int WriteFieldCExoLocString(CResStruct* structPtr, void* value, char* label);  // TODO: Replace void* with CExoLocString* once implemented
    int WriteFieldVOID(CResStruct* structPtr, void* data, DWORD size, char* label);
    int WriteFieldVector(CResStruct* structPtr, Vector* value, char* label);
    int WriteFieldQuaternion(CResStruct* structPtr, Quaternion* value, char* label);

    // ===== FILE/RESOURCE OPERATIONS =====
    void InitializeForWriting();
    void CreateGFFFile(CResStruct* structPtr, CExoString* fileType, CExoString* version);
    void WriteGFFData(CExoFile* file, DWORD* totalBytes);
    void WriteGFFFile(CExoString* name, ResourceType type);
    void Pack(byte alwaysZero1, DWORD alwaysZero2);
    void OnResourceFreed();
    void OnResourceServiced();
    void ReleaseResource();
    DWORD GetTotalSize();

    // ===== OVERRIDE METHODS =====
    void InitializeFunctions() override;
    void InitializeOffsets() override;
};