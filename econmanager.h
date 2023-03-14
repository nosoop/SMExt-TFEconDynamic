#ifndef _INCLUDE_DYNATTRIB_ECON_MANAGER_H_
#define _INCLUDE_DYNATTRIB_ECON_MANAGER_H_

#include "mmsplugin.h"

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
};

// handles 
class CEconManager {
	public:
	bool Init(char *error, size_t maxlength);
	bool InsertOrReplaceAttribute(KeyValues *pAttribKV);
};

// binary refers to 0x58 when iterating over the attribute map, so we'll refer to that value
// we could also do a runtime assertion
static_assert(sizeof(CEconItemAttributeDefinition) + 0x14 == 0x58, "CEconItemAttributeDefinition size mismatch");

extern CEconManager g_EconManager;

#endif // _INCLUDE_DYNATTRIB_ECON_MANAGER_H_
