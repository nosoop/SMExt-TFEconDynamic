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

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);

DynSchema g_Plugin;
IFileSystem *filesystem = nullptr;

SMEXT_LINK(&g_Plugin);

const char* NATIVE_ATTRIB_DIR = "addons/sourcemod/configs/tf2nativeattribs";

bool DynSchema::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late) {
	GET_V_IFACE_CURRENT(GetFileSystemFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	
	return true;
}

bool DynSchema::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, gamedll, this, &DynSchema::Hook_LevelInitPost, true);

	/* Prepare our manager */
	if (!g_EconManager.Init(error, maxlen)) {
		return false;
	}

	return true;
}

void DynSchema::SDK_OnUnload() {
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, gamedll, this, &DynSchema::Hook_LevelInitPost, true);
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
				g_EconManager.RegisterAttribute(kv);
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
				g_EconManager.RegisterAttribute(kv);
			}
			
			META_CONPRINTF("Discovered custom schema %s\n", pathbuf);
		}
		filename = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);
	
	// perhaps add some other validations before we actually process our attributes?
	// TODO ensure the name doesn't clash with existing / newly injected attributes
	
	g_EconManager.InstallAttributes();
	
	return true;
}
