/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * TF2 Dynamic Schema Injector
 * Written by nosoop
 * ======================================================
 */

#include <stdio.h>
#include "mmsplugin.h"

SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);

DynSchema g_Plugin;
IServerGameDLL *server = NULL;

PLUGIN_EXPOSE(DynSchema, g_Plugin);

bool DynSchema::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	/* Make sure we build on MM:S 1.4 */
#if defined METAMOD_PLAPI_VERSION
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
#else
	GET_V_IFACE_ANY(serverFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
#endif

	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, server, this, &DynSchema::Hook_LevelInitPost, true);

	return true;
}

bool DynSchema::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, server, this, &DynSchema::Hook_LevelInitPost, true);

	return true;
}

bool DynSchema::Hook_LevelInitPost(const char *pMapName, char const *pMapEntities,
		char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background) {
	// this hook should fire shortly after the schema is (re)initialized
	
	// TODO determine if the schema was updated, we can do this by:
	// - adding a sentinel attribute that we test the existence of later, or
	// - check in LevelInitPre if we have a non-null CEconItemSchema::m_pDelayedSchemaData
	
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
	return "1.0.0.0";
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
	return "Injects user-defined attributes into the game schema";
}

const char *DynSchema::GetName() {
	return "TF2 Dynamic Schema";
}

const char *DynSchema::GetURL() {
	return "https://git.csrd.science/";
}
