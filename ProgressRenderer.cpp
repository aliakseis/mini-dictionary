#include "stdafx.h"

#include "ProgressRenderer.h"

class CStreamOnBytes : public IStream
{
public:
// Construction
    CStreamOnBytes(char* pStream_, unsigned int length_)
        : pStream(pStream_), length(length_), m_current_index(0) {}

// Implementation
	STDMETHOD(QueryInterface)(REFIID iid, void **ppUnk)
	{
		if (::IsEqualGUID(iid, __uuidof(IUnknown)) ||
			::IsEqualGUID(iid, __uuidof(IStream)) ||
			::IsEqualGUID(iid, __uuidof(ISequentialStream)))
		{
			*ppUnk = (void*)this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef( void) 
	{
		return (ULONG)1;
	}

	ULONG STDMETHODCALLTYPE Release( void) 
	{
		return (ULONG)1;
	}

	STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead)
	{
		if (pcbRead)
			*pcbRead = 0;
		if (!pv)
			return E_POINTER;

		if (!pStream)
			return E_UNEXPECTED;

		char *pStart = pStream + m_current_index;
		char *pEnd = pStream + length;
		if (pStart >= pEnd)
			return S_FALSE; // no more data to read

		int bytes_left = (int)(pEnd-pStart);
		int bytes_to_copy = (int)min(bytes_left, (int)cb);
		if (bytes_to_copy <= 0)
		{
			// reset members so this stream can be used again
			m_current_index = 0;
			return S_FALSE;
		}

		memcpy(pv, pStream + m_current_index, bytes_to_copy);
		if (pcbRead)
			*pcbRead = (ULONG)bytes_to_copy;
		m_current_index += bytes_to_copy;
		return S_OK;
	}

	STDMETHOD(Write)(const void*, ULONG, ULONG*)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(Seek)(LARGE_INTEGER dlibMove,
            DWORD dwOrigin,
            ULARGE_INTEGER* plibNewPosition)
	{

        unsigned int newIndex = dlibMove.QuadPart;
        switch (dwOrigin)
        {
        case STREAM_SEEK_SET: break;
        case STREAM_SEEK_CUR: newIndex += m_current_index; break;
        case STREAM_SEEK_END: newIndex += length; break;
        default: return E_UNEXPECTED;
        }
        if ((int)newIndex < 0)
            newIndex = 0;
        if (newIndex >= length)
            newIndex = length - 1;

        m_current_index = newIndex;

        if (plibNewPosition)
            plibNewPosition->QuadPart = m_current_index;

        return S_OK;
	}

	STDMETHOD(SetSize)(ULARGE_INTEGER )
	{
		return E_NOTIMPL;
	}

	STDMETHOD(CopyTo)(IStream *, ULARGE_INTEGER , ULARGE_INTEGER *,
		ULARGE_INTEGER *)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(Commit)(DWORD )
	{
		return E_NOTIMPL;
	}

	STDMETHOD(Revert)( void)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(LockRegion)(ULARGE_INTEGER , ULARGE_INTEGER , DWORD )
	{
		return E_NOTIMPL;
	}

	STDMETHOD(UnlockRegion)(ULARGE_INTEGER , ULARGE_INTEGER ,
		DWORD )
	{
		return E_NOTIMPL;
	}

	STDMETHOD(Stat)(STATSTG *, DWORD )
	{
		return E_NOTIMPL;
	}

	STDMETHOD(Clone)(IStream **)
	{
		return E_NOTIMPL;
	}

protected:
	unsigned int length;
	char *pStream;
	UINT m_current_index;
};


// CpictureTestView drawing

#ifndef HIMETRIC_INCH
#define HIMETRIC_INCH   2540    // HIMETRIC units per inch
#endif

HBITMAP Create24BPPDIBSection(HDC hDC, int iWidth, int iHeight)
{
    BITMAPINFO bmi = {
        sizeof(BITMAPINFOHEADER),
        iWidth,
        iHeight,
        1,
        24,
        BI_RGB
    };
    // Create the surface.
    return CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
}



BOOL GrayImage(HBITMAP hbmDst)
{
    BITMAP bmDst;
    
    // Get information about the surfaces you were passed.
    if (!GetObject(hbmDst,  sizeof(BITMAP), &bmDst))  return FALSE;

    // Make sure you have data that meets your requirements.
    if (bmDst.bmBitsPixel != 24) return FALSE;
    if (bmDst.bmPlanes != 1) return FALSE;

    // Initialize the surface pointers.
    RGBTRIPLE *lprgbDst  = (RGBTRIPLE *)bmDst.bmBits;

    for (int y=0; y<bmDst.bmHeight; y++) {
        for (int x=0; x<bmDst.bmWidth; x++) {
            lprgbDst[x].rgbtRed   =                                     
            lprgbDst[x].rgbtGreen =                                     
            lprgbDst[x].rgbtBlue  = 
                (lprgbDst[x].rgbtRed  +                                     
                lprgbDst[x].rgbtGreen +                                     
                lprgbDst[x].rgbtBlue) / 5;                                     
        }

        // Move to next scan line.
        lprgbDst  = (RGBTRIPLE *)((LPBYTE)lprgbDst  + bmDst.bmWidthBytes);
    }

    return TRUE;
}

CProgressRenderer::CProgressRenderer()
: m_nWidth(0), m_nHeight(0)
, m_hbmDst(NULL)
, m_hdcSrc(NULL)
, m_hdcDst(NULL)
, m_nPos(0)
{
    HRSRC hFound = FindResource(NULL, MAKEINTRESOURCE(IDR_PICTURE), RT_RCDATA);
    HGLOBAL hRes = LoadResource(NULL, hFound);

    CStreamOnBytes streamOnBytes((char*) LockResource(hRes), SizeofResource(NULL, hFound));

    HRESULT hr = ::OleLoadPicture(
                    &streamOnBytes,            // [in] Pointer to the stream that contains picture's data
                    0,                    // [in] Number of bytes read from the stream (0 == entire)
                    true,                // [in] Loose original format if true
                    __uuidof(IPicture),        // [in] Requested interface
                    (void**)&m_spPicture // [out] IPictire object on success
                    );

    if (SUCCEEDED(hr) && m_spPicture)
    {
        OLE_HANDLE handle = NULL;
        m_spPicture->get_Handle(&handle);

        BITMAP bm;
        BOOL res = GetObject((HGDIOBJ)handle, sizeof(BITMAP), &bm);
        if (res)
        {
            m_nWidth = bm.bmWidth;
            m_nHeight = bm.bmHeight;

            m_hbmDst = Create24BPPDIBSection(NULL, m_nWidth, m_nHeight);
            m_hdcDst = CreateCompatibleDC(NULL);
            SelectObject(m_hdcDst, m_hbmDst);

            m_hdcSrc = CreateCompatibleDC(NULL);
            SelectObject(m_hdcSrc, (HGDIOBJ)handle);

            BitBlt(m_hdcDst, 0, 0, m_nWidth, m_nHeight,
                m_hdcSrc, 0, 0, SRCCOPY);

            GrayImage(m_hbmDst);
        }
    }
}

CProgressRenderer::~CProgressRenderer()
{
    DeleteDC(m_hdcSrc);
    DeleteDC(m_hdcDst);
    DeleteObject(m_hbmDst);
}

bool CProgressRenderer::SetPos(int nPos, int nCount)
{
    nPos = MulDiv(nPos, m_nWidth, nCount);
    if (nPos != m_nPos)
    {
        m_nPos = nPos;
        BitBlt(m_hdcDst, 0, 0, m_nPos, m_nHeight, 
               m_hdcSrc, 0, 0, SRCCOPY);
        return true;
    }
    return false;
}

void CProgressRenderer::Render(HDC hDC, const RECT& rect)
{
    BitBlt(hDC, 
        (rect.right - rect.left - m_nWidth) / 2, 
        (rect.bottom - rect.top - m_nHeight) / 2, 
        m_nWidth, m_nHeight, 
            m_hdcDst, 0, 0, SRCCOPY);
}
