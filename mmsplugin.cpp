/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * TF2 Dynamic Schema Injector
 * Written by nosoop
 * ======================================================
 */

#include <stdio.h>
#include "mmsplugin.h"
#include "econmanager.h"

#include <filesystem.h>

#include <map>

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

	/* Prepare our manager */
	if (!g_EconManager.Init(error, maxlength)) {
		return false;
	}
	
	return true;
}

bool DynSchema::Unload(char *error, size_t maxlen) {
	SM_UnloadExtension();
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, server, this, &DynSchema::Hook_LevelInitPost, true);
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
	
	// read our own dynamic schema file -- this one supports other sections
	KeyValues::AutoDelete pKVMainConfig("DynamicSchema");
	if (pKVMainConfig->LoadFromFile(filesystem, buffer)) {
		KeyValues *pKVAttributes = pKVMainConfig->FindKey( "attributes" );
		if (pKVAttributes) {
			FOR_EACH_TRUE_SUBKEY(pKVAttributes, kv) {
				InsertOrReplaceAttribute(kv);
			}
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
			
			FOR_EACH_TRUE_SUBKEY(nativeAttribConfig, kv) {
				InsertOrReplaceAttribute(kv);
			}
			
			META_CONPRINTF("Discovered custom schema %s\n", pathbuf);
		}
		filename = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);
	
	// perhaps add some other validations before we actually process our attributes?
	// TODO ensure the name doesn't clash with existing / newly injected attributes
	
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
	return "1.3.0";
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
