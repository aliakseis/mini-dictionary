#include "stdafx.h"


#ifdef _ATL_MIN_CRT

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>
#include <string.h>
#include <io.h>

#include <new>

#include <errno.h>

typedef int errno_t;
#ifndef ERANGE
#define ERANGE          34
#endif

#ifndef SIZE_MAX
#ifdef _WIN64 
#define SIZE_MAX _UI64_MAX
#else
#define SIZE_MAX 0xffffffff
#endif
#endif

const std::nothrow_t std::nothrow;

int _purecall( void )
{
    return 0;
} 

HANDLE g_hHeap;

void * operator new(size_t nSize, const std::nothrow_t&)
{
	if (!nSize)
		++nSize;
	return HeapAlloc( g_hHeap, 0, nSize );
}

void operator delete(void * lpMem)
{
	if(lpMem)
		HeapFree(g_hHeap, 0, lpMem);
}

void * operator new[](size_t nSize, const std::nothrow_t&)
{
	if (!nSize)
		++nSize;
	return HeapAlloc( g_hHeap, 0, nSize );
}

void operator delete[](void * lpMem)
{
	if(lpMem)
		HeapFree(g_hHeap, 0, lpMem);
}


void * memmove (
        void * dst,
        const void * src,
        size_t count
        )
{
	_asm
	{
		mov	ecx, count
		jecxz lexit
		push	esi
		push	edi
		mov	esi, src
			mov	edi, dst

		cmp	esi, edi
		jae	l1
		add	esi, ecx
		dec	esi
		add	edi, ecx
		dec	edi
		std
l1:	rep	movsb
		cld
		pop	edi
		pop	esi
lexit:
	}
	return dst;
}

#pragma function(memcpy, memset)

void * memcpy (
	   void * dst,
	   const void * src,
	   size_t count
	   )
{
	_asm
	{
		mov	ecx, count
		jecxz lexit
		push	esi
		push	edi
		mov	esi, src
		mov	edi, dst

		rep	movsb
		pop	edi
		pop	esi
lexit:
	}
	return dst;
}

void * memset (
	   void *dst,
	   int val,
	   size_t count
	   )
{
	_asm
	{
		mov	ecx, count
		jecxz lexit
		push	edi
		mov	edi, dst

		mov eax, val

		rep	stosb
		pop	edi
lexit:
	}
	return dst;
}

errno_t strcpy_s(
				 char *_DEST, size_t _SIZE, const char *_SRC 
				 )
{
	char *p = _DEST;
	size_t available = _SIZE;
	while ((*p++ = *_SRC++) != 0 && --available > 0)
	{
	}

	if (available == 0)
	{
		return ERANGE;                           

	}
	return 0;
}

void exit( int status )
{
	ExitProcess(status);
}

template <typename T>
inline void Div(T& dividend, unsigned long divisor)
{
	dividend /= divisor;
}

inline void Div(unsigned __int64& dividend, unsigned long divisor)
{
	__asm 
	{
		mov		ebx, dividend
		mov     ecx, divisor
		mov     eax, [ebx + 4]	// load high word of dividend
		xor     edx, edx
		div     ecx             // get high order bits of quotient
		mov     [ebx + 4], eax  // save high bits of quotient
		mov     eax, [ebx]		// edx:eax <- remainder:lo word of dividend
		div     ecx             // get low order bits of quotient
		mov     [ebx], eax      
	};
}


//	Taken from the CRT

template <typename T>
void xtoa (
        /*unsigned long*/ T val,
        char *buf,
        unsigned radix,
        int is_neg
        )
{
        char *p = buf;		/* pointer to traverse string */

        if (is_neg) {
            /* negative, so output '-' and negate */
            *p++ = '-';
            val = -val;//(unsigned long)(-(long)val);
        }

        char *firstdig = p;           /* save pointer to first digit */

        do {
            //digval = (unsigned) (val % radix);
			T old_val = val;
            Div(val, radix); //val /= radix;       /* get next digit */
			unsigned digval = old_val - val * radix;/* value of digit */

            /* convert to ascii and store */
            if (digval > 9)
                *p++ = (char) (digval - 10 + 'a');  /* a letter */
            else
                *p++ = (char) (digval + '0');       /* a digit */
        } while (val > 0);

        /* We now have the digit of the number in the buffer, but in reverse
           order.  Thus we reverse them now. */

        *p-- = '\0';            /* terminate string; p points to last digit */

        do {
            char temp = *p;
            *p = *firstdig;
            *firstdig = temp;   /* swap *p and *firstdig */
            --p;
            ++firstdig;         /* advance to next two digits */
        } while (firstdig < p); /* repeat until halfway */
}


char * __cdecl itoa (
        int val,
        char *buf,
        int radix
        )
{
        if (radix == 10 && val < 0)
            xtoa((unsigned long)val, buf, radix, 1);
        else
            xtoa((unsigned long)(unsigned int)val, buf, radix, 0);
        return buf;
}

errno_t __cdecl _ui64toa_s(unsigned __int64 _Val, char *_Buf, size_t _SizeInChars, int _Radix)
{
	(_SizeInChars);
	xtoa(_Val, _Buf, _Radix, 0);
	return 0;
}


typedef void (__cdecl *_PVFV)(void);


#pragma data_seg(".CRT$XCA")
_PVFV __xc_a[] = { NULL };


#pragma data_seg(".CRT$XCZ")
_PVFV __xc_z[] = { NULL };

#pragma data_seg()  /* reset */

#pragma comment(linker, "/merge:.CRT=.data")


void __cdecl _initterm (
        _PVFV * pfbegin,
        _PVFV * pfend
        )
{
    for (; pfbegin < pfend; ++pfbegin)
    {
        // if current table entry is non-NULL, call thru it.
        if ( *pfbegin != NULL )
            (**pfbegin)();
    }
}





char g_chEndingZero;

void WinMainCRTStartup()
{
	g_hHeap = GetProcessHeap();

	_initterm( __xc_a, __xc_z );

	STARTUPINFO StartupInfo;
	StartupInfo.dwFlags = 0;
	GetStartupInfo(&StartupInfo);

	ExitProcess(WinMain(GetModuleHandle(NULL)
		, NULL, &g_chEndingZero, 
		(StartupInfo.dwFlags & STARTF_USESHOWWINDOW)? StartupInfo.wShowWindow : SW_SHOWDEFAULT));
}


void *malloc(size_t size )
{
	if (!size)
		++size;
	return HeapAlloc( g_hHeap, 0, size );
}


void *realloc(void *p,
   size_t n )
{
	if (p == NULL)
		return malloc(n);
	else if (n == 0)
	{
		free(p);
		return NULL;
	}
	else
		return HeapReAlloc(g_hHeap, 0, p, n);
}

void free( void *memblock )
{
	if(memblock)
		HeapFree(g_hHeap, 0, memblock);
}

void* __cdecl _recalloc(void* p, size_t n, size_t c)
{
	if(n!=0 && SIZE_MAX/n<c)
	{
		return NULL;
	}
	return realloc(p, n*c);
}


int atexit(void (__cdecl * )( void ))
{
	return -1;
}


const IID GUID_NULL = { 0 };


// ATL stuff
#if _ATL_VER >= 0x0800

ATL::CAtlWinModule ATL::_AtlWinModule;

namespace ATL {

	PVOID
		__stdcall __AllocStdCallThunk (
		VOID
		)
	{
		return HeapAlloc(g_hHeap,
			0,
			sizeof(ATL::_stdcallthunk));
	}

	VOID
		__stdcall __FreeStdCallThunk (
		IN PVOID Thunk
		)
	{
		HeapFree(g_hHeap,0,Thunk);
	}

}   // namespace ATL

#endif

#endif // NODEFAULTLIB
