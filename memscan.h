#ifndef _INCLUDE_MEMSCAN_UTIL_H_
#define _INCLUDE_MEMSCAN_UTIL_H_

#include <stdio.h>
#include <cstdint>

#if _WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif _LINUX
#include "mmsplugin.h"
#include <fcntl.h>
#include <gelf.h>
#endif

class ILibInfo {
public:
	virtual ~ILibInfo() {};
	virtual bool LocatePattern(const char* bytes, size_t length, void** result) = 0;
	virtual bool LocateSymbol(const char* name, void** result) = 0;
	
protected:
	bool m_bValid;
};

#if _WINDOWS
class CWinLibInfo : public ILibInfo {
public:
	CWinLibInfo(void* pInBase);
	
	virtual bool LocatePattern(const char* bytes, size_t length, void** result);
	virtual bool LocateSymbol(const char* name, void** result);
	
private:
	uintptr_t m_StartRange, m_EndRange;
};
#elif _LINUX
class CLinuxLibInfo : public ILibInfo {
public:
	CLinuxLibInfo(void* pInBase);
	~CLinuxLibInfo();
	
	virtual bool LocatePattern(const char* bytes, size_t length, void** result);
	virtual bool LocateSymbol(const char* name, void** result);
	
private:
	int m_fd;
	Elf *m_Elf;
	Dl_info m_info;
};
#endif

#endif