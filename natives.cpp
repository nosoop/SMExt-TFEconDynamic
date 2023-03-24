/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * TF2 Dynamic Schema Injector
 * Written by nosoop
 * ======================================================
 */

#include <stdio.h>
#include "mmsplugin.h"
#include "natives.h"
#include "econmanager.h"


HandleType_t g_EconInjectedAttributeType{};
EconInjectedAttributeHandler g_EconInjectedAttributeHandler;

void EconInjectedAttributeHandler::OnHandleDestroy(HandleType_t type, void* object) {
	if (type == g_EconInjectedAttributeType) {
		delete reinterpret_cast<AutoKeyValues*>(object);
	}
}

cell_t sm_EconAttributeCreate(IPluginContext *pContext, const cell_t *params) {
	AutoKeyValues *pContainer = new AutoKeyValues();
	return g_pHandleSys->CreateHandle(g_EconInjectedAttributeType, pContainer,
			pContext->GetIdentity(), myself->GetIdentity(), NULL);
}

// void TF2EconDynAttribute.SetClass(const char[] attrClass)
cell_t sm_EconAttributeSetClass(IPluginContext *pContext, const cell_t *params) {
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HandleError err;
	
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	
	AutoKeyValues *pContainer = nullptr;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_EconInjectedAttributeType, &sec, (void**) &pContainer)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid TF2EconDynAttribute handle %x (error: %d)", hndl, err);
	}
	
	char *value;
	pContext->LocalToString(params[2], &value);
	(*pContainer)->SetString("attribute_class", value);
	
	return 0;
}

// void TF2EconDynAttribute.SetName(const char[] attrName)
cell_t sm_EconAttributeSetName(IPluginContext *pContext, const cell_t *params) {
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HandleError err;
	
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	
	AutoKeyValues *pContainer = nullptr;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_EconInjectedAttributeType, &sec, (void**) &pContainer)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid TF2EconDynAttribute handle %x (error: %d)", hndl, err);
	}
	
	char *value;
	pContext->LocalToString(params[2], &value);
	(*pContainer)->SetString("name", value);
	
	return 0;
}

// void TF2EconDynAttribute.SetDescriptionFormat(const char[] attrDescFmt)
cell_t sm_EconAttributeSetDescriptionFormat(IPluginContext *pContext, const cell_t *params) {
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HandleError err;
	
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	
	AutoKeyValues *pContainer = nullptr;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_EconInjectedAttributeType, &sec, (void**) &pContainer)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid TF2EconDynAttribute handle %x (error: %d)", hndl, err);
	}
	
	char *value;
	pContext->LocalToString(params[2], &value);
	(*pContainer)->SetString("description_format", value);
	
	return 0;
}

// void TF2EconDynAttribute.SetCustom(const char[] key, const char[] value)
cell_t sm_EconAttributeSetCustomKeyValue(IPluginContext *pContext, const cell_t *params) {
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HandleError err;
	
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	
	AutoKeyValues *pContainer = nullptr;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_EconInjectedAttributeType, &sec, (void**) &pContainer)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid TF2EconDynAttribute handle %x (error: %d)", hndl, err);
	}
	
	char *key, *value;
	pContext->LocalToString(params[2], &key);
	pContext->LocalToString(params[3], &value);
	(*pContainer)->SetString(key, value);
	
	return 0;
}

// void TF2EconDynAttribute.SetDefIndex(int attrdef)
cell_t sm_EconAttributeSetDefIndex(IPluginContext *pContext, const cell_t *params) {
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HandleError err;
	
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	
	AutoKeyValues *pContainer = nullptr;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_EconInjectedAttributeType, &sec, (void**) &pContainer)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid TF2EconDynAttribute handle %x (error: %d)", hndl, err);
	}
	
	(*pContainer)->SetName(std::to_string(params[2]).c_str());
	
	return 0;
}

// void TF2EconDynAttribute.ClearDefIndex()
cell_t sm_EconAttributeClearDefIndex(IPluginContext *pContext, const cell_t *params) {
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HandleError err;
	
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	
	AutoKeyValues *pContainer = nullptr;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_EconInjectedAttributeType, &sec, (void**) &pContainer)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid TF2EconDynAttribute handle %x (error: %d)", hndl, err);
	}
	
	(*pContainer)->SetName("auto");
	
	return 0;
}

// bool TF2EconDynAttribute.Register()
cell_t sm_EconAttributeRegister(IPluginContext *pContext, const cell_t *params) {
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HandleError err;
	
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	
	AutoKeyValues *pContainer = nullptr;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_EconInjectedAttributeType, &sec, (void**) &pContainer)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid TF2EconDynAttribute handle %x (error: %d)", hndl, err);
	}
	
	
	if (!g_EconManager.RegisterAttribute(*pContainer) || !g_EconManager.InsertOrReplaceAttribute(*pContainer)) {
		return false;
	}
	return true;
}

// void TF2EconDynAttribute.Clear();
cell_t sm_EconAttributeClear(IPluginContext *pContext, const cell_t *params) {
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HandleError err;
	
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	
	AutoKeyValues *pContainer = nullptr;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_EconInjectedAttributeType, &sec, (void**) &pContainer)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid TF2EconDynAttribute handle %x (error: %d)", hndl, err);
	}
	
	KeyValues::AutoDelete empty("auto");
	pContainer->Assign(empty);
	
	return 0;
}
