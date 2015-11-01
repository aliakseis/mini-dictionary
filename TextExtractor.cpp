#include "stdafx.h"

#include "TextExtractor.h"

#include <assert.h>

inline LPSTR W2A_(LPCWSTR _lpw) 
{
	int _convert;
	return (_lpw == NULL) ? NULL : (
		_convert = (lstrlenW(_lpw)+1)*2,
		ATLW2AHELPER((LPSTR) malloc(_convert), _lpw, _convert, 1251));
}

static LPFNOBJECTFROMLRESULT g_pfObjectFromLresult 
	= (LPFNOBJECTFROMLRESULT) GetProcAddress(LoadLibrary("OLEACC.DLL"), "ObjectFromLresult");


bool IsOleAccAvailable()
{
	return g_pfObjectFromLresult != NULL;
}

///////////////////////////////////////////////////////////////////////////////////

struct __declspec(uuid("3050f4ae-98b5-11cf-bb82-00aa00bdce0b"))
IHTMLWindow3_ : IDispatch
{
    // Raw methods provided by interface

    virtual HRESULT __stdcall get_screenLeft (
        long * p ) = 0;
    virtual HRESULT __stdcall get_screenTop (
        long * p ) = 0;
    virtual HRESULT __stdcall attachEvent (
        BSTR event,
        IDispatch * pdisp,
        VARIANT_BOOL * pfResult ) = 0;
    virtual HRESULT __stdcall detachEvent (
        BSTR event,
        IDispatch * pdisp ) = 0;
    virtual HRESULT __stdcall setTimeout (
        VARIANT * expression,
        long msec,
        VARIANT * language,
        long * timerID ) = 0;
    virtual HRESULT __stdcall setInterval (
        VARIANT * expression,
        long msec,
        VARIANT * language,
        long * timerID ) = 0;
    virtual HRESULT __stdcall print ( ) = 0;
    virtual HRESULT __stdcall put_onbeforeprint (
        VARIANT p ) = 0;
    virtual HRESULT __stdcall get_onbeforeprint (
        VARIANT * p ) = 0;
    virtual HRESULT __stdcall put_onafterprint (
        VARIANT p ) = 0;
    virtual HRESULT __stdcall get_onafterprint (
        VARIANT * p ) = 0;
    virtual HRESULT __stdcall get_clipboardData (
        struct IHTMLDataTransfer * * p ) = 0;
    virtual HRESULT __stdcall showModelessDialog (
        BSTR url,
        VARIANT * varArgIn,
        VARIANT * options,
        struct IHTMLWindow2 * * pDialog ) = 0;
};


struct __declspec(uuid("4e747be5-2052-4265-8af0-8ecad7aad1c0"))
ISimpleDOMText_ : IUnknown
{
    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_domText( 
        /* [retval][out] */ BSTR __RPC_FAR *domText) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE get_clippedSubstringBounds( 
        /* [in] */ unsigned int startIndex,
        /* [in] */ unsigned int endIndex,
        /* [out] */ int __RPC_FAR *x,
        /* [out] */ int __RPC_FAR *y,
        /* [out] */ int __RPC_FAR *width,
        /* [out] */ int __RPC_FAR *height) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE get_unclippedSubstringBounds( 
        /* [in] */ unsigned int startIndex,
        /* [in] */ unsigned int endIndex,
        /* [out] */ int __RPC_FAR *x,
        /* [out] */ int __RPC_FAR *y,
        /* [out] */ int __RPC_FAR *width,
        /* [out] */ int __RPC_FAR *height) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE scrollToSubstring( 
        /* [in] */ unsigned int startIndex,
        /* [in] */ unsigned int endIndex) = 0;
    
    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_fontFamily( 
        /* [retval][out] */ BSTR __RPC_FAR *fontFamily) = 0;
    
};


struct __declspec(uuid("5007373a-20d7-458f-9ffb-abc900e3a831")) 
IPDDomNode : IDispatch
{

    virtual HRESULT STDMETHODCALLTYPE GetParent
    (
        /*[out, retval]*/ IPDDomNode **ppDispParent
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetType
    (
		/*[out, retval]*/ long *pNodeType
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetChildCount
    (
        /*[out, retval]*/ long *pCountChildren
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetChild
    (
		/*[in]*/ long index,
        /*[out, retval]*/ IPDDomNode **ppDispChild
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetName
    (
        /*[out, retval]*/ BSTR *pszName
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetValue
    (
        /*[out, retval]*/ BSTR *pszName
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE IsSame
    (
	    /*[in]*/ IPDDomNode *node,
	    /*[out, retval]*/ BOOL *isSame
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetTextContent
    (
        /*[out, retval]*/ BSTR *pszText
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetLocation
    (
		/*[out]*/ long *pxLeft, 
		/*[out]*/ long *pyTop, 
		/*[out]*/ long *pcxWidth, 
		/*[out]*/ long *pcyHeight
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetFontInfo
    (
		/*[out]*/ long *fontStatus,
        /*[out]*/ BSTR *pszName,
        /*[out]*/ float *fontSize,
		/*[out]*/ long *fontFlags, 
		/*[out]*/ float *red,
 		/*[out]*/ float *green,
		/*[out]*/ float *blue
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetFromID
    (
		/*[in]*/ BSTR id,
        /*[out, retval]*/ IPDDomNode **ppDispNode
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetIAccessible
    (
		 /*[out, retval]*/ IAccessible **ppDispIAccessible
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE ScrollTo
    (
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetTextInLines
    (
		/*[in]*/ BOOL visibleOnly,
        /*[out, retval]*/ IPDDomNode **ppDisp
    ) = 0;

};

struct __declspec(uuid("f9f2fe81-f764-4bd0-afa5-5de841ddb625")) 
IGetPDDomNode : IUnknown
{
	/*[id(24), helpstring("method get_PDDomNode")]*/ 
	virtual HRESULT STDMETHODCALLTYPE get_PDDomNode(
		/*[in]*/ VARIANT varID, /*[out, retval]*/ IPDDomNode **ppDispDoc) = 0;
};

static const GUID SID_GetPDDomNode = 
{ 0xc0a1d5e9, 0x1142, 0x4cf3, { 0xb6, 0x7, 0x82, 0xfc, 0x3b, 0x96, 0xa4, 0xdf } };


///////////////////////////////////////////////////////////////////////////////////

void IeZoomRecalculate(IAccessible* pacc, POINT& pt)
{
	long count = 0;
	pacc->get_accChildCount(&count);
	if (1 == count)
	{
		VARIANT v;
		v.vt = VT_I4;
		v.lVal = 0;
		CComPtr<IDispatch> spDisp;
		pacc->get_accChild(v, &spDisp);
		CComQIPtr<IServiceProvider> pServiceProvider(spDisp);

		CComDispatchDriver spElement;
		if (pServiceProvider != NULL 
			&& SUCCEEDED(pServiceProvider->QueryService(__uuidof(IHTMLElement), __uuidof(IDispatch), 
				(void **)&spElement))
			&& spElement != NULL)
		{
			CComQIPtr<IAccessible> paccChild(spDisp);
  			long screenLeft, screenTop, screenWidth, screenHeight;

			VARIANT v;
			v.vt = VT_I4;
			v.lVal = CHILDID_SELF;

			if (paccChild != NULL 
				&& SUCCEEDED(paccChild->accLocation(&screenLeft, &screenTop, &screenWidth, &screenHeight, v))
				&& screenWidth > 0 && screenHeight > 0)
			{
 				CComVariant width, height;
				if (spElement != NULL 
                    && SUCCEEDED(spElement.GetPropertyByName(L"offsetWidth", &width)) && width.lVal > 0
					&& SUCCEEDED(spElement.GetPropertyByName(L"offsetHeight", &height)) && height.lVal > 0)
				{
 					pt.x = pt.x * width.lVal / screenWidth;
 					pt.y = pt.y * height.lVal / screenHeight;
				}
			}
		}
	}
}

CComPtr<IServiceProvider> GetServiceProvider(HWND hWnd, POINT& pt, bool isIE = false)
{
	CComVariant vtChild;
	CComPtr<IAccessible> pacc;
	LRESULT lRes = 0;

	// Send WM_GETOBJECT to document window
	if (!SendMessageTimeout( hWnd, WM_GETOBJECT, 0L, OBJID_CLIENT,
						SMTO_ABORTIFHUNG, 500, (DWORD*)&lRes )
			|| 0 == lRes)
		return 0;

	if (FAILED(g_pfObjectFromLresult(lRes, __uuidof(IAccessible), 0, (void**)&pacc))) 
		return 0;

	POINT ptScreen(pt);

	if (isIE)
		IeZoomRecalculate(pacc, pt);

	CComQIPtr<IAccessible> paccChild;
	for (; SUCCEEDED(pacc->accHitTest(ptScreen.x, ptScreen.y, &vtChild))
				 && VT_DISPATCH == vtChild.vt && (paccChild = vtChild.pdispVal) != NULL; 
			vtChild.Clear() )
	{
		pacc.Attach(paccChild.Detach());
	}

	return CComQIPtr<IServiceProvider>(pacc);
}


class ITextExtractor
{
public:
	virtual bool ExtractText(HWND hWnd, 
		POINT pt,
		char*& rpszWord, char*& rpszText) = 0;
protected:
	~ITextExtractor() {}
};


class InternetExplorerTextExtractor : public ITextExtractor
{
public:
	enum { EXTRACT_TYPE = EXTRACT_TYPE_INTERNET_EXPLORER };
	static const char g_szClassName[];

	virtual bool ExtractText(HWND hWnd,
		POINT pt,
		char*& rpszWord, char*& rpszText);
};

class MozillaFirefoxTextExtractor : public ITextExtractor
{
public:
	enum { EXTRACT_TYPE = EXTRACT_TYPE_MOZILLA_FIREFOX };
	static const char g_szClassName[];

	virtual bool ExtractText(HWND hWnd,
		POINT pt,
		char*& rpszWord, char*& rpszText);
};

class AdobeAcrobatTextExtractor : public ITextExtractor
{
public:
	enum { EXTRACT_TYPE = EXTRACT_TYPE_ADOBE_ACROBAT };
	static const char g_szClassName[];

	virtual bool ExtractText(HWND hWnd,
		POINT pt,
		char*& rpszWord, char*& rpszText);
};

class MsWordTextExtractor : public ITextExtractor
{
public:
	enum { EXTRACT_TYPE = EXTRACT_TYPE_MS_WORD };
	static const char g_szClassName[];

	virtual bool ExtractText(HWND hWnd,
		POINT pt,
		char*& rpszWord, char*& rpszText);
};


bool InternetExplorerTextExtractor::ExtractText(
		HWND hWnd, 
		POINT pt,
		char*& rpszWord, char*& rpszText)
{
	CComPtr<IServiceProvider> pServiceProvider(GetServiceProvider(hWnd, pt, true));
	if (!pServiceProvider)
		return false;

	CComDispatchDriver spElement;
	if (FAILED(pServiceProvider->QueryService(__uuidof(IHTMLElement), __uuidof(IDispatch), 
				(void **)&spElement))
			|| !spElement)
		return false;

	CComVariant isTextEdit;

    if (SUCCEEDED(spElement.GetPropertyByName(L"isTextEdit", &isTextEdit)) && !isTextEdit.boolVal)
	{
        CComVariant parent;
		if (FAILED(spElement.GetPropertyByName(L"parentTextEdit", &parent)))
			return 0;

        if (VT_DISPATCH != parent.vt)
			return 0;
        spElement = parent.pdispVal;
        if (!spElement)
            return 0;
	}

	CComQIPtr<IHTMLInputTextElement> spInput(spElement);
	if (spInput != NULL)
		return 0;

	{		
		CComVariant spDisp;
		if (FAILED(spElement.GetPropertyByName(L"document", &spDisp)))
			return 0;

        assert(VT_DISPATCH == spDisp.vt);
		CComQIPtr<IHTMLDocument2> spDoc(spDisp.pdispVal);
		if (spDoc == NULL)
			return 0;

		CComPtr<IHTMLWindow2> spWindow;
		if (FAILED(spDoc->get_parentWindow(&spWindow))) 
			return 0;
		CComQIPtr<IHTMLWindow3_> spContentWindow3(spWindow);
		if (spContentWindow3 == NULL)
			return 0;

		long left = 0, top = 0;
		spContentWindow3->get_screenLeft(&left);
		spContentWindow3->get_screenTop(&top);
		pt.x -= left;
		pt.y -= top;
	}

	CComVariant vtTxtRange;
	if (FAILED(spElement.Invoke0(L"createTextRange", &vtTxtRange) ))
		return 0;

	CComQIPtr<IHTMLTxtRange> spOrigin(vtTxtRange.pdispVal);
	vtTxtRange.Clear();

	if (!spOrigin)
		return false;

	if (FAILED(spOrigin->moveToPoint(pt.x, pt.y)))
		return 0;

	CComPtr<IHTMLTxtRange> spRange;
	if (FAILED(spOrigin->duplicate(&spRange)))
		return 0;

	CComBSTR bstrText, bstrWord;
	const CComBSTR bstrWordUnit(L"word");

	long lActualCount = 0;
	spRange->move(bstrWordUnit, -1, &lActualCount);
	spRange->moveEnd(bstrWordUnit, 1, &lActualCount);

	spOrigin->move(CComBSTR(L"character"), 1, &lActualCount);

	VARIANT_BOOL isInRange;
	if (FAILED(spRange->inRange(spOrigin, &isInRange)) || !isInRange)
		return 0;

	spRange->get_text(&bstrWord);
	if (!bstrWord)
		return 0;

	spRange->moveEnd(bstrWordUnit, 3, &lActualCount);

	spRange->get_text(&bstrText);
	if (!bstrText)
		return 0;

	rpszWord = W2A_(bstrWord);
	rpszText = W2A_(bstrText);

	return true;
}


inline bool IsWSpace(unsigned int wchar)
{
	return wchar < 'A' || 160 == wchar;
}


bool MozillaFirefoxTextExtractor::ExtractText(
		HWND hWnd, 
		POINT pt,
		char*& rpszWord, char*& rpszText)
{
	CComPtr<IServiceProvider> pServiceProvider(GetServiceProvider(hWnd, pt));
	if (!pServiceProvider)
		return false;

	const GUID refguid = {0x0c539790, 0x12e4, 0x11cf,
							0xb6, 0x61, 0x00, 0xaa, 0x00, 0x4c, 0xd6, 0xd8};

	CComPtr<ISimpleDOMText_> spSimpleDOMText;
	if (FAILED(pServiceProvider->QueryService(refguid, __uuidof(ISimpleDOMText_), 
				(void **)&spSimpleDOMText))
			|| !spSimpleDOMText)
		return false;

	CComBSTR bstrText;

	if (FAILED(spSimpleDOMText->get_domText(&bstrText)))
		return false;

	int length = bstrText.Length();

	int l = 0;
	int h = length - 1;
	while( l <= h )
	{
		int i = (l + h) >> 1;

		int x, y, width, height;
		x = y = width = height = 0;
		while ( i >= l 
			&& (spSimpleDOMText->get_unclippedSubstringBounds(i, i + 1, 
											  &x,
											  &y,
											  &width,
											  &height)
				, 0 == x && 0 == y && 0 == width && 0 == height))
			--i;

		if ( i < l || (pt.y >= y + height || pt.y >= y && pt.x >= x) )
			l = ((l + h) >> 1) + 1;
		else
			h = i - 1;
	}

	if (h < 0 || IsWSpace(bstrText[h]))
		return false;

	int begin = h;
	while (begin > 0 && !IsWSpace(bstrText[begin - 1]))
		--begin;

	if (length - begin > 1000)
		bstrText[begin + 1000] = 0;

	rpszText = W2A_(bstrText + begin);

	int end = h;
	while (end < length && !IsWSpace(bstrText[end]))
		++end;
	bstrText[end] = 0;
	rpszWord = W2A_(bstrText + begin);

	return true;
}

bool AdobeAcrobatTextExtractor::ExtractText(
		HWND hWnd, 
		POINT pt,
		char*& rpszWord, char*& rpszText)
{
	CComPtr<IServiceProvider> pServiceProvider(GetServiceProvider(hWnd, pt));
	if (!pServiceProvider)
		return false;

	CComPtr<IGetPDDomNode> spGetPDDomNode;
	if (FAILED(pServiceProvider->QueryService(SID_GetPDDomNode, __uuidof(IGetPDDomNode), 
				(void **)&spGetPDDomNode))
			|| !spGetPDDomNode)
		return false;

	CComPtr<IPDDomNode> spPDDomNode;

	if (FAILED(spGetPDDomNode->get_PDDomNode (CComVariant(CHILDID_SELF), &spPDDomNode))
			|| !spPDDomNode)
		return false;

	long lNodeType = 0;
	if (FAILED(spPDDomNode->GetType(&lNodeType))
			|| lNodeType != 4)
		return false;


	long lCountChildren = 0;
	if (FAILED(spPDDomNode->GetChildCount(&lCountChildren))
			|| 0 == lCountChildren)
		return false;

	long l = 0;
	long h = lCountChildren - 1;
	while( l <= h )
	{
		long i = (l + h) >> 1;

		CComPtr<IPDDomNode> spPDChildNode;
		if (FAILED(spPDDomNode->GetChild(i, &spPDChildNode))
				|| !spPDChildNode)
			return false;

		CComPtr<IPDDomNode> spPDChildNode2;
		if (SUCCEEDED(spPDChildNode->GetChild(0, &spPDChildNode2))
				&& !!spPDChildNode2)
		{
			spPDChildNode.Attach(spPDChildNode2.Detach());
		}

		long x, y, width, height;
		x = y = width = height = 0;

		if (FAILED(spPDChildNode->GetLocation(&x, &y, &width, &height)))
			return false;

		if (pt.y >= y + height || pt.y >= y && pt.x >= x)
			l = i + 1;
		else
			h = i - 1;
	}

	if (h < 0)
		return false;

	CComPtr<IPDDomNode> spPDChildNode;
	if (FAILED(spPDDomNode->GetChild(h, &spPDChildNode))
			|| !spPDChildNode)
		return false;

	CComBSTR bstrText;
	if (FAILED(spPDChildNode->GetTextContent(&bstrText)))
		return false;

	rpszWord = W2A_(bstrText);

	if (lCountChildren > h + 4)
		lCountChildren = h + 4;

	while (++h < lCountChildren)
	{
		CComPtr<IPDDomNode> spPDChildNode;
		CComBSTR bstrWord;
		if (FAILED(spPDDomNode->GetChild(h, &spPDChildNode))
				|| !spPDChildNode
				|| FAILED(spPDChildNode->GetTextContent(&bstrWord)))
		{
			free(rpszWord);
			rpszWord = 0;
			return false;
		}
		bstrText += bstrWord;
	}

	rpszText = W2A_(bstrText);

	return true;
}


bool MsWordTextExtractor::ExtractText(
		HWND hWnd, 
		POINT pt,
		char*& rpszWord, char*& rpszText)
{
	LRESULT lRes = 0;
	// Send WM_GETOBJECT to document window
	if (!SendMessageTimeout(hWnd, WM_GETOBJECT, 0L, OBJID_NATIVEOM,
						SMTO_ABORTIFHUNG, 500, (DWORD*)&lRes )
			|| 0 == lRes)
		return 0;

	CComDispatchDriver pacc;

	if (FAILED(g_pfObjectFromLresult(lRes, __uuidof(IDispatch), 0, (void**)&pacc.p))) 
		return 0;

	CComVariant vtApplication;
	if (FAILED(pacc.GetPropertyByName(L"Application", &vtApplication)))
		return 0;

	CComDispatchDriver spApplication(vtApplication.pdispVal);

	CComVariant vtActiveWindow;
	if (FAILED(spApplication.GetPropertyByName(L"ActiveWindow", &vtActiveWindow)))
		return 0;

	CComDispatchDriver spWindow(vtActiveWindow.pdispVal);

	CComVariant vtRange;
	if (spWindow == NULL || FAILED(spWindow.Invoke2(L"RangeFromPoint", 
			&CComVariant(pt.x),
			&CComVariant(pt.y),
			&vtRange)))
		return 0;

	if (VT_EMPTY == V_VT(&vtRange))
		return 0;

	CComDispatchDriver spOrigin(vtRange.pdispVal);
	vtRange.Clear();

	if (spOrigin == NULL || FAILED(spOrigin.GetPropertyByName(L"Duplicate", &vtRange)))
		return 0;

	CComDispatchDriver spRange(vtRange.pdispVal);
	vtRange.Clear();

	if (spRange == NULL || FAILED(spRange.Invoke2(L"Move", 
			&CComVariant(2L),
			&CComVariant(-1L),
			&vtRange)))
		return 0;
	vtRange.Clear();

	if (FAILED(spRange.Invoke2(L"MoveEnd", 
			&CComVariant(2L),
			&CComVariant(1L),
			&vtRange)))
		return 0;
	vtRange.Clear();

	if (FAILED(spOrigin.Invoke2(L"Move", 
			&CComVariant(1L),
			&CComVariant(1L),
			&vtRange)))
		return 0;
	vtRange.Clear();


	if (FAILED(spOrigin.Invoke1(L"InRange", 
			&CComVariant(spRange),
			&vtRange)))
		return 0;
	vtRange.Clear();

	CComVariant vtWord;
	if (FAILED(spRange.GetPropertyByName(L"Text", &vtWord)))
		return 0;

	if (FAILED(spRange.Invoke2(L"MoveEnd", 
			&CComVariant(2L),
			&CComVariant(3L),
			&vtRange)))
		return 0;
	vtRange.Clear();

	CComVariant vtText;
	if (FAILED(spRange.GetPropertyByName(L"Text", &vtText)))
		return 0;

	rpszWord = W2A_(vtWord.bstrVal);
	rpszText = W2A_(vtText.bstrVal);

	return true;
}

const char InternetExplorerTextExtractor::g_szClassName[] = "Internet Explorer_Server"; 
const char MozillaFirefoxTextExtractor::g_szClassName[]  = "MozillaWindowClass";
const char AdobeAcrobatTextExtractor::g_szClassName[]  = "AVL_AVView";
const char MsWordTextExtractor::g_szClassName[]  = "_WwG";


template<typename T, typename N>
struct ExtractorHandler
{
	static ITextExtractor* Get(int nTypes
		, const char* szClassName, int classNameLength)
	{
		const int CLASS_NAME_LENGTH 
			= sizeof(T::g_szClassName) / sizeof(T::g_szClassName[0]) - 1;
		if ((T::EXTRACT_TYPE & nTypes)
			&& CLASS_NAME_LENGTH == classNameLength
			&& 0 == memcmp(T::g_szClassName, szClassName, CLASS_NAME_LENGTH))
		{
			static T textExtractor;
			return &textExtractor;
		}
		return N::Get(nTypes, szClassName, classNameLength);
	}
};

struct NullHandler
{
	static ITextExtractor* Get(...) { return 0; }
};

ITextExtractor* GetTextExtractor(HWND hWnd, int nTypes)
{
	if (NULL == hWnd)
		return 0;

	char szBufrer[64];

	const int classNameLength
		= ::GetClassName(hWnd, szBufrer, sizeof(szBufrer) / sizeof(szBufrer[0]));

	return 
		ExtractorHandler<InternetExplorerTextExtractor,
		ExtractorHandler<MozillaFirefoxTextExtractor,
		ExtractorHandler<AdobeAcrobatTextExtractor,
		ExtractorHandler<MsWordTextExtractor, NullHandler> > > >
			::Get(nTypes, szBufrer, classNameLength);
}

bool ExtractText(
		HWND hWnd, 
		int nTypes,
		POINT pt,
		char*& rpszWord, 
		char*& rpszText)
{
	ITextExtractor* pTextExtractor = GetTextExtractor(hWnd, nTypes);
	if (0 != pTextExtractor)
	{

	//  extract
		return  pTextExtractor->ExtractText(hWnd, pt, rpszWord, rpszText);
	}
	return false;
}
