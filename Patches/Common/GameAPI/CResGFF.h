/*

GFF (Bioware General File Format) files store general file data
CResGFF is the resource handler for this format in kotor

I want a GameAPI class for its functions and some offset variables

Select Offsets:
===
id, class_name, member_name, offset, notes
34, CResGFF,	header,		 64,	 GFFHeaderInfo *
35, CResGFF,	structs,	 68,	 GFFStructData *
36, CResGFF,	fields,		 76,	 GFFFieldData *
37, CResGFF,	labels,		 84,	 char[16] *
38, CResGFF,	field_type,	 141,	 char[4]
===

Functions:
===
class_name,function_name,address,calling_convention,param_size_bytes,notes
CResGFF,AddDataField,0x00410F50,__thiscall,12,"CResGFF * this, int existingBlockOffset, DWORD sizeOfExistingBlock, DWORD dataLength"
CResGFF,AddDataLayoutField,0x00411020,__thiscall,12,"CResGFF * this, int existingBlockOffset, DWORD sizeOfExistingBlock, DWORD dataLength"
CResGFF,AddDataLayoutList,0x004110F0,__thiscall,12,"CResGFF * this, int existingBlockOffset, DWORD sizeOfExistingBlock, DWORD dataLength"
CResGFF,AddField,0x00411730,__thiscall,12,"CResGFF * this, CResStruct * struct, char * labelText, GFFFieldTypes fieldType"
CResGFF,AddLabel,0x00410E20,__thiscall,4,"CResGFF * this, char * text"
CResGFF,AddList,0x00412450,__thiscall,12,"CResGFF * this, CResList * list, CResStruct * struct, char * labelText"
CResGFF,AddListElement,0x004124E0,__thiscall,12,"CResGFF * this, CResStruct * struct, CResList * list, DWORD structId"
CResGFF,AddStruct,0x00410D60,__thiscall,4,"CResGFF * this, DWORD id"
CResGFF,AddStructToStruct,0x004125B0,__thiscall,16,"CResGFF * this, CResStruct * parentStruct, CResStruct * childStruct, char * labelText, DWORD structId"
CResGFF,Constructor,0x004105A0,__thiscall,0,CResGFF * this
CResGFF,Constructor_2,0x00410630,__thiscall,12,"CResGFF * this, ResourceType resourceType, char * GFFtype, CResRef * templateResRef"
CResGFF,CreateGFFFile,0x00411260,__thiscall,12,"CResGFF * this, CResStruct * struct, CExoString * fileType, CExoString * version"
CResGFF,Destructor,0x00411570,__thiscall,0,CResGFF * this
CResGFF,Destructor_2,0x00413010,__thiscall,1,"CResGFF * this, byte shouldFree"
CResGFF,Destructor_3,0x00506F10,__thiscall,0,CResGFF * this
CResGFF,GetDataField,0x00410A20,__thiscall,8,"CResGFF * this, CResGFFField * field, DWORD * dataSize"
CResGFF,GetDataLayoutList,0x00410A60,__thiscall,8,"CResGFF * this, CResGFFField * param_1, DWORD * dataLength"
CResGFF,GetElementType,0x004111C0,__thiscall,4,"CResGFF * this, CResStruct * struct"
CResGFF,GetField,0x00410930,__thiscall,8,"CResGFF * this, CResGFFStruct * structData, DWORD index"
CResGFF,GetFieldByLabel,0x00411630,__thiscall,8,"CResGFF * this, CResStruct * struct, char * label"
CResGFF,GetFieldCount,0x00411200,__thiscall,4,"CResGFF * this, CResStruct * struct"
CResGFF,GetFieldType,0x00411880,__thiscall,12,"CResGFF * this, CResStruct * struct, char * label, DWORD fieldIndex"
CResGFF,GetField_2,0x00410990,__thiscall,8,"CResGFF * this, CResStruct * struct, DWORD fieldIndex"
CResGFF,GetList,0x004118C0,__thiscall,12,"CResGFF * this, CResList * outList, CResStruct * struct, char * label"
CResGFF,GetListCount,0x00411940,__thiscall,4,"CResGFF * this, CResList * list"
CResGFF,GetListElement,0x00411990,__thiscall,12,"CResGFF * this, CResStruct * struct, CResList * list, DWORD index"
CResGFF,GetStructFromStruct,0x00411A10,__thiscall,12,"CResGFF * this, CResStruct * outStruct, CResStruct * inStruct, char * label"
CResGFF,GetTopLevelStruct,0x00411240,__thiscall,4,"CResGFF * this, CResStruct * outStruct"
CResGFF,GetTotalSize,0x00411540,__thiscall,0,CResGFF * this
CResGFF,InitializeForWriting,0x00410AA0,__thiscall,0,CResGFF * this
CResGFF,OnResourceFreed,0x00410880,__thiscall,0,CResGFF * this
CResGFF,OnResourceServiced,0x00410740,__thiscall,0,CResGFF * this
CResGFF,Pack,0x00412DB0,__thiscall,8,"CResGFF * this, byte alwaysZero1, DWORD alwaysZero2"
CResGFF,ReadFieldBYTE,0x00411A60,__thiscall,13,"CResGFF * this, CResStruct * struct, char * label, int * success, byte default"
CResGFF,ReadFieldCExoLocString,0x00411FD0,__thiscall,20,"CResGFF * this, CExoLocString * out, CResStruct * struct, char * label, int * success, CExoLocString * default"
CResGFF,ReadFieldCExoString,0x00411EC0,__thiscall,20,"CResGFF * this, CExoString * out, CResStruct * struct, char * label, int * success, CExoString * default"
CResGFF,ReadFieldCHAR,0x00411AD0,__thiscall,13,"CResGFF * this, CResStruct * struct, char * label, int * success, char default"
CResGFF,ReadFieldCResRef,0x00411E10,__thiscall,20,"CResGFF * this, CResRef * out, CResStruct * struct, char * label, int * success, CResRef * default"
CResGFF,ReadFieldDWORD,0x00411C20,__thiscall,16,"CResGFF * this, CResStruct * struct, char * label, int * success, DWORD default"
CResGFF,ReadFieldDWORD64,0x00411D70,__thiscall,20,"CResGFF * this, CResStruct * struct, char * label, int * success, DWORD64 default"
CResGFF,ReadFieldFLOAT,0x00411D00,__thiscall,16,"CResGFF * this, CResStruct * struct, char * label, int * success, float default"
CResGFF,ReadFieldINT,0x00411C90,__thiscall,16,"CResGFF * this, CResStruct * struct, char * label, int * success, int default"
CResGFF,ReadFieldQuaternion,0x004121B0,__thiscall,20,"CResGFF * this, Quaternion * out, CResStruct * struct, char * label, int * success, Quaternion * default"
CResGFF,ReadFieldSHORT,0x00411BB0,__thiscall,14,"undefined4 this, undefined4 struct, undefined4 label, int * success, undefined2 default"
CResGFF,ReadFieldVOID,0x00412380,__thiscall,24,"CResGFF * this, CResStruct * struct, void * buffer, DWORD size, char * label, int * success, void * default"
CResGFF,ReadFieldVector,0x004122A0,__thiscall,20,"CResGFF * this, Vector * out, CResStruct * struct, char * label, int * success, Vector * default"
CResGFF,ReadFieldWORD,0x00411B40,__thiscall,14,"CResGFF * this, CResStruct * struct, char * label, int * success, ushort default"
CResGFF,ReleaseResource,0x004108C0,__thiscall,0,CResGFF * this
CResGFF,WriteFieldBYTE,0x00412620,__thiscall,12,"CResGFF * this, CResStruct * struct, byte value, char * label"
CResGFF,WriteFieldCExoLocString,0x00412A10,__thiscall,12,"CResGFF * this, CResStruct * struct, CExoLocString * value, char * label"
CResGFF,WriteFieldCExoString,0x00412970,__thiscall,12,"CResGFF * this, CResStruct * struct, CExoString * value, char * label"
CResGFF,WriteFieldCHAR,0x00412670,__thiscall,12,"CResGFF * this, CResStruct * struct, char value, char * label"
CResGFF,WriteFieldCResRef,0x004128C0,__thiscall,12,"CResGFF * this, CResStruct * struct, CResRef * value, char * label"
CResGFF,WriteFieldDWORD,0x00412760,__thiscall,12,"CResGFF * this, CResStruct * struct, DWORD value, char * label"
CResGFF,WriteFieldDWORD64,0x00412800,__thiscall,16,"CResGFF * this, CResStruct * struct, DWORD64 value, char * label"
CResGFF,WriteFieldFLOAT,0x00412870,__thiscall,12,"CResGFF * this, CResStruct * struct, float value, char * label"
CResGFF,WriteFieldINT,0x004127B0,__thiscall,12,"CResGFF * this, CResStruct * struct, int value, char * label"
CResGFF,WriteFieldQuaternion,0x00412CA0,__thiscall,12,"CResGFF * this, CResStruct * struct, Quaternion * value, char * label"
CResGFF,WriteFieldSHORT,0x00412710,__thiscall,12,"CResGFF * this, CResStruct * struct, short value, char * label"
CResGFF,WriteFieldVOID,0x00412C10,__thiscall,16,"CResGFF * this, CResStruct * struct, void * data, DWORD size, char * label"
CResGFF,WriteFieldVector,0x00412D30,__thiscall,12,"CResGFF * this, CResStruct * struct, Vector * value, char * label"
CResGFF,WriteFieldWORD,0x004126C0,__thiscall,12,"CResGFF * this, CResStruct * struct, ushort value, char * label"
CResGFF,WriteGFFData,0x004113D0,__thiscall,8,"CResGFF * this, CExoFile * file, DWORD * totalBytes"
CResGFF,WriteGFFFile,0x00413030,__thiscall,6,"CResGFF * this, CExoString * name, ResourceType type"
===

Size in bytes of CResGFF in game: 0xa0

Will inherit CRes. This means that all of CRes's functions and offsets apply here

See below where I've already defined some important information.

There should be getters for the various offset data. The notes above have types for these offset variables. (Notice labels is an array of char arrays, that is, an array where each element is a char[16])

These functions should be implemented with the standard wrapping pattern. Some notes:
- The type Vector can be found in Common.h
- Same for Quaternion
- Same for ResourceType
- Just treat CExoLocString * as a void * for now, leave a note to indicate that we need to create CExoLocString as a GameAPI class in the future
- The various AddData*, AddList*, AddStructToStruct, GetData* functions return an int (true/false <-> 0/1)
- AddField returns CResGFFField *
- AddLabel returns DWORD label count
- AddStruct returns DWORD structindex
- The Read* functions return either the type being read (i.e. ReadWord returns a ushort) or a pointer to that datatype (i.e.ReadFieldCExoString returns CExoString *)
- Write* functions return an int 0 or 1 for success checks
- There are other specifics for other functions, but I'll be going through and validating all the return types later, so just use your best judgment for now

*/


#pragma once
#include <windows.h>
#include "../Common.h"
#include "GameVersion.h"
#include "CRes.h"
#include "CExoString.h"

typedef enum GFFFieldTypes {
    BYTE = 0,
    CHAR = 1,
    WORD = 2,
    SHORT = 3,
    DWORD = 4,
    DWORD64 = 5,
    INT = 6,
    INT64 = 7,
    FLOAT = 8,
    DOUBLE = 9,
    CExoString = 10,
    ResRef = 11,
    CExoLocString = 12,
    VOID = 13,
    Struct = 14,
    List = 15,
    Orientation = 16,
    Vector = 17,
    StrRef = 18
} GFFFieldTypes;

struct GFFStructData {
    DWORD id;
    undefined4 data_or_data_offset;
    DWORD field_count;
};

struct GFFFieldData {
    enum GFFFieldTypes field_type;
    DWORD label_index;
    undefined4 data_or_data_offset;
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
    struct CResStruct struct;
    char label[16];
};

// These will later be replaced by an actual class
// But for now we'll just use a struct
// TODO: Convert CExoFile into a GameAPI class
struct CExoFile {
    struct CExoFileInternal* internal;
};

struct CExoFileInternal {
    FILE* fp;
    struct CExoString file_name;
    struct CExoString mode;
};