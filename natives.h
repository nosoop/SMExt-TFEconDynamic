/**
 * vim: set ts=4 :
 * =============================================================================
 * TF2 Econ Dynamic Extension
 * Copyright (C) 2023 nosoop.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_NATIVES_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_NATIVES_H_
/**
 * @file natives.h
 * @brief Declares native bindings for SourcePawn.
 */
 
class EconInjectedAttributeHandler : public IHandleTypeDispatch {
public:
	void OnHandleDestroy(HandleType_t type, void* object);
};

extern HandleType_t g_EconInjectedAttributeType;
extern EconInjectedAttributeHandler g_EconInjectedAttributeHandler;

cell_t sm_EconAttributeCreate(IPluginContext *pContext, const cell_t *params);
cell_t sm_EconAttributeSetClass(IPluginContext *pContext, const cell_t *params);
cell_t sm_EconAttributeSetName(IPluginContext *pContext, const cell_t *params);
cell_t sm_EconAttributeSetDescriptionFormat(IPluginContext *pContext, const cell_t *params);
cell_t sm_EconAttributeSetCustomKeyValue(IPluginContext *pContext, const cell_t *params);
cell_t sm_EconAttributeSetDefIndex(IPluginContext *pContext, const cell_t *params);
cell_t sm_EconAttributeClearDefIndex(IPluginContext *pContext, const cell_t *params);
cell_t sm_EconAttributeRegister(IPluginContext *pContext, const cell_t *params);
cell_t sm_EconAttributeClear(IPluginContext *pContext, const cell_t *params);

const sp_nativeinfo_t g_EconAttributeNatives[] = {
	{ "EconInjectedAttribute.EconInjectedAttribute", sm_EconAttributeCreate },
	{ "EconInjectedAttribute.SetClass", sm_EconAttributeSetClass },
	{ "EconInjectedAttribute.SetName", sm_EconAttributeSetName },
	{ "EconInjectedAttribute.SetDescriptionFormat", sm_EconAttributeSetDescriptionFormat },
	{ "EconInjectedAttribute.SetCustom", sm_EconAttributeSetCustomKeyValue },
	{ "EconInjectedAttribute.SetDefIndex", sm_EconAttributeSetDefIndex },
	{ "EconInjectedAttribute.ClearDefIndex", sm_EconAttributeClearDefIndex },
	{ "EconInjectedAttribute.Register", sm_EconAttributeRegister },
	{ "EconInjectedAttribute.Clear", sm_EconAttributeClear },
	
	{NULL, NULL},
};

#endif //_INCLUDE_SOURCEMOD_EXTENSION_NATIVES_H_
