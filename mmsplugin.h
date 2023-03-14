/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod:Source Stub Plugin
 * Written by AlliedModders LLC.
 * ======================================================
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from 
 * the use of this software.
 *
 * This stub plugin is public domain.
 */

#ifndef _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
#define _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_

#include <ISmmPlugin.h>
#include <smsdk_config.h>

#if defined WIN32 && !defined snprintf
#define snprintf _snprintf
#endif

using SourceMod::IExtension;
using SourceMod::IShareSys;
using SourceMod::IExtensionManager;

class DynSchema : public ISmmPlugin, public SourceMod::IExtensionInterface, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	bool Pause(char *error, size_t maxlen);
	bool Unpause(char *error, size_t maxlen);
	void AllPluginsLoaded();
	
	bool Hook_LevelInitPost(const char *pMapName, char const *pMapEntities, char const *pOldLevel,
			char const *pLandmarkName, bool loadGame, bool background);
	
	void *OnMetamodQuery(const char* iface, int *ret);
public:
	const char *GetAuthor();
	const char *GetName();
	const char *GetDescription();
	const char *GetURL();
	const char *GetLicense();
	const char *GetVersion();
	const char *GetDate();
	const char *GetLogTag();
	
public:
	virtual bool OnExtensionLoad(IExtension *me, IShareSys *sys, char* error, size_t maxlength, bool late);
	virtual void OnExtensionUnload();
	virtual void OnExtensionsAllLoaded();
	virtual void OnExtensionPauseChange(bool pause);
	virtual bool QueryRunning(char *error, size_t maxlength);
	virtual bool IsMetamodExtension();
	virtual const char *GetExtensionName();
	virtual const char *GetExtensionURL();
	virtual const char *GetExtensionTag();
	virtual const char *GetExtensionAuthor();
	virtual const char *GetExtensionVerString();
	virtual const char *GetExtensionDescription();
	virtual const char *GetExtensionDateString();
};

extern DynSchema g_Plugin;

extern SourceMod::IExtensionManager *smexts;

extern SourceMod::IShareSys *sharesys;
extern SourceMod::IExtension *myself;

extern IServerGameDLL *server;

PLUGIN_GLOBALVARS();

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
