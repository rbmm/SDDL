#pragma once

class __declspec(novtable) VPrint
{
public:
	virtual VPrint& operator ()(PCWSTR format, ...) = 0;
};

void PrintBin(VPrint& log, PULONG pu, ULONG s);

class LSA_LOOKUP
{
	LSA_HANDLE PolicyHandle;
	VPrint& log;
public:

	LSA_LOOKUP(VPrint& log) : log(log), PolicyHandle(0)
	{
	}

	~LSA_LOOKUP()
	{
		if (PolicyHandle)
		{
			LsaClose(PolicyHandle);
		}
	}

	NTSTATUS Init();
	NTSTATUS DumpGroups(PTOKEN_GROUPS ptg);
	NTSTATUS DumpACEList(ULONG AceCount, PVOID FirstAce);
	void DumpSid(PCWSTR Prefix, PSID Sid);
	void DumpAcl(PACL acl, PCWSTR caption);

	void DumpSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor);
	void DumpStringSecurityDescriptor(PCWSTR StringSecurityDescriptor);
};