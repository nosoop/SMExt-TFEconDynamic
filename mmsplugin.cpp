/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * TF2 Dynamic Schema Injector
 * Written by nosoop
 * ======================================================
 */

#include <stdio.h>
#include "mmsplugin.h"

#include <utlmap.h>
#include <utlstring.h>
#include <KeyValues.h>
#include <filesystem.h>

#if WINDOWS
#include <windows.h>
#elif _LINUX
#include <fcntl.h>
#include <gelf.h>
#endif

SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);

DynSchema g_Plugin;
IServerGameDLL *server = nullptr;
IVEngineServer *engine = NULL;
IBaseFileSystem *basefilesystem = nullptr;

PLUGIN_EXPOSE(DynSchema, g_Plugin);

class ISchemaAttributeType;

// this may need to be updated in the future
class CEconItemAttributeDefinition
{
public:
	/* 0x00 */ KeyValues *m_KeyValues;
	/* 0x04 */ unsigned short m_iIndex;
	/* 0x08 */ ISchemaAttributeType *m_AttributeType;
	/* 0x0c */ bool m_bHidden;
	/* 0x0d */ bool m_bForceOutputDescription;
	/* 0x0e */ bool m_bStoreAsInteger;
	/* 0x0f */ bool m_bInstanceData;
	/* 0x10 */ int m_iAssetClassExportType;
	/* 0x14 */ int m_iAssetClassBucket;
	/* 0x18 */ bool m_bIsSetBonus;
	/* 0x1c */ int m_iIsUserGenerated;
	/* 0x20 */ int m_iEffectType;
	/* 0x24 */ int m_iDescriptionFormat;
	/* 0x28 */ char *m_pszDescriptionString;
	/* 0x2c */ char *m_pszArmoryDesc;
	/* 0x30 */ char *m_pszName;
	/* 0x34 */ char *m_pszAttributeClass;
	/* 0x38 */ bool m_bCanAffectMarketName;
	/* 0x39 */ bool m_bCanAffectRecipeCompName;
	/* 0x3c */ int m_nTagHandle;
	/* 0x40 */ string_t m_iszAttributeClass;
};

// binary refers to 0x58 when iterating over the attribute map, so we'll refer to that value
// we could also do a runtime assertion
static_assert(sizeof(CEconItemAttributeDefinition) + 0x14 == 0x58, "CEconItemAttributeDefinition size mismatch");

// pointer to item schema attribute map singleton
using AttributeMap = CUtlMap<int, CEconItemAttributeDefinition, int>;
AttributeMap *g_SchemaAttributes;

using GetEconItemSchemaFn_t = uintptr_t();
GetEconItemSchemaFn_t *fnGetEconItemSchema = nullptr;

// https://www.unknowncheats.me/wiki/Calling_Functions_From_Injected_Library_Using_Function_Pointers_in_C%2B%2B
#ifdef WIN32
typedef bool (__thiscall *CEconItemAttributeInitFromKV_fn)(CEconItemAttributeDefinition* pThis, KeyValues* pAttributeKeys, CUtlVector<CUtlString>* pErrors);
#elif defined(_LINUX)
typedef bool (__cdecl *CEconItemAttributeInitFromKV_fn)(CEconItemAttributeDefinition* pThis, KeyValues* pAttributeKeys, CUtlVector<CUtlString>* pErrors);
#endif
CEconItemAttributeInitFromKV_fn fnItemAttributeInitFromKV = nullptr;

uintptr_t FindPattern(uintptr_t start, uintptr_t end, const char* pattern, size_t length) {
	uintptr_t ptr = start;
	bool f;
	while (ptr < end - length) {
		f = true;
		const char* cur = reinterpret_cast<char*>(ptr);
		for (register size_t i = 0; i < length; i++) {
			if (pattern[i] != '\x2A' && pattern[i] != cur[i]) {
				f = false;
				break;
			}
		}
		if (f) {
			return ptr;
		}
		ptr++;
	}
	return 0;
}

bool DynSchema::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, basefilesystem, IBaseFileSystem, BASEFILESYSTEM_INTERFACE_VERSION);

	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, server, this, &DynSchema::Hook_LevelInitPost, true);

	// get the base address of the server
	// TODO windows support
#if WINDOWS
	MEMORY_BASIC_INFORMATION info;
	if (!VirtualQuery(server, &info, sizeof(MEMORY_BASIC_INFORMATION))) {
		return false;
	}
	uintptr_t base = reinterpret_cast<uintptr_t>(info.AllocationBase);
	IMAGE_DOS_HEADER *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(base);
	IMAGE_NT_HEADERS *pe = reinterpret_cast<IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
	IMAGE_OPTIONAL_HEADER *opt = &pe->OptionalHeader;
	size_t size = opt->SizeOfImage;
	
	fnGetEconItemSchema = (GetEconItemSchemaFn_t*) (FindPattern(base, base + size, "\xE8\x2A\x2A\x2A\x2A\x83\xC0\x04\xC3", 9));
	fnItemAttributeInitFromKV = (CEconItemAttributeInitFromKV_fn) (FindPattern(base, base + size, "\x55\x8B\xEC\x53\x8B\x5D\x08\x56\x8B\xF1\x8B\xCB\x57\xE8\x2A\x2A\x2A\x2A", 18));
#elif _LINUX
	Dl_info info;
	if (!dladdr(server, &info)) {
		return false;
	}
	
	// locate symbols within our server binary
	elf_version(EV_CURRENT);

	int fd = open(info.dli_fname, O_RDONLY);
	Elf *elf = elf_begin(fd, ELF_C_READ, NULL);

	Elf_Scn *scn = NULL;
	GElf_Shdr shdr;
	while ((scn = elf_nextscn(elf, scn)) != NULL) {
		gelf_getshdr(scn, &shdr);
		if (shdr.sh_type == SHT_SYMTAB) {
			break;
		}
	}

	Elf_Data *data = elf_getdata(scn, NULL);
	size_t count = shdr.sh_size / shdr.sh_entsize;

	/* print the symbol names */
	for (size_t i = 0; i < count; ++i) {
		GElf_Sym sym;
		gelf_getsym(data, i, &sym);
		
		const char *symname = elf_strptr(elf, shdr.sh_link, sym.st_name);
		if (!strcmp(symname,
				"_ZN28CEconItemAttributeDefinition11BInitFromKVEP9KeyValuesP10CUtlVectorI10CUtlString10CUtlMemoryIS3_iEE")) {
			fnItemAttributeInitFromKV = (CEconItemAttributeInitFromKV_fn) (reinterpret_cast<uintptr_t>(info.dli_fbase) + sym.st_value);
		} else if (!strcmp(symname, "_Z15GEconItemSchemav")) {
			fnGetEconItemSchema = reinterpret_cast<GetEconItemSchemaFn_t*>(reinterpret_cast<uintptr_t>(info.dli_fbase) + sym.st_value);
		}
	}
	elf_end(elf);
	close(fd);
#endif
	
	if (!fnItemAttributeInitFromKV || !fnGetEconItemSchema) {
		META_CONPRINTF("Failed to get GEIS or BIFKV\n");
		return false;
	}
	
	// is this late enough in the MM:S load stage?  we might just have to hold the function
	g_SchemaAttributes = reinterpret_cast<AttributeMap*>(fnGetEconItemSchema() + 0x1BC);

	return true;
}

bool DynSchema::Unload(char *error, size_t maxlen) {
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, server, this, &DynSchema::Hook_LevelInitPost, true);
	return true;
}

bool AddAttribute(KeyValues *pAttribKV) {
	int attrdef = atoi(pAttribKV->GetName());
	
	// TODO add a copy of these tests in native
	if (attrdef <= 0) {
		return false;
	}
	
	if (g_SchemaAttributes->IsValidIndex(g_SchemaAttributes->Find(attrdef))) {
		return false;
	}
	
	CEconItemAttributeDefinition def;
	fnItemAttributeInitFromKV(&def, pAttribKV, nullptr);
	
	g_SchemaAttributes->Insert(attrdef, def);
	return true;
}

bool DynSchema::Hook_LevelInitPost(const char *pMapName, char const *pMapEntities,
		char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background) {
	// this hook should fire shortly after the schema is (re)initialized
	
	// TODO determine if the schema was updated, we can do this by:
	// - adding a sentinel attribute that we test the existence of later, or
	// - check in LevelInitPre if we have a non-null CEconItemSchema::m_pDelayedSchemaData
	
	// TODO create a map of existing attribute names
	
	char game_path[256];
	engine->GetGameDir(game_path, sizeof(game_path));
	
	char buffer[1024];
	g_SMAPI->PathFormat(buffer, sizeof(buffer), "%s/%s",
			game_path, "addons/dynattrs/items_dynamic.txt");
	
	KeyValues::AutoDelete pItemKV("DynamicSchema");
	if (pItemKV->LoadFromFile(basefilesystem, buffer)) {
		KeyValues *pKVAttributes = pItemKV->FindKey( "attributes" );
		if (pKVAttributes) {
			FOR_EACH_TRUE_SUBKEY(pKVAttributes, pKVAttribute) {
				AddAttribute(pKVAttribute);
			}
		}
		META_CONPRINTF("Successfully injected custom schema %s\n", buffer);
	} else {
		META_CONPRINTF("Failed to inject custom schema %s\n", buffer);
	}
	
	return true;
}

void DynSchema::AllPluginsLoaded() {
	/* This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */
}

bool DynSchema::Pause(char *error, size_t maxlen) {
	return true;
}

bool DynSchema::Unpause(char *error, size_t maxlen) {
	return true;
}

const char *DynSchema::GetLicense() {
	return "Proprietary";
}

const char *DynSchema::GetVersion() {
	return "1.0.1";
}

const char *DynSchema::GetDate() {
	return __DATE__;
}

const char *DynSchema::GetLogTag() {
	return "dynschema";
}

const char *DynSchema::GetAuthor() {
	return "nosoop";
}

const char *DynSchema::GetDescription() {
	return "Injects user-defined content into the game schema";
}

const char *DynSchema::GetName() {
	return "TF2 Dynamic Schema";
}

const char *DynSchema::GetURL() {
	return "https://git.csrd.science/";
}
