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

static KHook::Virtual<IServerGameDLL, bool, const char *, const char *, const char *, const char *, bool, bool> g_LevelInitHook(&IServerGameDLL::LevelInit, nullptr, &DynSchema::Hook_LevelInitPost);

DynSchema g_Plugin;

SMEXT_LINK(&g_Plugin);

bool DynSchema::SDK_OnLoad(char *error, size_t maxlen, bool late) {
	g_LevelInitHook.Add(gamedll);

	sharesys->AddNatives(myself, g_EconAttributeNatives);
	g_EconInjectedAttributeType = g_pHandleSys->CreateType("EconDynAttr", &g_EconInjectedAttributeHandler, 0, NULL, NULL, myself->GetIdentity(), NULL);

	/* Prepare our manager */
	if (!g_EconManager.Init(error, maxlen)) {
		return false;
	}

	return true;
}

void DynSchema::SDK_OnUnload() {
	g_LevelInitHook.Remove(gamedll);

	g_pHandleSys->RemoveType(g_EconInjectedAttributeType, myself->GetIdentity());
}

KHook::Return<bool> DynSchema::Hook_LevelInitPost(IServerGameDLL *pThis, const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background) {
	// reinstall attributes as needed
	g_EconManager.InstallAttributes();

	return { KHook::Action::Ignore };
}
