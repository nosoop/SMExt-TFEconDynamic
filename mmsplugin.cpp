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

#include "memscan.h"

void BindToSourceMod();
bool SM_LoadExtension(char *error, size_t maxlength);
void SM_UnloadExtension();

SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);

DynSchema g_Plugin;
IServerGameDLL *server = nullptr;
IVEngineServer *engine = NULL;
IFileSystem *filesystem = nullptr;

PLUGIN_EXPOSE(DynSchema, g_Plugin);

class ISchemaAttributeType;

// this may need to be updated in the future
class CEconItemAttributeDefinition
{
public:
	// TODO implementing ~CEconItemAttributeDefinition segfaults. not sure what's up.
	// ideally we implement it to match the game so InsertOrReplace is sure to work correctly
	
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

typedef uintptr_t (*GetEconItemSchema_fn)(void);
GetEconItemSchema_fn fnGetEconItemSchema = nullptr;

// https://www.unknowncheats.me/wiki/Calling_Functions_From_Injected_Library_Using_Function_Pointers_in_C%2B%2B
#ifdef WIN32
typedef bool (__thiscall *CEconItemAttributeInitFromKV_fn)(CEconItemAttributeDefinition* pThis, KeyValues* pAttributeKeys, CUtlVector<CUtlString>* pErrors);
#elif defined(_LINUX)
typedef bool (__cdecl *CEconItemAttributeInitFromKV_fn)(CEconItemAttributeDefinition* pThis, KeyValues* pAttributeKeys, CUtlVector<CUtlString>* pErrors);
#endif
CEconItemAttributeInitFromKV_fn fnItemAttributeInitFromKV = nullptr;

const char* NATIVE_ATTRIB_DIR = "addons/sourcemod/configs/tf2nativeattribs";

bool DynSchema::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, server, this, &DynSchema::Hook_LevelInitPost, true);
	return true;
}

bool DynSchema::OnExtensionLoad(IExtension *me, IShareSys *sys, char *error, size_t maxlength, bool late) {
	sharesys = sys;
	myself = me;

	/* Get the default interfaces from our configured SDK header */
	if (!SM_AcquireInterfaces(error, maxlength)) {
		return false;
	}
	
	// get the base address of the server
	{
#if _WINDOWS
	fnGetEconItemSchema = reinterpret_cast<GetEconItemSchema_fn>(sm_memutils->FindPattern(server, "\xE8\x2A\x2A\x2A\x2A\x83\xC0\x04\xC3", 9));
	fnItemAttributeInitFromKV = reinterpret_cast<CEconItemAttributeInitFromKV_fn>(sm_memutils->FindPattern(server, "\x55\x8B\xEC\x53\x8B\x5D\x08\x56\x8B\xF1\x8B\xCB\x57\xE8\x2A\x2A\x2A\x2A", 18));
#elif _LINUX
		Dl_info info;
		if (dladdr(server, &info) == 0) {
			snprintf(error, maxlength, "dladdr failed");
			return 0;
		}
		void *handle = dlopen(info.dli_fname, RTLD_NOW);
		if (!handle) {
			snprintf(error, maxlength, "Failed to dlopen server.");
			return 0;
		}
		
		fnGetEconItemSchema = reinterpret_cast<GetEconItemSchema_fn>(sm_memutils->ResolveSymbol(handle, "_Z15GEconItemSchemav"));
		
		fnItemAttributeInitFromKV = reinterpret_cast<CEconItemAttributeInitFromKV_fn>(sm_memutils->ResolveSymbol(handle, "_ZN28CEconItemAttributeDefinition11BInitFromKVEP9KeyValuesP10CUtlVectorI10CUtlString10CUtlMemoryIS3_iEE"));
		
		dlclose(handle);
#endif
	}
	
	if (fnGetEconItemSchema == nullptr) {
		snprintf(error, maxlength, "Failed to setup call to GetEconItemSchema()");
		return false;
	} else if (fnItemAttributeInitFromKV == nullptr) {
		snprintf(error, maxlength, "Failed to setup call to CEconItemAttributeDefinition::BInitFromKV");
		return false;
	}
	
	// is this late enough in the MM:S load stage?  we might just have to hold the function
	g_SchemaAttributes = reinterpret_cast<AttributeMap*>(fnGetEconItemSchema() + 0x1BC);

	return true;
}

bool DynSchema::Unload(char *error, size_t maxlen) {
	SM_UnloadExtension();
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, server, this, &DynSchema::Hook_LevelInitPost, true);
	return true;
}

/**
 * Initializes a CEconItemAttributeDefinition from a KeyValues definition, then inserts or
 * replaces the appropriate entry in the schema.
 */
bool InsertOrReplaceAttribute(KeyValues *pAttribKV) {
	/**
	 * TODO implement special handling when "auto" is provided; use an autoincrementing value
	 * that checks for a free slot in the attribute mapping, then internally map the attribute
	 * name to that value for persistence and so we don't add that attribute multiple times
	 */
	int attrdef = atoi(pAttribKV->GetName());
	if (attrdef <= 0) {
		return false;
	}
	
	// only replace existing injected attributes; fail on schema attributes
	auto existingIndex = g_SchemaAttributes->Find(attrdef);
	if (existingIndex != g_SchemaAttributes->InvalidIndex()) {
		auto &existingAttr = g_SchemaAttributes->Element(existingIndex);
		if (!existingAttr.m_KeyValues->GetBool("injected")) {
			META_CONPRINTF("WARN: Not overriding native attribute '%s'\n",
					existingAttr.m_pszName);
			return false;
		}
	}
	
	// embed additional custom data into attribute KV; econdata and the like can deal with this
	// one could also add this data into the file itself, but this leaves less room for error
	pAttribKV->SetBool("injected", true);
	
	CEconItemAttributeDefinition def;
	fnItemAttributeInitFromKV(&def, pAttribKV, nullptr);
	
	// TODO verify that this doesn't leak, or just shrug it off
	g_SchemaAttributes->InsertOrReplace(attrdef, def);
	return true;
}

bool DynSchema::Hook_LevelInitPost(const char *pMapName, char const *pMapEntities,
		char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background) {
	// this hook should fire shortly after the schema is (re)initialized
	char game_path[256];
	engine->GetGameDir(game_path, sizeof(game_path));
	
	char buffer[1024];
	g_SMAPI->PathFormat(buffer, sizeof(buffer), "%s/%s",
			game_path, "addons/dynattrs/items_dynamic.txt");
	
	// always initialize attributes -- it's better than losing attributes on schema reinit
	// coughhiddendevattributescough
	
	// collect raw attribute keyvalue entries scattered across files
	KeyValues::AutoDelete rawAttributes("attributes");
	
	// read our own dynamic schema file -- this one supports other sections
	KeyValues::AutoDelete pKVMainConfig("DynamicSchema");
	if (pKVMainConfig->LoadFromFile(filesystem, buffer)) {
		KeyValues *pKVAttributes = pKVMainConfig->FindKey( "attributes" );
		if (pKVAttributes) {
			rawAttributes->RecursiveMergeKeyValues(pKVAttributes);
			META_CONPRINTF("Successfully injected custom schema %s\n", buffer);
		} else {
			META_CONPRINTF("Failed to inject custom schema %s\n", buffer);
		}
	}
	
	// iterate over TF2 Hidden Dev Attributes KV format
	// https://forums.alliedmods.net/showthread.php?t=326853
	g_SMAPI->PathFormat(buffer, sizeof(buffer), "%s/%s/*", game_path, NATIVE_ATTRIB_DIR);
	FileFindHandle_t findHandle;
	const char *filename = filesystem->FindFirst(buffer, &findHandle);
	while (filename) {
		char pathbuf[1024];
		g_SMAPI->PathFormat(pathbuf, sizeof(pathbuf), "%s/%s/%s", game_path,
				NATIVE_ATTRIB_DIR, filename);
		if (!filesystem->FindIsDirectory(findHandle)) {
			KeyValues::AutoDelete nativeAttribConfig("attributes");
			nativeAttribConfig->LoadFromFile(filesystem, pathbuf);
			
			rawAttributes->RecursiveMergeKeyValues(nativeAttribConfig);
			
			META_CONPRINTF("Discovered custom schema %s\n", pathbuf);
		}
		filename = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);
	
	// perhaps add some other validations before we actually process our attributes?
	// TODO ensure the name doesn't clash with existing / newly injected attributes
	
	// finally process our attributes
	FOR_EACH_TRUE_SUBKEY(rawAttributes, kv) {
		InsertOrReplaceAttribute(kv);
	}
	
	return true;
}

void DynSchema::AllPluginsLoaded() {
	/* This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */
	BindToSourceMod();
}

void* DynSchema::OnMetamodQuery(const char* iface, int *ret) {
	if (strcmp(iface, SOURCEMOD_NOTICE_EXTENSIONS) == 0) {
		BindToSourceMod();
	}
	if (ret != NULL) {
		*ret = IFACE_OK;
	}
	return NULL;
}

void BindToSourceMod() {
	char error[256];
	if (!SM_LoadExtension(error, sizeof(error))) {
		char message[512];
		snprintf(message, sizeof(message), "Could not load as a SourceMod extension: %s\n", error);
		engine->LogPrint(message);
	}
}

bool SM_LoadExtension(char *error, size_t maxlength) {
	if ((smexts = (IExtensionManager *)
			g_SMAPI->MetaFactory(SOURCEMOD_INTERFACE_EXTENSIONS, NULL, NULL)) == NULL) {
		if (error && maxlength) {
			snprintf(error, maxlength, SOURCEMOD_INTERFACE_EXTENSIONS " interface not found");
		}
		return false;
	}

	/* This could be more dynamic */
	char path[256];
	g_SMAPI->PathFormat(path, sizeof(path),  "addons/dynattrs/tf2dynschema%s",
#if defined __linux__
		"_mm.so"
#else
		".dll"
#endif	
	);

	if ((myself = smexts->LoadExternal(&g_Plugin, path, "dynschema.ext", error, maxlength))
			== NULL) {
		SM_UnsetInterfaces();
		return false;
	}
	return true;
}

void SM_UnloadExtension() {
	smexts->UnloadExtension(myself);
}

bool DynSchema::Pause(char *error, size_t maxlen) {
	return true;
}

bool DynSchema::Unpause(char *error, size_t maxlen) {
	return true;
}

const char *DynSchema::GetLicense() {
	return "GPLv3+";
}

const char *DynSchema::GetVersion() {
	return "1.2.1";
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

void DynSchema::OnExtensionUnload() {
	SM_UnsetInterfaces();
}

void DynSchema::OnExtensionsAllLoaded() {
	// no-op
}

void DynSchema::OnExtensionPauseChange(bool pause) {
	// no-op
}

bool DynSchema::QueryRunning(char *error, size_t maxlength) {
	return true;
}

bool DynSchema::IsMetamodExtension() {
	return true;
}

const char *DynSchema::GetExtensionName() {
	return this->GetName();
}

const char *DynSchema::GetExtensionURL() {
	return this->GetURL();
}

const char *DynSchema::GetExtensionTag() {
	return this->GetLogTag();
}

const char *DynSchema::GetExtensionAuthor() {
	return this->GetAuthor();
}

const char *DynSchema::GetExtensionVerString() {
	return this->GetVersion();
}

const char *DynSchema::GetExtensionDescription() {
	return this->GetDescription();
}

const char *DynSchema::GetExtensionDateString() {
	return this->GetDate();
}
