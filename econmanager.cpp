#include "econmanager.h"

#include <map>

CEconManager g_EconManager;

// pointer to item schema attribute map singleton
using AttributeMap = CUtlMap<int, CEconItemAttributeDefinition, int>;
AttributeMap *g_SchemaAttributes;

size_t g_nAutoAttributeBase = 4000;
std::map<std::string, int> g_AutoNumberedAttributes;

typedef uintptr_t (*GetEconItemSchema_fn)(void);
GetEconItemSchema_fn fnGetEconItemSchema = nullptr;

// https://www.unknowncheats.me/wiki/Calling_Functions_From_Injected_Library_Using_Function_Pointers_in_C%2B%2B
#ifdef WIN32
typedef bool (__thiscall *CEconItemAttributeInitFromKV_fn)(CEconItemAttributeDefinition* pThis, KeyValues* pAttributeKeys, CUtlVector<CUtlString>* pErrors);
#elif defined(_LINUX)
typedef bool (__cdecl *CEconItemAttributeInitFromKV_fn)(CEconItemAttributeDefinition* pThis, KeyValues* pAttributeKeys, CUtlVector<CUtlString>* pErrors);
#endif
CEconItemAttributeInitFromKV_fn fnItemAttributeInitFromKV = nullptr;

bool CEconManager::Init(char *error, size_t maxlength) {
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

/**
 * Initializes a CEconItemAttributeDefinition from a KeyValues definition, then inserts or
 * replaces the appropriate entry in the schema.
 */
bool InsertOrReplaceAttribute(KeyValues *pAttribKV) {
	const char* attrID = pAttribKV->GetName();
	const char* attrName = pAttribKV->GetString("name");
	
	int attrdef;
	if (strcmp(attrID, "auto") == 0) {
		/**
		 * Have the plugin automatically allocate an attribute ID.
		 * - if the name is already mapped to an ID, then use that
		 * - otherwise, continue to increment our counter until we find an unused one
		 */
		auto search = g_AutoNumberedAttributes.find(attrName);
		if (search != g_AutoNumberedAttributes.end()) {
			attrdef = search->second;
		} else {
			while (g_SchemaAttributes->Find(g_nAutoAttributeBase) != g_SchemaAttributes->InvalidIndex()) {
				g_nAutoAttributeBase++;
			}
			attrdef = g_nAutoAttributeBase;
			g_AutoNumberedAttributes[attrName] = attrdef;
		}
	} else {
		attrdef = atoi(attrID);
		if (attrdef <= 0) {
			META_CONPRINTF("Attribute '%s' has invalid index string '%s'\n", attrName, attrID);
			return false;
		}
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
