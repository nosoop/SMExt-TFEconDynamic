#include "econmanager.h"

#include <IGameConfigs.h>

#include <map>

CEconManager g_EconManager;

// CEconItemSchema member offsets, resolved from gamedata (tf2.econ_dynamic).
int g_iOffsM_Attributes;
int g_iOffsM_AttributeNameIndex;
int g_iOffsM_AttributeVector;

// pointer to item schema attribute map singleton
using AttributeMap = CUtlMap<int, CEconItemAttributeDefinition, int>;
AttributeMap *g_SchemaAttributes;

// pointer to the schema's name -> attribute map element index lookup table
using AttributeNameMap = CUtlMap<const char *, int, int>;
AttributeNameMap *g_SchemaAttributeNameIndex;

// pointer to the schema's flat def-index -> attribute definition lookup vector
using AttributeVector = CUtlVector<CEconItemAttributeDefinition *>;
AttributeVector *g_SchemaAttributeVector;

size_t g_nAutoAttributeBase = 4000;
std::map<std::string, int> g_AutoNumberedAttributes{};

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
	
	SourceMod::IGameConfig *conf;
	if (!gameconfs->LoadGameConfigFile("tf2.econ_dynamic", &conf, error, maxlength)) {
		return false;
	}
	
	if (!conf->GetMemSig("GEconItemSchema()", (void**) &fnGetEconItemSchema)) {
		snprintf(error, maxlength, "Failed to setup call to GetEconItemSchema()");
		return false;
	} else if (!conf->GetMemSig("CEconItemAttributeDefinition::BInitFromKV()", (void**) &fnItemAttributeInitFromKV)) {
		snprintf(error, maxlength, "Failed to setup call to CEconItemAttributeDefinition::BInitFromKV");
		return false;
	} else if (!conf->GetOffset("CEconItemSchema::m_mapAttributes", &g_iOffsM_Attributes)) {
		snprintf(error, maxlength, "Failed to get offset CEconItemSchema::m_mapAttributes");
		return false;
	} else if (!conf->GetOffset("CEconItemSchema::m_mapAttributeNames", &g_iOffsM_AttributeNameIndex)) {
		snprintf(error, maxlength, "Failed to get offset CEconItemSchema::m_mapAttributeNames");
		return false;
	} else if (!conf->GetOffset("CEconItemSchema::m_vecAttributeDefinitions", &g_iOffsM_AttributeVector)) {
		snprintf(error, maxlength, "Failed to get offset CEconItemSchema::m_vecAttributeDefinitions");
		return false;
	}
	
	gameconfs->CloseGameConfigFile(conf);
	
	// is this late enough in the MM:S load stage?  we might just have to hold the function
	g_SchemaAttributes = reinterpret_cast<AttributeMap*>(fnGetEconItemSchema() + g_iOffsM_Attributes);
	g_SchemaAttributeNameIndex = reinterpret_cast<AttributeNameMap*>(fnGetEconItemSchema() + g_iOffsM_AttributeNameIndex);
	g_SchemaAttributeVector = reinterpret_cast<AttributeVector*>(fnGetEconItemSchema() + g_iOffsM_AttributeVector);
	return true;
}

/**
 * Initializes a CEconItemAttributeDefinition from a KeyValues definition, then inserts or
 * replaces the appropriate entry in the schema.
 */
bool CEconManager::InsertOrReplaceAttribute(KeyValues *pAttribKV) {
	KeyValues::AutoDelete attribute{pAttribKV->MakeCopy()};
	const char* attrID = attribute->GetName();
	const char* attrName = attribute->GetString("name");
	
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
	
	attribute->SetName(std::to_string(attrdef).c_str());
	
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
	attribute->SetBool("injected", true);
	
	CEconItemAttributeDefinition def;
	fnItemAttributeInitFromKV(&def, attribute, nullptr);
	
	// TODO verify that this doesn't leak, or just shrug it off
	auto insertedIndex = g_SchemaAttributes->InsertOrReplace(attrdef, def);
	
	// Since TF2 11087207, name-based schema lookups consult a secondary map that
	// is built alongside m_mapAttributes.  Register the injected attribute there so
	// plugins that resolve attributes by name (e.g. tf2attributes) can find it.
	if (insertedIndex != g_SchemaAttributes->InvalidIndex()) {
		const char *pszDefinitionName = g_SchemaAttributes->Element(insertedIndex).m_pszName;
		if (pszDefinitionName) {
			g_SchemaAttributeNameIndex->InsertOrReplace(pszDefinitionName, insertedIndex);
		}
	}
	
	// GetAttributeDefinition() caches pointers into m_mapAttributes, which we may
	// have just reallocated; rebuild its contents from the map.
	SyncAttributeVector();
	return true;
}

/**
 * Rebuilds the schema's flat def-index -> definition lookup vector from the
 * attribute map.  The vector holds pointers into m_mapAttributes, so it has to be
 * refreshed after any mutation of the map (TF2 11138802+).
 */
void CEconManager::SyncAttributeVector() {
	if (!g_SchemaAttributeVector) {
		return;
	}
	
	int maxIndex = -1;
	FOR_EACH_MAP_FAST((*g_SchemaAttributes), i) {
		const int index = g_SchemaAttributes->Key(i);
		if (index > maxIndex) {
			maxIndex = index;
		}
	}
	if (maxIndex < 0) {
		return;
	}
	
	const int count = maxIndex + 1;
	if (g_SchemaAttributeVector->Count() < count) {
		g_SchemaAttributeVector->SetCount(count);
	}
	
	for (int i = 0; i < g_SchemaAttributeVector->Count(); ++i) {
		(*g_SchemaAttributeVector)[i] = nullptr;
	}
	
	FOR_EACH_MAP_FAST((*g_SchemaAttributes), i) {
		const int index = g_SchemaAttributes->Key(i);
		(*g_SchemaAttributeVector)[index] = &g_SchemaAttributes->Element(i);
	}
}

bool CEconManager::RegisterAttribute(KeyValues* pAttribKV) {
	AutoKeyValues kv{pAttribKV};
	std::string attrName = kv->GetString("name");
	if (attrName.empty()) {
		attrName = kv->GetString("attribute_class");
		if (attrName.empty()) {
			return false;
		}
		kv->SetString("name", attrName.c_str());
	}
	
	this->m_RegisteredAttributes[attrName] = std::move(kv);
	return true;
}

void CEconManager::InstallAttributes() {
	for (const auto& pair : m_RegisteredAttributes) {
		InsertOrReplaceAttribute(pair.second);
	}
}

KeyValues *CEconManager::GetAttributeDefinitionKeyValuesByName(const char *name) {
	if (!name) {
		return nullptr;
	}
	
	FOR_EACH_MAP_FAST( (*g_SchemaAttributes), i ) {
		if (!(*g_SchemaAttributes)[i].m_pszName) {
			continue;
		}
		if (stricmp((*g_SchemaAttributes)[i].m_pszName, name) == 0) {
			return (*g_SchemaAttributes)[i].m_KeyValues;
		}
	}
	return nullptr;
}
