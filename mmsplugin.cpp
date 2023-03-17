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
#include "natives.h"

#include <map>

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);

DynSchema g_Plugin;

SMEXT_LINK(&g_Plugin);

bool DynSchema::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, gamedll, this, &DynSchema::Hook_LevelInitPost, true);

	sharesys->AddNatives(myself, g_EconAttributeNatives);
	g_EconInjectedAttributeType = g_pHandleSys->CreateType("EconInjectedAttribute", &g_EconInjectedAttributeHandler, 0, NULL, NULL, myself->GetIdentity(), NULL);

	/* Prepare our manager */
	if (!g_EconManager.Init(error, maxlen)) {
		return false;
	}

	return true;
}

void DynSchema::SDK_OnUnload() {
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, gamedll, this, &DynSchema::Hook_LevelInitPost, true);
	
	g_pHandleSys->RemoveType(g_EconInjectedAttributeType, myself->GetIdentity());
}

bool DynSchema::Hook_LevelInitPost(const char *pMapName, char const *pMapEntities,
		char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background) {
	// reinstall attributes as needed
	g_EconManager.InstallAttributes();
	
	return true;
}
