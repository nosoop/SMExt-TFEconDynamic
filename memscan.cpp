#include "memscan.h"

uintptr_t FindPattern(uintptr_t start, uintptr_t end, const char* pattern, size_t length) {
	uintptr_t ptr = start;
	while (ptr < end - length) {
		bool f = true;
		const char* cur = reinterpret_cast<char*>(ptr);
		for (register size_t i = 0; i < length; i++) {
			if (pattern[i] != '\x2A' && pattern[i] != cur[i]) {
				f = false;
				break;
			}
		}
		if (f) {
			return ptr;
		}
		ptr++;
	}
	return 0;
}

#if WINDOWS
CWinLibInfo::CWinLibInfo(void* pInBase) {
	MEMORY_BASIC_INFORMATION info;
	if (!VirtualQuery(pInBase, &info, sizeof(MEMORY_BASIC_INFORMATION))) {
		this->m_bValid = false;
		return;
	}
	uintptr_t base = reinterpret_cast<uintptr_t>(info.AllocationBase);
	IMAGE_DOS_HEADER *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(base);
	IMAGE_NT_HEADERS *pe = reinterpret_cast<IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
	IMAGE_OPTIONAL_HEADER *opt = &pe->OptionalHeader;
	size_t size = opt->SizeOfImage;
	
	this->m_StartRange = reinterpret_cast<uintptr_t>(info.AllocationBase);
	this->m_EndRange = this->m_StartRange + size;
	this->m_bValid = true;
}

bool CWinLibInfo::LocateSymbol(const char* name, void** result) {
	return false;
}

bool CWinLibInfo::LocatePattern(const char* bytes, size_t length, void** result) {
	if (!this->m_bValid) {
		return false;
	}
	
	uintptr_t pResult = FindPattern(this->m_StartRange, this->m_EndRange, bytes, length);
	if (pResult) {
		*result = reinterpret_cast<void*>(pResult);
		return true;
	}
	return false;
}
#elif _LINUX
CLinuxLibInfo::CLinuxLibInfo(void* pInBase) {
	if (!dladdr(pInBase, &this->m_info)) {
		this->m_bValid = false;
		return;
	}
	
	elf_version(EV_CURRENT);
	
	this->m_fd = open(m_info.dli_fname, O_RDONLY);
	this->m_Elf = elf_begin(this->m_fd, ELF_C_READ, NULL);
	this->m_bValid = true;
}

bool CLinuxLibInfo::LocatePattern(const char* bytes, size_t length, void** result) {
	return false;
}

bool CLinuxLibInfo::LocateSymbol(const char* name, void** result) {
	if (!this->m_bValid) {
		return false;
	}
	
	Elf_Scn *scn = NULL;
	GElf_Shdr shdr;
	while ((scn = elf_nextscn(this->m_Elf, scn)) != NULL) {
		gelf_getshdr(scn, &shdr);
		if (shdr.sh_type == SHT_SYMTAB) {
			break;
		}
	}

	Elf_Data *data = elf_getdata(scn, NULL);
	size_t count = shdr.sh_size / shdr.sh_entsize;

	// iterate symbols
	for (size_t i = 0; i < count; ++i) {
		GElf_Sym sym;
		gelf_getsym(data, i, &sym);
		
		const char *symname = elf_strptr(this->m_Elf, shdr.sh_link, sym.st_name);
		if (!strcmp(symname, name)) {
			*result = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_info.dli_fbase) + sym.st_value);
			return true;
		}
	}
	return false;
}

CLinuxLibInfo::~CLinuxLibInfo() {
	elf_end(this->m_Elf);
	close(this->m_fd);
}
#endif
