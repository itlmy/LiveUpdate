#include "AntiMultiOpen.h"
#include "Windows.h"
#include "tchar.h"

#define MUTEX_UNIQUE "mutex_56dad5a1cd758471"

#pragma data_seg("Shared")
int volatile isExist = 0;
#pragma data_seg() 
#pragma comment(linker, "/Section:Shared,RWS")

CAntiMultiOpen::CAntiMultiOpen()
{
}


CAntiMultiOpen::~CAntiMultiOpen()
{
}

bool CAntiMultiOpen::checkMultiOpen()
{
	if (!checkMutex())
	{
		return false;
	}

	if (!checkDataSeg())
	{
		return false;
	}

	return true;
}


// IsInsideVPC's exception filter  
DWORD __forceinline IsInsideVPC_exceptionFilter(LPEXCEPTION_POINTERS ep)
{
	PCONTEXT ctx = ep->ContextRecord;

	ctx->Ebx = -1; // Not running VPC  
	ctx->Eip += 4; // skip past the "call VPC" opcodes  
	return EXCEPTION_CONTINUE_EXECUTION; // we can safely resume execution since we skipped faulty instruction  
}


bool CAntiMultiOpen::checkVPC()
{
	bool rc = false;

	__try
	{
		_asm push ebx
		_asm mov  ebx, 0 // Flag  
		_asm mov  eax, 1 // VPC function number  

		// call VPC   
		_asm __emit 0Fh
		_asm __emit 3Fh
		_asm __emit 07h
		_asm __emit 0Bh

		_asm test ebx, ebx
		_asm setz[rc]
			_asm pop ebx
	}
	// The except block shouldn't get triggered if VPC is running!!  
	__except (IsInsideVPC_exceptionFilter(GetExceptionInformation()))
	{
	}

	return rc;
}

bool CAntiMultiOpen::checkVMWare()
{
	bool rc = true;

	__try
	{
		__asm
		{
			push   edx
				push   ecx
				push   ebx

				mov    eax, 'VMXh'
				mov    ebx, 0 // any value but not the MAGIC VALUE  
				mov    ecx, 10 // get VMWare version  
				mov    edx, 'VX' // port number  

				in     eax, dx // read port  
				// on return EAX returns the VERSION  
				cmp    ebx, 'VMXh' // is it a reply from VMWare?  
				setz[rc] // set return value  

				pop    ebx
				pop    ecx
				pop    edx
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		rc = false;
	}

	return rc;
}

bool CAntiMultiOpen::checkMutex()
{
	HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, false, _T(MUTEX_UNIQUE));
	if (!hMutex)
	{
		HANDLE hObject = ::CreateMutex(NULL, FALSE, _T(MUTEX_UNIQUE));
		//CloseHandle(hObject);
		return true;
	}
	
	return false;
}



bool CAntiMultiOpen::checkDataSeg()
{
	if (isExist > 0)
	{
		return false;
	}
	isExist = isExist + 1;

	return true;
}

bool CAntiMultiOpen::checkVM()
{
	if (checkVMWare())
		return false;

	if (checkVPC())
		return false;

	return true;
}

void CAntiMultiOpen::minusProcessCount()
{
	isExist = isExist - 1;

	while (HANDLE hMutex=OpenMutex(MUTEX_ALL_ACCESS, false, _T(MUTEX_UNIQUE)))
	{
		bool suc = CloseHandle(hMutex);
		Sleep(200);
	}
	
}
