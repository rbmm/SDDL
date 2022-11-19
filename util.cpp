#include "stdafx.h"

_NT_BEGIN

struct __declspec(novtable) VTask 
{
	virtual void OnName(ULONG Ordinal, PCSTR Name = 0) = 0;
	virtual void OnDataName(ULONG /*Ordinal*/, PCSTR /*Name*/ = 0)
	{

	}

	void DoTask(PVOID ImageBase, PIMAGE_EXPORT_DIRECTORY pied)
	{
		if (DWORD NumberOfFunctions = pied->NumberOfFunctions)
		{
			ULONG OrdinalBase = pied->Base, NumberOfFunctions2 = NumberOfFunctions, cb;

			if (PLONG bits = (PLONG)_malloca(cb = (NumberOfFunctions + 7) >> 3))
			{
				PULONG AddressOfFunctions = (PULONG)RtlOffsetToPointer(ImageBase, pied->AddressOfFunctions);
				union {
					ULONG rva;
					PIMAGE_SECTION_HEADER pish;
				};

				if (DWORD NumberOfNames = pied->NumberOfNames)
				{
					RtlZeroMemory(bits, cb);

					PIMAGE_NT_HEADERS pinth = RtlImageNtHeader(ImageBase);

					PULONG AddressOfNames = (PULONG)RtlOffsetToPointer(ImageBase, pied->AddressOfNames);
					PUSHORT AddressOfNameOrdinals = (PUSHORT)RtlOffsetToPointer(ImageBase, pied->AddressOfNameOrdinals);

					do 
					{
						USHORT Ordinal = *AddressOfNameOrdinals++;

						if (Ordinal >= NumberOfFunctions)
						{
							__debugbreak();
						}

						if (!_bittestandset(bits, Ordinal))
						{
							NumberOfFunctions2--;
						}
						
						PCSTR name = RtlOffsetToPointer(ImageBase, *AddressOfNames++);

						if (rva = AddressOfFunctions[Ordinal])
						{
							if (pish = RtlImageRvaToSection(pinth, ImageBase, rva))
							{
								if (pish->Characteristics & IMAGE_SCN_CNT_CODE)
								{
									OnName(Ordinal + OrdinalBase, name);
								}
								else
								{
									OnDataName(Ordinal + OrdinalBase, name);
								}
							}
						}

					} while (--NumberOfNames);
				}

				if (NumberOfFunctions2)
				{
					do 
					{
						if (!_bittestandset(bits, --NumberOfFunctions))
						{
							if (AddressOfFunctions[NumberOfFunctions])
							{
								OnName(OrdinalBase + NumberOfFunctions);
							}
						}
					} while (NumberOfFunctions);
				}
				_freea(bits);
			}
		}
	}
};

struct DefTask : VTask 
{
	virtual void OnDataName(ULONG Ordinal, PCSTR Name = 0)
	{
		if (Name)
		{
			DbgPrint("%s=_pokertablewidget.dll.%s PRIVATE\n", Name, Name);
		}
		else
		{
			DbgPrint("#%u=_pokertablewidget.dll.#%u @%u NONAME PRIVATE\n", Ordinal, Ordinal, Ordinal);
		}
	}

	virtual void OnName(ULONG Ordinal, PCSTR Name = 0)
	{
		if (Name)
		{
			DbgPrint("%s @%u PRIVATE\n", Name, Ordinal);
		}
		else
		{
			DbgPrint("#%u @%u NONAME PRIVATE\n", Ordinal, Ordinal);
		}
	}
};

struct CodeTask : VTask 
{
	virtual void OnName(ULONG Ordinal, PCSTR Name = 0)
	{
		if (Name)
		{
			DbgPrint("EXPORT %s\n", Name);
		}
		else
		{
			DbgPrint("EXPORT_ %u\n", Ordinal);
		}
	}
};

struct ConstTask : VTask 
{
	virtual void OnName(ULONG /*Ordinal*/, PCSTR Name = 0)
	{
		if (Name)
		{
			DbgPrint("__name_%s DB \"%s\",0\n", Name, Name);
		}
	}
};

struct BbsTask : VTask 
{
	virtual void OnName(ULONG Ordinal, PCSTR Name = 0)
	{
		if (Name)
		{
			DbgPrint("__imp__%s DQ ?\n", Name);
		}
		else
		{
			DbgPrint("__imp__$%u DQ ?\n", Ordinal);
		}
	}
};

void TYER(PCWSTR pszLib)
{
	if (HMODULE hmod = LoadLibraryExW(pszLib, 0, LOAD_LIBRARY_AS_IMAGE_RESOURCE))
	{
		PVOID ImageBase = PAGE_ALIGN(hmod);
		ULONG Size;
		if (PIMAGE_EXPORT_DIRECTORY pied = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
			ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &Size))
		{
			if (pied->NumberOfFunctions)
			{
				DbgPrint("EXPORTS\n");
				DefTask def;
				CodeTask code;
				ConstTask cnst;
				BbsTask bbs;

				def.DoTask(ImageBase, pied);
				code.DoTask(ImageBase, pied);
				cnst.DoTask(ImageBase, pied);
				bbs.DoTask(ImageBase, pied);
			}
		}
		FreeLibrary(hmod);
	}
}

_NT_END