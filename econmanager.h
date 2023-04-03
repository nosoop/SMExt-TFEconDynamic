#ifndef _INCLUDE_DYNATTRIB_ECON_MANAGER_H_
#define _INCLUDE_DYNATTRIB_ECON_MANAGER_H_

#include "mmsplugin.h"

#include <map>
#include <memory>

#include <utlmap.h>
#include <utlstring.h>
#include <KeyValues.h>

class ISchemaAttributeType;

// this may need to be updated in the future
class CEconItemAttributeDefinition
{
public:
	// TODO implementing ~CEconItemAttributeDefinition segfaults. not sure what's up.
	// ideally we implement it to match the game so InsertOrReplace is sure to work correctly
	
	/* 0x00 */ KeyValues *m_KeyValues;
	/* 0x04 */ unsigned short m_iIndex;
	/* 0x08 */ ISchemaAttributeType *m_AttributeType;
	/* 0x0c */ bool m_bHidden;
	/* 0x0d */ bool m_bForceOutputDescription;
	/* 0x0e */ bool m_bStoreAsInteger;
	/* 0x0f */ bool m_bInstanceData;
	/* 0x10 */ int m_iAssetClassExportType;
	/* 0x14 */ int m_iAssetClassBucket;
	/* 0x18 */ bool m_bIsSetBonus;
	/* 0x1c */ int m_iIsUserGenerated;
	/* 0x20 */ int m_iEffectType;
	/* 0x24 */ int m_iDescriptionFormat;
	/* 0x28 */ char *m_pszDescriptionString;
	/* 0x2c */ char *m_pszArmoryDesc;
	/* 0x30 */ char *m_pszName;
	/* 0x34 */ char *m_pszAttributeClass;
	/* 0x38 */ bool m_bCanAffectMarketName;
	/* 0x39 */ bool m_bCanAffectRecipeCompName;
	/* 0x3c */ int m_nTagHandle;
	/* 0x40 */ string_t m_iszAttributeClass;
	
	~CEconItemAttributeDefinition() {
		if (m_KeyValues) {
			m_KeyValues->deleteThis();
		}
		m_KeyValues = nullptr;
	}
};

/**
 * Copied implementation of KeyValues::AutoDelete.
 * This implementation creates copies of any KeyValues pointers passed into it.
 */
class AutoKeyValues {
	public:
	AutoKeyValues() : m_pKeyValues{new KeyValues("auto")} {}
	
	AutoKeyValues(KeyValues *pKeyValues) : m_pKeyValues{pKeyValues->MakeCopy()} {}
	AutoKeyValues(const AutoKeyValues &other) : m_pKeyValues{other.m_pKeyValues->MakeCopy()} {}
	
	/**
	 * Copy assignment.  The contents of the KeyValues* needs to be copied; the default
	 * implementation seems to cause a use-after-free.
	 */
	AutoKeyValues& operator =(const AutoKeyValues &other) {
		this->Assign(other);
		return *this;
	}
	
	~AutoKeyValues() {
		if (m_pKeyValues) {
			m_pKeyValues->deleteThis();
		}
	}
	
	void Assign(KeyValues *pKeyValues) {
		if (m_pKeyValues) {
			m_pKeyValues->deleteThis();
		}
		m_pKeyValues = pKeyValues->MakeCopy();
	}
	
	KeyValues *operator->() const { return m_pKeyValues; }
	operator KeyValues *() const { return m_pKeyValues; }
	
	private:
	KeyValues *m_pKeyValues;
};

/**
 * Keeps a copy of existing schema items to inject.
 */
class CEconManager {
	public:
	CEconManager() : m_RegisteredAttributes{} {}
	
	bool Init(char *error, size_t maxlength);
	bool InsertOrReplaceAttribute(KeyValues *pAttribKV);
	bool RegisterAttribute(KeyValues *pAttribKV);
	void InstallAttributes();
	
	private:
	std::map<std::string, AutoKeyValues> m_RegisteredAttributes;
};

// binary refers to 0x58 when iterating over the attribute map, so we'll refer to that value
// we could also do a runtime assertion
static_assert(sizeof(CEconItemAttributeDefinition) + 0x14 == 0x58, "CEconItemAttributeDefinition size mismatch");

extern CEconManager g_EconManager;

#endif // _INCLUDE_DYNATTRIB_ECON_MANAGER_H_
