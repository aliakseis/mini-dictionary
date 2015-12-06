#include "stdafx.h"


#include "dictlib.h"
#include "TextExtractor.h"
#include "ProgressRenderer.h"

#include "resource.h"

#pragma intrinsic(memcpy, strlen, strcmp, strcat, strcpy, abs)


#define COMMAND_ID_HANDLER_NO_PARAMS(id, func) \
	if(uMsg == WM_COMMAND && id == LOWORD(wParam)) \
	{ \
		func(); \
		return TRUE; \
	}

inline LPWSTR A2W_(LPCSTR _lpa)
{
	int _convert;
	return (_lpa == NULL) ? NULL : (
		_convert = (lstrlenA(_lpa)+1),
		ATLA2WHELPER((LPWSTR) malloc(_convert*2), _lpa, _convert, 1251));
}


class CSubStringSearch
{

    unsigned char *pattern;
    int plen;        
    int d[256];      

    int indexBM(const unsigned char *str)        
    {
        int slen = strlen((const char*)str); 
        int pindx;  
        int cmppos; 
        int endpos;

        for( endpos = plen; endpos < slen ; )
		{
           for( cmppos = endpos, pindx = plen;
                                 pindx >= 0 ;
                                 cmppos--, pindx-- )
              if ( ToLowerCase(str[cmppos]) != pattern[pindx] )
			  {
                 endpos += d[ ToLowerCase(str[endpos]) ];
                 break;  
              }          

           if ( pindx < 0 ) return ( endpos - plen);
        }
        return( -1 );       
    }

    void compilePatternBM(unsigned char *ptrn)
	{
        int c;

        pattern = ptrn; 
		plen = strlen((char*)ptrn);

        for(c = 0; c < 256; c++)
                d[c] = plen;
		plen--;
        for(c = 0; c < plen; c++)
		{
                d[ pattern[c] |= 0x20 ] = plen - c;
		}
		pattern[plen] |= 0x20;
    }


public:
	void Init(char* szStr)
	{
		compilePatternBM((unsigned char*)szStr);
	}
	bool Search(const char* szStr)
	{
		return indexBM((const unsigned char*)szStr) >= 0;
	}
};


HINSTANCE g_hInstance;


void CenterWindow(HWND hWnd)
{
	CWindow(hWnd).CenterWindow();
}

const char g_szStatic[] = "Static";

void InsertFrame(HWND hWnd)
{
	RECT rect;
	GetClientRect(hWnd, &rect);

	CreateWindow(g_szStatic, NULL, 
		WS_VISIBLE | WS_CHILD | SS_BLACKFRAME,
		rect.left, rect.top, 
		rect.right - rect.left, 
		rect.bottom - rect.top,
		hWnd, NULL, g_hInstance, NULL);
}


class wColl
{
public:
	wColl() 
	{
		items=NULL;count=limit=0;

		GetExeDir(filename);
		strcat(filename, "DELLIST.TMP");
	}
	~wColl() {count=0;setlimit(0);}
	int load();
	void save();
	void free() {count=0;setlimit(0);}
	void erase() {DeleteFile(filename);count=0;setlimit(0);}
	Ident at(Ident n) {return items[n];}
	int getcount() {return count;}
	BOOL search(Ident n,BYTE trig);
private:
	Ident *items;
	int count;
	int limit;
	char filename[_MAX_PATH];
	void setlimit(int aLimit);
};

int wColl::load()
{
	int f=_lopen(filename,OF_READ|OF_SHARE_EXCLUSIVE);
	if (f==-1)
		return 0;
	long l=_llseek(f,0,2); 
	_llseek(f,0,0);
	if (l==-1)
		return 0;
	count=l>>1;
	setlimit(count);
	_lread(f, items, count * sizeof(Ident));
	_lclose(f);
	return count;
}

void wColl::save()
{
	int f=_lcreat(filename,0);
	_lwrite(f, (LPCSTR)items, count * sizeof(Ident));
	_lclose(f);
}

BOOL wColl::search(Ident n,BYTE trig)
{
	int l = 0;
	int h = count - 1;
	BOOL res = FALSE;
	while( l <= h )
	{
		int i = (l +  h) >> 1;
		if ( n < items[i] )
			l = i + 1;
		else
			if ( n == items[i] ) {l = i; res = TRUE; break; }
			else h = i - 1;
	}
	if(trig) 
		if(res) 
		{
			count--;
			memmove( &items[l], &items[l+1], (count-l)*sizeof(Ident) );
		}
		else 
		{
			if ( count == limit ) setlimit(count + 100);
			memmove( &items[l+1], &items[l], (count-l)*sizeof(Ident) );
			count++;
			items[l] = n;
		}
	return res;
}

void wColl::setlimit(int aLimit)
{
	if ( aLimit < count )
		aLimit =  count;
	if ( aLimit > maxCollectionSize)
		aLimit = maxCollectionSize;
	if ( aLimit != limit )
	{
		Ident *aItems;
		if (aLimit == 0 )
			aItems = 0;
		else
		{
			aItems = new(nothrow) Ident[aLimit];
			if ( count !=  0 )
				memcpy( aItems, items, count*sizeof(Ident) );
		}
		delete[] items;
		items =  aItems;
		limit =  aLimit;
	}
}

struct SCROLLKEYS
{
	BYTE bVirtkey;
	WORD wMessage;
	WORD wRequest;
};

const SCROLLKEYS key2scroll [] =
{
	VK_ESCAPE,WM_SYSCOMMAND,SC_MINIMIZE,
	VK_TAB,   WM_COMMAND, IDM_CHANGE,
	VK_INSERT,WM_COMMAND, IDM_TAG,
	VK_F4,    WM_COMMAND, IDM_INSERT,
	VK_F7,    WM_COMMAND, IDM_SEARCH,
	VK_F8,    WM_COMMAND, IDM_DELETE,
	VK_HOME,  WM_VSCROLL, SB_TOP,
	VK_PRIOR, WM_VSCROLL, SB_PAGEUP,
	VK_NEXT,  WM_VSCROLL, SB_PAGEDOWN,
	VK_UP,    WM_VSCROLL, SB_LINEUP,
	VK_DOWN,  WM_VSCROLL, SB_LINEDOWN,
	219,      WM_KEYDOWN, 245,
	221,      WM_KEYDOWN, 250,
	186,      WM_KEYDOWN, 230,
	222,      WM_KEYDOWN, 253,
	188,      WM_KEYDOWN, 225,
	190,      WM_KEYDOWN, 254
};

enum { NUMKEYS = sizeof key2scroll / sizeof key2scroll[0] };


BOOL g_bCyrillic, g_bUseHook, g_bCapitalLetters, 
	g_bSoftGray, g_bMinimizeToTray;

int g_nTranslationBalloon;

HHOOK g_hhook;

const BYTE xlat[]={0xD4,0xC8,0xD1,0xC2,0xD3,0xC0,0xCF,0xD0,
	0xD8,0xCE,0xCB,0xC4,0xDC,0xD2,0xD9,
	0xC7,0xC9,0xCA,0xDB,0xC5,0xC3,0xCC,0xD6,0xD7,0xCD,0xDF};

LRESULT CALLBACK MsgHookProc(int code,WPARAM wParam,LPARAM lParam)
{
	if(code<0)
		return CallNextHookEx(g_hhook,code,wParam,lParam);
	if(code==MSGF_DIALOGBOX && g_bCyrillic 
	    && ((LPMSG)lParam)->message==WM_KEYDOWN) 
	{
		WPARAM wMsgParam=((LPMSG)lParam)->wParam;
		BOOL bChange=TRUE;
		if(wMsgParam>0x40 && wMsgParam<0x5B)
			wMsgParam=xlat[wMsgParam-0x41];
		else switch(wMsgParam) 
		{
		case 219:      
			wMsgParam= 213; 
			break;
		case 221:      
			wMsgParam= 218; 
			break;
		case 186:      
			wMsgParam= 198; 
			break;
		case 222:      
			wMsgParam= 221; 
			break;
		case 188:      
			wMsgParam= 193; 
			break;
		case 190:      
			wMsgParam= 222; 
			break;
		case 0xBF:     
			wMsgParam=(GetKeyState(VK_SHIFT) & 0x8000)? ',': '.';
			break;
		default:       
			bChange= FALSE;
		}
		if(bChange) 
		{
			if(wMsgParam > 0x40 
			    && !(GetKeyState(VK_CAPITAL) & 1 ^ (WORD)GetKeyState(VK_SHIFT) >> 15))
				wMsgParam+= 0x20;
			SendMessage(((LPMSG)lParam)->hwnd, WM_CHAR, wMsgParam, 
				((LPMSG)lParam)->lParam);
			return 1;
		}
	}
	return 0;
}


long CALLBACK WndProc( HWND hWnd, UINT iMessage,
	WPARAM wParam, LPARAM lParam );  


enum 
{ 
	INTERLIN = 16,
	WIDTH	= 600
};


typedef char search_buf[BUFFER_SIZE];
typedef search_buf *aoutstr;


LOGFONT g_lf;

HFONT g_hFont;

search_buf g_szSearchStr;

INT_PTR CALLBACK SearchStrDlgProc(HWND hDlg, UINT iMessage,
WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
	case WM_INITDIALOG:
		CenterWindow(hDlg);
		InsertFrame(hDlg);
		SendDlgItemMessage(hDlg, IDD_SEARCHSTR, EM_LIMITTEXT, BUFFER_MAX, 0L);
		if (g_hFont) 
			SendDlgItemMessage(hDlg, IDD_SEARCHSTR, WM_SETFONT,
			(WPARAM) g_hFont, 0);
		SetDlgItemText(hDlg, IDD_SEARCHSTR, "");
		return(TRUE);
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDD_SEARCHSTR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				EnableWindow(GetDlgItem(hDlg, IDOK),
					SendMessage((HWND)lParam, WM_GETTEXTLENGTH, 0, 0L));
			}
			break;
		case IDOK:
			GetDlgItemText(hDlg, IDD_SEARCHSTR, g_szSearchStr, BUFFER_MAX);
			EndDialog(hDlg, TRUE);
			break;
		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;
		default:
			return(FALSE);
		}
		break;
	default:
		return(FALSE);
	}
	return(TRUE);
}

search_buf g_szLeftStr, g_szRightStr;

BOOL g_bLeftCyrillic;

INT_PTR CALLBACK InsertDlgProc(HWND hDlg, UINT iMessage,
WPARAM wParam, LPARAM)
{             
	switch (iMessage)
	{
	case WM_INITDIALOG:
		CenterWindow(hDlg);
		InsertFrame(hDlg);
		SendDlgItemMessage(hDlg, IDD_LEFTSTR, EM_LIMITTEXT, BUFFER_MAX, 0L);
		SendDlgItemMessage(hDlg, IDD_RIGHTSTR, EM_LIMITTEXT, BUFFER_MAX, 0L);
		if (g_hFont) 
		{
			SendDlgItemMessage(hDlg, IDD_LEFTSTR, WM_SETFONT,
				(WPARAM) g_hFont, 0);
			SendDlgItemMessage(hDlg, IDD_RIGHTSTR, WM_SETFONT,
				(WPARAM) g_hFont, 0);
		}

		SetDlgItemText(hDlg, IDD_LEFTSTR, g_szLeftStr);
		SetDlgItemText(hDlg, IDD_RIGHTSTR, g_szRightStr);
		return(TRUE);
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDD_LEFTSTR:
		case IDD_RIGHTSTR:
			g_bCyrillic = g_bLeftCyrillic ^ (LOWORD(wParam) == IDD_RIGHTSTR);
			if (HIWORD(wParam) == EN_CHANGE)
			{
				BOOL valid = 
					SendMessage(GetDlgItem(hDlg,IDD_LEFTSTR),
						WM_GETTEXTLENGTH, 0, 0) 
					&& SendMessage(GetDlgItem(hDlg,IDD_RIGHTSTR),
						WM_GETTEXTLENGTH, 0, 0);
				EnableWindow(GetDlgItem(hDlg, IDOK), valid);
				EnableWindow(GetDlgItem(hDlg, ID_APPLY), valid);
			}
			break;
		case IDOK:
		case ID_APPLY:
			GetDlgItemText(hDlg, IDD_LEFTSTR, g_szLeftStr, BUFFER_MAX);
			GetDlgItemText(hDlg, IDD_RIGHTSTR, g_szRightStr, BUFFER_MAX);
			EndDialog(hDlg, LOWORD(wParam));
			break;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			break;
		default:
			return(FALSE);
		}
		break;
	default:
		return(FALSE);
	}
	return(TRUE);
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			CenterWindow(hDlg);
			InsertFrame(hDlg);
			return TRUE;
		}

	case WM_COMMAND:
		{
			switch (wParam)
			{
			case IDOK:
			case IDCANCEL:
				EndDialog(hDlg, TRUE);
				return(TRUE);

			default:
				return(TRUE);
			}
		}
	case WM_LBUTTONDBLCLK:
		EndDialog(hDlg, TRUE);
		return(TRUE);
	}
	return(FALSE);
}

const char szFontSection[] = "Font";
const char szSettingsSection[] = "Settings";

const char szHeight[]	= "Height";
const char szWeight[]	= "Weight";
const char szFaceName[] = "FaceName";
const char szCharSet[]	= "CharSet";
const char szSystem[]	= "System";

const char szNumRows[]	= "NumRows";
const char szDefNumRows[]	= "10";
const char szUseHook[]	= "UseHook";
const char szCapitalLetters[]	= "CapitalLetters";
const char szSoftGray[]	= "SoftGray";
const char szTranslationBalloon[] = "TranslationBalloon";
const char szMinimizeToTray[] = "MinimizeToTray";

const char szEngFName[] = "dct.dbe";
const char szRusFName[] = "dct.dbr";

const int g_arrTranslationOptions[] = 
{ 
	IDC_TRANSLATION_IE, 
	IDC_TRANSLATION_FIREFOX, 
	IDC_TRANSLATION_ACROBAT,
	IDC_TRANSLATION_MS_WORD,
	IDC_TRANSLATION_CHROME,
};

const char* GetIniFilename()
{
    static char iniFilename[_MAX_PATH];
    if (!iniFilename[0])
    {
        GetExeDir(iniFilename);
        strcat(iniFilename, "dictionary.ini");
    }

    return iniFilename;
}

BOOL WriteProfileInt(const char* lpAppName,  // section name
					const char* lpKeyName,  // key name
					int nValue)   // value to add
{
	char szBuf[18];
	itoa(nValue, szBuf, 10);
    return WritePrivateProfileString(lpAppName, lpKeyName, szBuf, GetIniFilename());
}


INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{  
	char szBuf[18];
	switch (iMessage)
	{
	case WM_INITDIALOG:
		{
			CenterWindow(hDlg);
			InsertFrame(hDlg);
			SendDlgItemMessage(hDlg, IDC_NUM_ROWS, EM_LIMITTEXT, 2, 0);
			SendDlgItemMessage(hDlg, IDC_SPIN1, UDM_SETRANGE, 0, (LPARAM) MAKELONG(99, 2)); 
			GetPrivateProfileString(szSettingsSection, szNumRows, 
                szDefNumRows, szBuf, 3, GetIniFilename());
			SetDlgItemText(hDlg, IDC_NUM_ROWS, szBuf);
			SendDlgItemMessage(hDlg, IDC_USE_HOOK, BM_SETCHECK, g_bUseHook, 0);
			SendDlgItemMessage(hDlg, IDC_CAPITAL_LETTERS, BM_SETCHECK, g_bCapitalLetters, 0);
			SendDlgItemMessage(hDlg, IDC_SOFT_GRAY, BM_SETCHECK, g_bSoftGray, 0);

			for (int i = 0
					; i < sizeof(g_arrTranslationOptions) / sizeof(g_arrTranslationOptions[0])
					; ++i)
				if (IsOleAccAvailable())
					SendDlgItemMessage(hDlg, g_arrTranslationOptions[i], BM_SETCHECK
							, g_nTranslationBalloon & (1 << i), 0);
				else
					::EnableWindow(GetDlgItem(hDlg, g_arrTranslationOptions[i]), FALSE);

			SendDlgItemMessage(hDlg, IDC_MINIMIZE_TO_TRAY, BM_SETCHECK, g_bMinimizeToTray, 0);

			return TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_NUM_ROWS:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				GetDlgItemText(hDlg, IDC_NUM_ROWS, szBuf, 3);
				EnableWindow(GetDlgItem(hDlg, IDOK), 
					szBuf[0] != 0 && !(szBuf[1] == 0 && (szBuf[0] == '0' || szBuf[0] == '1')));
			}
			break;
		case IDOK:
			GetDlgItemText(hDlg, IDC_NUM_ROWS, szBuf, 3);
            WritePrivateProfileString(szSettingsSection, szNumRows, szBuf, GetIniFilename());
			g_bUseHook = SendDlgItemMessage(hDlg, IDC_USE_HOOK, BM_GETCHECK, 0, 0);
			WriteProfileInt(szSettingsSection, szUseHook, !!g_bUseHook);

			g_bCapitalLetters = SendDlgItemMessage(hDlg, IDC_CAPITAL_LETTERS, BM_GETCHECK, 0, 0);
			WriteProfileInt(szSettingsSection, szCapitalLetters, !!g_bCapitalLetters);

			if (IsOleAccAvailable())
			{
				g_nTranslationBalloon = 0;
				for (int i = 0
						; i < sizeof(g_arrTranslationOptions) / sizeof(g_arrTranslationOptions[0])
						; ++i)
					g_nTranslationBalloon |= 
						((BST_CHECKED == SendDlgItemMessage(
							hDlg, g_arrTranslationOptions[i], BM_GETCHECK, 0, 0)) 
						<< i);

				WriteProfileInt(szSettingsSection, szTranslationBalloon, g_nTranslationBalloon);
			}

			g_bMinimizeToTray = SendDlgItemMessage(hDlg, IDC_MINIMIZE_TO_TRAY, BM_GETCHECK, 0, 0);
			WriteProfileInt(szSettingsSection, szMinimizeToTray, !!g_bMinimizeToTray);

			EndDialog(hDlg, TRUE);
			break;
		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;
		default:
			return(FALSE);
		}
		break;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            // If the notification came from the syslink control
            if (pnmh->idFrom == IDC_IAccessible2) {
                // NM_CLICK is the notification is normally used.
                // NM_RETURN is the notification needed for return keypress, otherwise the control is not keyboard accessible.
                if ((pnmh->code == NM_CLICK) || (pnmh->code == NM_RETURN)) {
                    // Recast lParam to an NMLINK item because it also contains NMHDR as part of its structure
                    PNMLINK link = (PNMLINK)lParam;
                    ShellExecuteW(NULL, L"open", link->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
                }
            }
        }
    default:
		return(FALSE);
	}
	return(TRUE);
}


UINT_PTR CALLBACK CFHookProc(
  HWND hDlg,      // handle to dialog box
  UINT uiMsg,     // message identifier
  WPARAM //wParam,  // message parameter
  ,LPARAM //lParam   // message parameter
)
{
	if (uiMsg == WM_INITDIALOG)
	{
		CenterWindow(hDlg);
		InsertFrame(hDlg);
		return(TRUE);
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////

static NOTIFYICONDATA g_tnd;

void LoadToTray( HWND    hWnd, 
	  	        UINT	  uCallbackMessage,
				LPCTSTR sTip, // the text for a standard ToolTip. 
				HICON	  icon )
{
	g_tnd.cbSize		      = sizeof( NOTIFYICONDATA );
	g_tnd.hWnd			  = hWnd;
	g_tnd.uID			  = 0;
	
	g_tnd.uFlags		      = NIF_ICON | NIF_TIP | NIF_MESSAGE;// | NIF_INFO;

	g_tnd.uCallbackMessage = uCallbackMessage;  
	g_tnd.hIcon		   	  = icon;

	strcpy(g_tnd.szTip, sTip);

	Shell_NotifyIcon( NIM_ADD, &g_tnd ); // add to the taskbar's status area
}

/////////////////////////////////////////////////////////////////////////

#define WM_TRAY_NOTIFY (WM_APP + 1000)

enum { FLAG_IDS_REORDERED = 1 };

class MainWindow 
: public CWindowImpl<MainWindow>
{
	enum { IDEVT_TRANSLATION_BALLOON = 1 };

public:
	static const char szClassName[];
	static CWndClassInfo& GetWndClassInfo()
	{
      // a manual DECLARE_WND_CLASS macro expansion
      static CWndClassInfo wc =
      {
         { 
			 sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, 
            StartWindowProc,
            0, 0, NULL, 
			LoadIcon(g_hInstance, szClassName), 
			NULL, 
			(HBRUSH)(COLOR_WINDOW + 1), 
			szClassName, 
            szClassName, 
			NULL 
		 },
         NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
      };
      return wc;
	}

	MainWindow();
	~MainWindow() 
	{
		if (g_hFont)
		{
			DeleteObject(g_hFont);
			g_hFont = NULL;
		}

		delete m_pleftd;
		delete m_prightd;
		delete[] m_st_left;
		delete[] m_st_right;
		delete[] m_id;
	}

   BEGIN_MSG_MAP( MainWindow )
      MESSAGE_HANDLER( WM_PAINT, OnPaint )
      MESSAGE_HANDLER( WM_DESTROY, OnDestroy )
      MESSAGE_HANDLER( WM_VSCROLL, OnVScroll )
	  MESSAGE_HANDLER( WM_MOUSEWHEEL, OnMouseWheel )
      MESSAGE_HANDLER( WM_SYSCOMMAND, OnSysCommand )
      MESSAGE_HANDLER( WM_KEYDOWN, OnKeyDown )
      MESSAGE_HANDLER( WM_SETCURSOR, OnSetCursor )
      MESSAGE_HANDLER( WM_NCMOUSEMOVE, OnNcMouseMove )
      MESSAGE_HANDLER( WM_MOUSEMOVE, OnMouseMove )
      MESSAGE_HANDLER( WM_LBUTTONUP, OnLButtonUp )
      MESSAGE_HANDLER( WM_LBUTTONDOWN, OnLButtonDown )
      MESSAGE_HANDLER( WM_TIMER, OnTimer )
      MESSAGE_HANDLER( WM_TRAY_NOTIFY, OnTrayNotify )

	  COMMAND_ID_HANDLER_NO_PARAMS(IDM_CHANGE, change)
	  COMMAND_ID_HANDLER_NO_PARAMS(IDM_TAG, tag)
	  COMMAND_ID_HANDLER_NO_PARAMS(IDM_DELETE, delitems)
	  COMMAND_ID_HANDLER_NO_PARAMS(IDM_COMPRESS, compress)
	  COMMAND_ID_HANDLER_NO_PARAMS(IDM_IMPORT, import)
	  COMMAND_ID_HANDLER_NO_PARAMS(IDM_EXPORT, doExport)
	  COMMAND_ID_HANDLER_NO_PARAMS(IDM_INSERT, insert)
	  COMMAND_ID_HANDLER_NO_PARAMS(IDM_SEARCH, OnSearch)
	  COMMAND_ID_HANDLER_NO_PARAMS(IDM_SETTINGS, OnSettings)
	  COMMAND_ID_HANDLER_NO_PARAMS(IDM_FONT, OnFont)
	  COMMAND_ID_HANDLER_NO_PARAMS(IDM_CONTENTS_MODE, OnContentsMode)
	  COMMAND_ID_HANDLER_NO_PARAMS(IDM_SEARCH_MODE, OnSearchMode)
   END_MSG_MAP()

	void rollback(mydict* pdict, int size);
	bool FixReorderedIds(mydict* pleftd, mydict* prightd);

	long WndProc( UINT iMessage, WPARAM wParam, LPARAM lParam );

	LRESULT OnPaint( UINT, WPARAM, LPARAM, BOOL& );

	LRESULT OnDestroy( UINT, WPARAM, LPARAM, BOOL& )
	{
	  PostQuitMessage( 0 );
	  return 0;
	}

	LRESULT OnVScroll( UINT, WPARAM wParam, LPARAM, BOOL& )
	{
		switch (LOWORD(wParam))
		{
		case SB_TOP:
			putfirst("");
			break;

		case SB_LINEUP:
			showbar();
			m_ip--;
			showbar();
			break;

		case SB_LINEDOWN:
			showbar();
			m_ip++;
			showbar();
			break;

		case SB_PAGEUP:
			putprev();
			break;

		case SB_PAGEDOWN:
			putnext();
			break;

		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			putfirstbypos(HIWORD(wParam));
			break;
		default:
			return 0;
		}
		ResetPage();
		return 0;
	}

	LRESULT OnMouseWheel( UINT, WPARAM wParam, LPARAM, BOOL& bHandled)
	{
		short fwKeys = LOWORD(wParam);
		// we don't handle anything but scrolling just now
		if (fwKeys & (MK_SHIFT | MK_CONTROL))
		{
			bHandled = FALSE; // alternatively: DefWindowProc()
			return 0;
		}

		short zDelta = HIWORD(wParam);
		int nDisplacement = -zDelta / WHEEL_DELTA;
		if (nDisplacement != 0)
		{
			if (nDisplacement < -m_NumRows)
				nDisplacement = -m_NumRows;
			else if (nDisplacement > m_NumRows)
				nDisplacement = m_NumRows;

			showbar();
			m_ip += nDisplacement;
			showbar();
		}
		return 0;
	}

	LRESULT OnSysCommand( UINT, WPARAM wParam, LPARAM, BOOL& bHandled)
	{
		switch (wParam & 0xFFF0)
		{
		case IDM_ABOUT:
			{
				DialogBox(g_hInstance, MAKEINTRESOURCE(ABOUT), m_hWnd, About);
				return 0;
			}
		case SC_MINIMIZE:
			if (g_bMinimizeToTray)
			{
				ShowWindow(SW_HIDE);
				LoadToTray(m_hWnd, WM_TRAY_NOTIFY, szClassName, LoadIcon(g_hInstance, szClassName));
				return 0;
			}
			break;
		}
		bHandled = FALSE; // alternatively: DefWindowProc()
		return 0;
	}

	LRESULT OnTrayNotify( UINT, WPARAM, LPARAM lParam, BOOL& bHandled)
	{
		if (WM_LBUTTONUP == lParam)
		{
			Shell_NotifyIcon( NIM_DELETE, &g_tnd ); // delete from the status area
			ShowWindow( SW_SHOW );
			SetForegroundWindow(*this);
			return 0;
		}
		bHandled = FALSE; // alternatively: DefWindowProc()
		return 0;
	}

	LRESULT OnKeyDown( UINT, WPARAM wParam, LPARAM, BOOL& )
	{
		if(wParam==VK_BACK) 
		{
			int l = strlen(m_szbuf);
			if(l) 
			{
				m_szbuf[l-1] = '\0'; 
				putfirst(m_szbuf);
			}
		}
		else if ((wParam==VK_SPACE)||
		    (wParam>='A')&&(wParam<='Z')||
		    (wParam>=224)&&(wParam<=255)) 
		{
			int l = strlen(m_szbuf);
			if(l<BUFFER_MAX) 
			{
				if(g_bUseHook && m_pleftd->IsCyrillic()
						&& wParam>0x40 && wParam<0x5B)
					wParam=xlat[wParam-0x41];

				m_szbuf[l] = (char)wParam;
				m_szbuf[l+1]='\0';
				putfirst(m_szbuf);
			}
		}
		else if (wParam==VK_TAB && (GetKeyState(VK_CONTROL) & 0x8000))
		{
			if (m_filterMode)
				OnContentsMode();
			else
				OnSearchMode();
		}
		else for (int i = 0; i < NUMKEYS; i++)
		{
			if (wParam == key2scroll[i].bVirtkey)
			{
				SendMessage(key2scroll[i].wMessage, key2scroll[i].wRequest, 0);
				break;
			}
		}
		return 0;
	}
	LRESULT OnSetCursor( UINT, WPARAM, LPARAM, BOOL& bHandled)
	{
		if (m_onSeparator)
		{
			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		}
		else
		{
			bHandled = FALSE; // alternatively: DefWindowProc()
		}
		return 0;
	}
	LRESULT OnNcMouseMove( UINT, WPARAM, LPARAM, BOOL& )
	{
		if (m_onSeparator)
		{
			m_onSeparator = false;
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}
		return 0;
	}
	LRESULT OnMouseMove( UINT, WPARAM, LPARAM lParam, BOOL& )
	{
		if (m_captured)
		{
			int x = short(lParam & 0xFFFF);
			if (x < INTERLIN)
				x = INTERLIN;
			else if (x > WIDTH - INTERLIN)
				x = WIDTH - INTERLIN;
			if (x != m_splitterPos)
			{
				HDC dc = GetDC();
				RECT rect;
				GetClientRect(&rect);
				int iOldROP = SetROP2(dc, R2_NOTXORPEN);
				MoveToEx(dc, m_splitterPos, rect.top, NULL);
				LineTo(dc, m_splitterPos, rect.bottom);
				m_splitterPos = x;
				MoveToEx(dc, m_splitterPos, rect.top, NULL);
				LineTo(dc, m_splitterPos, rect.bottom);
				SetROP2(dc, iOldROP);
				ReleaseDC( dc );
			}
		}
		else
		{
			bool onSeparator = (abs((lParam & 0xFFFF) - m_splitterPos) < 3);
			if (onSeparator && !m_onSeparator)
			{
				m_onSeparator = true;
				SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			}
			else if (!onSeparator && m_onSeparator)
			{
				m_onSeparator = false;
				SetCursor(LoadCursor(NULL, IDC_ARROW));
			}
		}
		return 0;
	}

	LRESULT OnLButtonUp( UINT, WPARAM, LPARAM, BOOL& )
	{
		ResetPage();
		if (m_captured)
		{
			ReleaseCapture();
			m_captured = false;
			Invalidate();
		}
		return 0;
	}

	LRESULT OnLButtonDown( UINT, WPARAM, LPARAM lParam, BOOL& )
	{
		ResetPage();
		if (m_onSeparator)
		{
			SetCapture();
			m_captured = true;
			HDC dc = GetDC();
			RECT rect;
			GetClientRect(&rect);
			int iOldROP = SetROP2(dc, R2_NOTXORPEN);
			MoveToEx(dc, m_splitterPos, rect.top, NULL);
			LineTo(dc, m_splitterPos, rect.bottom);
			SetROP2(dc, iOldROP);
			ReleaseDC( dc );
		}
		else
		{
			int y = lParam >> 20;
			if(y == m_ip) 
				tag();
			else if(y < m_bufptr) 
			{
				showbar();
				m_ip = y;
				showbar();
			}
		}
		return 0;
	}

	LRESULT OnTimer( UINT, WPARAM, LPARAM lParam, BOOL& );
	

	void OnSearch()
	{
		if(SearchDlg())
		{
			HCURSOR hCursorOld = NULL;
			if (m_filterMode)
				hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
			putfirst(m_filterMode? "" : g_szSearchStr);
			if (m_filterMode)
				SetCursor(hCursorOld);
		}
	}

	void OnSettings()
	{
		BOOL bCapitalLetters = g_bCapitalLetters;
		BOOL bSoftGray = g_bSoftGray;
		int nTranslationBalloon = g_nTranslationBalloon;
		DialogBox(g_hInstance, MAKEINTRESOURCE(SETTINGS), m_hWnd, SettingsDlgProc);
		if (bCapitalLetters != g_bCapitalLetters)
			reread();
		else if (bSoftGray != g_bSoftGray)
			Invalidate();

		if (g_nTranslationBalloon && !nTranslationBalloon)
			SetTimer(IDEVT_TRANSLATION_BALLOON, 700);
		else if (!g_nTranslationBalloon && nTranslationBalloon)
			KillTimer(IDEVT_TRANSLATION_BALLOON);
	}

	void OnFont()
	{                                
		int iHeight = 20 * 8 / HIWORD(GetDialogBaseUnits());
		CHOOSEFONT cf = {0};
		cf.lStructSize = sizeof(CHOOSEFONT);
		cf.hwndOwner = m_hWnd;
		cf.lpLogFont = &g_lf;
		cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_LIMITSIZE | CF_ENABLEHOOK;
		cf.nSizeMin = iHeight;
		cf.nSizeMax = iHeight + 2;
		cf.lpfnHook = CFHookProc;
		HFONT hFont;
		if(ChooseFont(&cf) && (hFont = CreateFontIndirect(&g_lf)) != NULL)
		{
			WriteProfileInt(szFontSection, szHeight, g_lf.lfHeight);
			WriteProfileInt(szFontSection, szWeight, g_lf.lfWeight);
			WritePrivateProfileString(szFontSection, szFaceName, g_lf.lfFaceName, 
                GetIniFilename());
			WriteProfileInt(szFontSection, szCharSet, g_lf.lfCharSet);

			if (g_hFont)
				DeleteObject(g_hFont);
			g_hFont = hFont;

			Invalidate();
		}
	}

	void OnContentsMode()
	{
		if (m_filterMode)
		{
			SetFilterMode(false);
			putfirst("");
		}
	}

	void OnSearchMode()
	{
		if (!m_filterMode && (g_szSearchStr[0] != 0 || SearchDlg()))
		{
			HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
			SetFilterMode(true);
			putfirst("");
			SetCursor(hCursorOld);
		}
	}

	void showbar();
	BOOL putnext();
	void reread();
	void putprev();
	void putfirst(char *s);
	void putfirstbypos(int pos);
	void change();
	void delitems();
	void insert();
	void tag();
	void compress();
	void import();
    void doExport();

	void ResetPage() 
	{
		m_szbuf[0] = 0;
	}

private:
	const int m_NumRows;
	mydict *m_pleftd,*m_prightd;
	wColl m_wcoll;

	int m_strsamount;
	int m_topCurp, m_topRptr;
	int m_botCurp, m_botRptr;
	int m_bufptr, m_ip;

	Ident m_ident;
	mybuf m_a;
	asubident m_as;
	Ident *m_id;
	aoutstr m_st_left, m_st_right;
	search_buf m_szbuf;
	HMENU m_popup;

	bool m_filterMode;
	bool m_captured;
	bool m_onSeparator;
	int m_splitterPos;

    IRenderer* m_pRenderer;

	BOOL SearchDlg();
	void SetFilterMode(bool filter = false);

	const MainWindow& operator = (const MainWindow& other);
};

const char MainWindow::szClassName[] = "Dictionary";

const char szAbout[]	= "&About";

MainWindow::MainWindow()
    : m_NumRows(GetPrivateProfileInt(szSettingsSection, szNumRows, 10, GetIniFilename()))
{
	m_ident = 0;
	m_filterMode = false;
	m_captured = false;
	m_onSeparator = false;
	m_splitterPos = WIDTH / 2;
	m_bufptr = 0;

    m_pRenderer = NULL;

	RECT rcPos = { 
	   CW_USEDEFAULT,
		0,
		CW_USEDEFAULT + WIDTH + GetSystemMetrics(SM_CXFIXEDFRAME) * 2 + GetSystemMetrics(SM_CXVSCROLL),
		INTERLIN * m_NumRows + 7 + GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + 
			 GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYMENU)
	};
	CWindowImpl<MainWindow>::Create( NULL, rcPos, szClassName,
      WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_VSCROLL);

    // Fix size
    RECT clientRect;
    GetClientRect(&clientRect);
    RECT windowRect;
    GetWindowRect(&windowRect);
    SetWindowPos(
        NULL,
        0,
        0,
        (windowRect.right - windowRect.left) + WIDTH - (clientRect.right - clientRect.left),
        (windowRect.bottom - windowRect.top) + INTERLIN * m_NumRows + 7 - (clientRect.bottom - clientRect.top),
        SWP_NOMOVE | SWP_NOZORDER);

	m_st_left = new(nothrow) search_buf[m_NumRows];
	m_st_right = new(nothrow) search_buf[m_NumRows];
	m_id = new(nothrow) Ident[m_NumRows];

	m_pleftd = new(nothrow) myengdict(szEngFName);
	m_prightd = new(nothrow) myrusdict(szRusFName);

	Ident left_ident = m_pleftd->medi_load();
	Ident right_ident = m_prightd->medi_load();
	if(left_ident==right_ident) 
	{
		m_ident=left_ident;
		if (!FixReorderedIds(m_pleftd, m_prightd))
			FixReorderedIds(m_prightd, m_pleftd);
	}
	else 
	{
		if(left_ident>right_ident) 
		{
			m_ident=left_ident;
			rollback(m_pleftd, left_ident-right_ident);
		}
		else 
		{
			m_ident = right_ident;
			rollback(m_prightd, right_ident-left_ident);
		}
	}
	
	HMENU hSystemMenu = GetSystemMenu(FALSE);  // reset option
	AppendMenu(hSystemMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(hSystemMenu, MF_STRING, IDM_ABOUT, szAbout);

	m_popup = GetSubMenu(GetMenu(), 0);
    g_lf.lfHeight = GetPrivateProfileInt(szFontSection, szHeight, 14, GetIniFilename());
    g_lf.lfWeight = GetPrivateProfileInt(szFontSection, szWeight, 700, GetIniFilename());
	g_lf.lfPitchAndFamily = 2;
    GetPrivateProfileString(szFontSection, szFaceName, szSystem, g_lf.lfFaceName, LF_FACESIZE, GetIniFilename());
    g_lf.lfCharSet = (BYTE)GetPrivateProfileInt(szFontSection, szCharSet, RUSSIAN_CHARSET, GetIniFilename());

	if (!g_hFont)
		g_hFont = CreateFontIndirect(&g_lf);

    g_bUseHook = GetPrivateProfileInt(szSettingsSection, szUseHook, TRUE, GetIniFilename());
    g_bCapitalLetters = GetPrivateProfileInt(szSettingsSection, szCapitalLetters, FALSE, GetIniFilename());
    g_bSoftGray = GetPrivateProfileInt(szSettingsSection, szSoftGray, TRUE, GetIniFilename());
	if (IsOleAccAvailable())
	{
        g_nTranslationBalloon = GetPrivateProfileInt(szSettingsSection, szTranslationBalloon, 0, GetIniFilename());
		if (g_nTranslationBalloon)
			SetTimer(IDEVT_TRANSLATION_BALLOON, 700);
	}
    g_bMinimizeToTray = GetPrivateProfileInt(szSettingsSection, szMinimizeToTray, FALSE, GetIniFilename());

	ResetPage();
	putfirst("");
}

void MainWindow::rollback(mydict *pdict, int size)
{
	int i;
	pdict->medi2memo();
	if(m_wcoll.load()) 
	{
		if(m_wcoll.getcount()!=size) 
			exit(EXIT_FAILURE);
		for(i=0; i<size; i++) 
		{
			Ident j = m_wcoll.at(i);
			pdict->deletebyident(j);
			m_ident--;
			pdict->correctident(m_ident, j);
		}
		m_wcoll.erase();
	}
	else 
		for(i=0;i<size;i++) 
			pdict->deletebyident(--m_ident);
	pdict->store();
	pdict->memo2medi();
}

bool MainWindow::FixReorderedIds(mydict* pleftd, mydict* prightd)
{
	if ((pleftd->getFlags() & FLAG_IDS_REORDERED)
		&& !(prightd->getFlags() & FLAG_IDS_REORDERED)// backward compatibility
		&& !prightd->is_sequential())
	{
        pleftd->free();

        prightd->medi2memo();
		prightd->store(0, true);
		prightd->free();

        pleftd->medi_load();
		prightd->medi_load();

		return true;
	}
	return false;
}

LRESULT MainWindow::OnPaint( UINT, WPARAM, LPARAM, BOOL& )
{
	int i;
	PAINTSTRUCT ps;
	SIZE size = {0};
	BeginPaint( &ps );

	RECT rect;
	GetClientRect(&rect);

	FrameRect(ps.hdc, &rect, (HBRUSH) GetStockObject(BLACK_BRUSH));

    if (m_pRenderer)
        m_pRenderer->Render(ps.hdc, rect);
    else
    {
	    if (g_bSoftGray)
	    {
		    RECT rect2;
		    rect2.left = rect.left + 1;
		    rect2.right = rect.right - 1;

		    HBRUSH hbr = CreateSolidBrush(0x00EEEEEE);
		    for (i = 1; i < m_NumRows; i += 2)
		    {
			    rect2.top = i*INTERLIN+3;
			    rect2.bottom = i*INTERLIN+17;
			    FillRect(ps.hdc, &rect2, hbr);
		    }
		    DeleteObject(hbr);
	    }

	    UINT uiAlignOld = SetTextAlign(ps.hdc, TA_LEFT | TA_BASELINE);
	    HPEN hOldpen = (HPEN)SelectObject(ps.hdc,CreatePen(PS_DOT, 1, 0));
	    HFONT hFontold = (HFONT)SelectObject(ps.hdc, g_hFont);
	    SetBkMode(ps.hdc, TRANSPARENT);
	    int len;
	    for (i=0; i<m_bufptr; i++) 
	    {
		    if ((i!=0) && (strcmp(m_st_left[i-1], m_st_left[i])==0)) 
		    {
			    MoveToEx(ps.hdc, 3, i*INTERLIN+11, NULL);
			    LineTo(ps.hdc, size.cx /*- 1*/, i*INTERLIN+11);
		    }
		    else 
		    {
			    len = strlen(m_st_left[i]);
			    while (GetTextExtentPoint32(ps.hdc, m_st_left[i], len, &size),
					    size.cx >= m_splitterPos)
				    --len;
			    TextOut(ps.hdc, 2, i*INTERLIN+15, m_st_left[i], len);
		    }
		    TextOut(ps.hdc, m_splitterPos, i*INTERLIN+15, m_st_right[i],strlen(m_st_right[i]));
	    }
	    DeleteObject(SelectObject(ps.hdc,hOldpen));
	    int iOldROP = SetROP2(ps.hdc, R2_NOTXORPEN);
	    for(i=0; i<m_bufptr; i++)
		    if (m_wcoll.search(m_id[i],FALSE)) 
		    {
			    MoveToEx(ps.hdc, 3, i*INTERLIN+18, NULL);
			    GetTextExtentPoint32(ps.hdc, m_st_right[i], strlen(m_st_right[i]), &size);
			    LineTo(ps.hdc, size.cx + m_splitterPos, i*INTERLIN+18);
		    }                                       
	    SetROP2(ps.hdc, iOldROP);
		SelectObject(ps.hdc,hFontold);
	    SetTextAlign(ps.hdc, uiAlignOld);

	    rect.left++;
	    rect.right--;
	    rect.top = m_ip*INTERLIN+3;
	    rect.bottom = m_ip*INTERLIN+17;
	    InvertRect(ps.hdc, &rect);
    }

    EndPaint( &ps );

	return 0;
}

inline bool HashAccept(const BYTE* pHashTable, mystring buf)
{
	for (BYTE* p = buf + buf[0]; p != buf; --p)
	{
		BYTE b = *p;
		if (pHashTable[b])
			return true;
	}
	return false;
}

BOOL MainWindow::putnext()
{
	int rptr = m_botRptr;
	if (m_botCurp != m_pleftd->getcurp())
		m_strsamount= m_pleftd->firstbypos(m_a, m_as, m_botCurp);

	if (m_strsamount==0) 
		return FALSE;

	int bufptr=0;
	SetScrollRange(SB_VERT, 0, m_pleftd->getCount(), FALSE);
	SetScrollPos(SB_VERT, m_pleftd->getcurp(), TRUE);

	hash_buf hash;
	bool hashCheck = false;

	CSubStringSearch* pSearch = NULL;
	if (m_filterMode)
	{
		pSearch = new(nothrow) CSubStringSearch;
		if (pSearch)
			pSearch->Init(g_szSearchStr);
		hashCheck = m_pleftd->MakeHash(g_szSearchStr, hash);
	}

	do 
	{
		bool found = true;
		for (int i = rptr-1; i>=0; i--) 
		{
			search_buf buf;
			if (pSearch)
			{
				if (!found && !m_pleftd->differs(i+1))
					continue;

				if (hashCheck && !HashAccept(hash, m_a[i]))
				{
					found = false;
					continue;
				}

				m_pleftd->expfunc(buf, m_a[i]);
				if (!pSearch->Search(buf))
				{
					found = false;
					continue;
				}
				found = true;
				strcpy(m_st_left[bufptr], buf);
			}
			else
				m_pleftd->expfunc(m_st_left[bufptr], m_a[i]);

			m_prightd->findbyident(m_st_right[bufptr], m_id[bufptr] = m_as[i]);
			if (0 == bufptr)
			{
				m_topCurp = m_botCurp;
				m_topRptr = m_botRptr;
			}
			bufptr++;
			if (bufptr == m_NumRows) 
			{
				rptr=i;
				goto loop_exit;
			}
		}
		rptr = m_strsamount = m_pleftd->findnextrec(m_a, m_as);
	} 
	while (m_strsamount != 0);
loop_exit:
	delete pSearch;

	m_botRptr = rptr;
	m_botCurp = m_pleftd->getcurp();

	if (0 != bufptr)
	{
		m_bufptr = bufptr;
		if (m_ip >= m_bufptr) 
			m_ip = (m_bufptr)? m_bufptr-1 : 0;
	}
	Invalidate();
	return m_bufptr != 0;
}

void MainWindow::reread()
{
	m_botCurp = m_topCurp;
	m_botRptr = m_topRptr;
	putnext();
}

void MainWindow::putprev()
{
	int rptr = m_topRptr;
	if (m_topCurp != m_pleftd->getcurp())
		m_strsamount= m_pleftd->firstbypos(m_a, m_as, m_topCurp);

	if (0 == m_topCurp 
	&& m_strsamount - m_topRptr <= m_NumRows)
	{
		if (m_strsamount == m_topRptr)
		{
			Invalidate();
			return;
		}
		m_botCurp = 0;
		m_botRptr = m_strsamount;
		putnext();
	}
	else 
	{
		int curp = m_topCurp;

		hash_buf hash;
		bool hashCheck = false;

		CSubStringSearch* pSearch = NULL;
		if (m_filterMode)
		{
			pSearch = new(nothrow) CSubStringSearch;
			if (pSearch)
				pSearch->Init(g_szSearchStr);
			hashCheck = m_pleftd->MakeHash(g_szSearchStr, hash);
		}

		int cnt = 0;
		while (m_strsamount != 0)
		{
			bool found = true;
			for (int i = rptr; i < m_strsamount; i++)
			{
				search_buf buf;
				if (pSearch)
				{
					if (!found && !m_pleftd->differs(i))
						continue;

					if (hashCheck && !HashAccept(hash, m_a[i]))
					{
						found = false;
						continue;
					}

					m_pleftd->expfunc(buf, m_a[i]);
					if (!pSearch->Search(buf))
					{
						found = false;
						continue;
					}
					found = true;
				}
				if (m_bufptr < m_NumRows)
					++m_bufptr;
				if (m_bufptr > 1)
				{
					memmove(m_st_left + 1, m_st_left, sizeof(m_st_left[0]) * (m_bufptr-1));
					memmove(m_st_right + 1, m_st_right, sizeof(m_st_right[0]) * (m_bufptr-1));
					memmove(m_id + 1, m_id, sizeof(m_id[0]) * (m_bufptr-1));
				}
				if (pSearch)
					strcpy(m_st_left[0], buf);
				else
					m_pleftd->expfunc(m_st_left[0], m_a[i]);

				m_prightd->findbyident(m_st_right[0], m_id[0] = m_as[i]);

				if (0 == cnt)
				{
					m_botCurp = m_topCurp;
					m_botRptr = m_topRptr;
				}
				if (++cnt == m_NumRows)
				{
					rptr = i+1;
					goto loop_exit;
				}
			}
			if (curp == 0)
			{
				rptr = m_strsamount;
				break;
			}
			m_strsamount = m_pleftd->firstbypos(m_a, m_as, --curp);
			rptr = 0;
		}

loop_exit:
		delete pSearch;

		m_topRptr = rptr;
		m_topCurp = curp;

		if (cnt)
		{
			SetScrollRange (SB_VERT, 0, m_pleftd->getCount(), FALSE);
			SetScrollPos(SB_VERT, m_pleftd->getcurp(), TRUE);
		}
		Invalidate();
	}
}

void MainWindow::showbar()
{
	if (m_ip < 0)  
	{ 
		m_ip = (m_bufptr)? m_bufptr-1: 0 ; 
		putprev(); 
		return; 
	}
	else if (m_ip >= m_bufptr) 
	{
		m_ip = 0;
		if(putnext()) 
			return;
		else 
			m_ip = (m_bufptr)? m_bufptr-1 : 0;
	}
	HDC DC = GetDC();
	RECT rect;
	GetClientRect(&rect);
	rect.left++;
	rect.right--;
	rect.top = m_ip*INTERLIN+3;
	rect.bottom = m_ip*INTERLIN+17;
	InvertRect(DC, &rect);
	ReleaseDC( DC );
}

void MainWindow::putfirst(char *s)
{
	m_pleftd->findstartrec(m_a, m_as, s, m_strsamount, m_botRptr);
	m_botCurp = m_pleftd->getcurp();
	m_bufptr = 0;
	m_ip = 0;
	putnext();
}

void MainWindow::putfirstbypos(int pos)
{
	m_botRptr = m_strsamount = m_pleftd->firstbypos(m_a, m_as, pos);
	m_botCurp = m_pleftd->getcurp();
	m_bufptr = 0;
	m_ip = 0;
	putnext();
}

void MainWindow::change()
{
	SetFilterMode();
	g_szSearchStr[0] = 0;
	m_splitterPos = WIDTH - m_splitterPos;
	m_onSeparator = false;

	m_wcoll.free();
	EnableMenuItem(m_popup, IDM_DELETE, MF_GRAYED);
	mydict *p = m_pleftd;
	m_pleftd = m_prightd;
	m_prightd = p;
	ResetPage();
	putfirst(m_st_right[m_ip]);
}

const char szWarning[] = "Warning:";
const char szWarning1[] = "You are about to delete ";
const char szWarning2[] = " record(s).\nDo you want to proceed?";

void MainWindow::delitems()
{
	int i;
	if(m_wcoll.getcount()==0) 
		return;

	char szBuf[256];
	strcpy(szBuf, szWarning1);
	itoa(m_wcoll.getcount(), szBuf + sizeof(szWarning1)/sizeof(szWarning1[0])-1, 10);
	strcat(szBuf, szWarning2);

	if(IDYES != MessageBox(szBuf, szWarning, MB_ICONWARNING|MB_YESNO))
		return;

	HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

	m_pleftd->free();
	m_prightd->medi2memo();
	for (i=0; i < m_wcoll.getcount(); i++) 
	{
		int j = m_wcoll.at(i);
		m_prightd->deletebyident(j);
		m_ident--;
		m_prightd->correctident(m_ident, j);
	}
	m_wcoll.save();
	m_prightd->store();
	m_prightd->free();
	m_ident = m_pleftd->memo_load();
	for (i=0; i < m_wcoll.getcount(); i++) 
	{
		int j = m_wcoll.at(i);
		m_pleftd->deletebyident(j);
		m_ident--;
		m_pleftd->correctident(m_ident, j);
	}
	m_pleftd->store();
	m_wcoll.erase();
	EnableMenuItem(m_popup, IDM_DELETE, MF_GRAYED);
	m_pleftd->memo2medi();
	if (m_ident != m_prightd->medi_load()) 
		exit(EXIT_FAILURE);
	SetCursor(hCursorOld);
	m_bufptr = 0;
	putfirst("");
}

class TIdentCollection : public THSortedCollection
{
public:
	TIdentCollection() : THSortedCollection(FAST_BUFFER_SIZE) {}
	~TIdentCollection()
	{
		freeAll();
		setLimit(0);
	}

	bool insert(Ident id)
	{
		int idx = 0;
		if (search((void*) id, idx)) 
			return false;
		atInsert(idx, (void*) id);
		return true;
	}
	bool find(Ident id)
	{
		int idx = 0;
		return search((void*) id, idx);
	}
protected:
	void DestructElement(void *) {}
};

bool present(mydict *p1, mydict *p2, const char *s1, const char *s2)
{
	Ident li = p1->firstident(s1);
	if (li == EMPTY_STRING)
		return true;

	Ident ri = p2->firstident(s2);
	if (ri == EMPTY_STRING)
		return true;

	TIdentCollection setIdents;

	for (; li != END_OF_DATA 
		; li = p1->nextident(), ri = p2->nextident())
	{
		if (ri == END_OF_DATA)
		{
			do
			{
				if (setIdents.find(li))
					return true;
				li = p1->nextident();
			}
			while (li != END_OF_DATA);
			return false;
		}

		if (li == ri || !setIdents.insert(li) || !setIdents.insert(ri))
			return true;
	}


	for (; ri != END_OF_DATA; ri = p2->nextident())
		if (setIdents.find(ri))
			return true;

	return false;
}


void MainWindow::insert()
{
	unsigned int i;
	search_buf buf;
	m_wcoll.free();
	EnableMenuItem(m_popup, IDM_DELETE, MF_GRAYED);
	mydict* pl = m_pleftd->newsimilar();
	mydict* pr = m_prightd->newsimilar();
	Ident locident = 0;
	g_szLeftStr[0] = 0;
	g_szRightStr[0]= 0;

	g_bLeftCyrillic = m_pleftd->IsCyrillic();
	if(g_bUseHook)
		g_hhook=SetWindowsHookEx(WH_MSGFILTER,MsgHookProc,
			NULL,GetCurrentThreadId());

	int result;
	do
	{
		result = DialogBox(g_hInstance,MAKEINTRESOURCE(INSERT),m_hWnd,InsertDlgProc);
		if (((result == IDOK) || (result == ID_APPLY))
			&& !present(pl, pr, g_szLeftStr, g_szRightStr)
			&& !present(m_pleftd, m_prightd, g_szLeftStr, g_szRightStr))
		{
			pl->insertbyfind(locident);
			pr->insertbyfind(locident);
			locident++;
		}
	} 
	while (result == ID_APPLY);

	if(g_hhook != NULL)
		UnhookWindowsHookEx(g_hhook);
	g_hhook = NULL;

	if(locident==0) 
	{
		delete pl;
		delete pr;
		return;
	}
	Invalidate();
	HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
	m_pleftd->free();
	m_prightd->medi2memo();
	for (i=0;i<locident;i++) 
	{
		pr->findbyident(buf, i);
		m_prightd->insertstr(buf, m_ident);
		m_ident++;
	}
	m_prightd->store();
	m_prightd->free();
	delete pr;
	m_ident = m_pleftd->memo_load();
	for (i=0;i<locident;i++) 
	{
		pl->findbyident(buf, i);
		m_pleftd->insertstr(buf, m_ident);
		m_ident++;
	}
	m_pleftd->store();
	delete pl;
	m_pleftd->memo2medi();
	if (m_prightd->medi_load() != m_ident) 
		exit(EXIT_FAILURE);
	SetCursor(hCursorOld);
	putfirst("");
}

void MainWindow::tag()
{
	if(m_ip < m_bufptr)
	{
		SIZE size;
		HDC DC = GetDC();
		m_wcoll.search(m_id[m_ip], TRUE);
		HFONT hFontold = (HFONT)SelectObject(DC, g_hFont);//CreateFontIndirect(&g_lf));
		int iOldROP = SetROP2(DC, R2_NOTXORPEN);
		MoveToEx(DC, 2, m_ip*INTERLIN+18, NULL);
		GetTextExtentPoint32(DC, m_st_right[m_ip], strlen(m_st_right[m_ip]), &size);
		LineTo(DC, size.cx + m_splitterPos, m_ip*INTERLIN+18);
		SetROP2(DC, iOldROP);
		SelectObject(DC,hFontold);
		ReleaseDC(DC);
		EnableMenuItem(m_popup, IDM_DELETE, (m_wcoll.getcount())? MF_ENABLED: MF_GRAYED);
	}
}

void MainWindow::compress()
{
	HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

	bool bSwap = false;
	if (m_pleftd->GetDataSize() < m_prightd->GetDataSize())
	{
		bSwap = true;
		mydict* tmp = m_pleftd;
		m_pleftd = m_prightd;
		m_prightd = tmp;
	}

	Ident* pSubstIds = new(nothrow) Ident[m_ident];
	Ident idx = 0;

	for (int iSeg = 0; iSeg < m_pleftd->getCount(); ++iSeg)
	{
		bucket_rec* pr = (bucket_rec*) m_pleftd->at(iSeg);
		int iCount = pr->get_idnr();
		for (int i = 0; i < iCount; ++i)
			pSubstIds[pr->get_id(i)] = idx++;
	}

	m_pleftd->free();
	m_prightd->medi2memo();

	m_prightd->compress(pSubstIds);
	delete[] pSubstIds;
	m_prightd->store(FLAG_IDS_REORDERED);
	m_prightd->free();
	m_pleftd->memo_load();
	m_pleftd->compress();

	m_pleftd->store(0, true);

	m_pleftd->free();

	m_pleftd->medi_load();
	m_prightd->medi_load();

	if (bSwap)
	{
		mydict* tmp = m_pleftd;
		m_pleftd = m_prightd;
		m_prightd = tmp;
	}

	SetCursor(hCursorOld);
	m_bufptr=0;
	putfirst("");
}

void PumpPaintMsg()
{
	MSG msg;
	while (::PeekMessage( &msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE ))
		::DispatchMessage(&msg);
}

void MainWindow::import()
{
	char szFile [_MAX_PATH];
	szFile[0] = 0;

	OPENFILENAME openFileName = {0};

	openFileName.lStructSize = sizeof (OPENFILENAME);
	openFileName.hwndOwner = m_hWnd;

	openFileName.lpstrFile = szFile;
	openFileName.nMaxFile = sizeof (szFile);

	if (!GetOpenFileName (&openFileName))
		return;

   HANDLE hFile = ::CreateFile(
		szFile,						// pointer to name of the file
		GENERIC_READ,// access (read-write) mode
		0,									// share mode 
		NULL,								// pointer to security attributes 
		OPEN_EXISTING,					// how to create 
		FILE_ATTRIBUTE_NORMAL,		// file attributes 
		NULL								// handle to file with attributes to copy
	); 
	if(INVALID_HANDLE_VALUE == hFile)
		return;
 
	DWORD dwFileSizeHigh = 0;
	DWORD dwFileSize = ::GetFileSize(hFile, &dwFileSizeHigh);
	if(dwFileSizeHigh != 0)
	{
		::CloseHandle(hFile);
		return;
	}

	HANDLE hFileMapping = ::CreateFileMapping(
		hFile,			// handle to file to map 
		NULL,				// optional security attributes 
		PAGE_READONLY,// protection for mapping object 
		0,					// high-order 32 bits of object size 
		dwFileSize,		// low-order 32 bits of object size 
		NULL				// name of file-mapping object 
	); 
	if(NULL == hFileMapping)
	{
		::CloseHandle(hFile);
		return;
	}
	LPVOID pData = ::MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, dwFileSize);
	if(NULL == pData)
	{
		::CloseHandle(hFileMapping);
		::CloseHandle(hFile);
		return;
	}
	LPBYTE pBuf = (LPBYTE) pData;


	HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));


	LPBYTE const pEndPos = pBuf + dwFileSize;
	LPBYTE pPos = pEndPos - 1;

	delete m_pleftd;
	delete m_prightd;

	m_pleftd = new(nothrow) myengdict(szEngFName);
	m_prightd = new(nothrow) myrusdict(szRusFName);

	m_ident = m_pleftd->memo_load();
	if (m_ident != m_prightd->memo_load())
		exit(EXIT_FAILURE);

	BOOL saveUseHook = g_bUseHook;
	g_bUseHook = false;

    CProgressRenderer progressRenderer;
    m_pRenderer = &progressRenderer;
	Invalidate();

	for(bool bLast = true;; bLast = false)
	{
		while (pPos > pBuf && *pPos != '\t' && *pPos != 0)
			--pPos;

		if (pPos <= pBuf)
			break;

		char buf[1024];
		const char* arrRightStrings[100];
		const char** pszRightStr = arrRightStrings;
		if (bLast)
		{
			int nSize = min(pEndPos - (pPos + 1), sizeof(buf) - 1);
			memcpy(buf, pPos + 1, nSize);
			buf[nSize] = 0;
			*pszRightStr = buf;
		}
		else 
		 *pszRightStr = (char*) (pPos + 1);

		for (int i = 1; i < sizeof(arrRightStrings) / sizeof(arrRightStrings[0]); i++)
		{
			bool bBreak = *pPos != '\t';

			while (--pPos >= pBuf && *pPos >= 32)
				;

			if (bBreak || pPos < pBuf)
				break;

			if (*pPos == '\t' || *pPos == 0)
			{
				*(++pszRightStr) = (char*) (pPos + 1);
			}
			else 
				break;
		}

		const char* szLeftStr = (char*) (pPos + 1);

		bool inserted = false;

		for (const char** psz = arrRightStrings; psz <= pszRightStr; ++psz)
			if (!present(m_pleftd, m_prightd, szLeftStr, *psz))
			{
				m_pleftd->insertbyfind(m_ident);
				m_prightd->insertbyfind(m_ident);
				m_ident++;
				inserted = true;
			}

		if (inserted)
		{
			PumpPaintMsg();

			if (progressRenderer.SetPos(pEndPos - pPos, dwFileSize))
			{
				HDC dc = GetDC();
				RECT rect;
				GetClientRect(&rect);
				progressRenderer.Render(dc, rect);
				ReleaseDC( dc );
			}
		}
	}

    m_pRenderer = NULL;

	g_bUseHook = saveUseHook;

	::UnmapViewOfFile(pData);
	::CloseHandle(hFileMapping);
	::CloseHandle(hFile);

	m_pleftd->store();
	m_prightd->store();

	m_pleftd->memo2medi();
	m_prightd->memo2medi();

	SetCursor(hCursorOld);
	m_bufptr=0;
	putfirst("");
}

void MainWindow::doExport()
{
	char szFile [_MAX_PATH];
	szFile[0] = 0;

	OPENFILENAME openFileName = {0};

	openFileName.lStructSize = sizeof (OPENFILENAME);
	openFileName.hwndOwner = m_hWnd;

	openFileName.lpstrFile = szFile;
	openFileName.nMaxFile = sizeof (szFile);

	if (!GetSaveFileName (&openFileName))
		return;

	HANDLE hFile = ::CreateFile(szFile, GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (INVALID_HANDLE_VALUE == hFile)
		return;

	HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    CProgressRenderer progressRenderer;
    m_pRenderer = &progressRenderer;
	Invalidate();

	mystring* b = new(nothrow) mybuf;
	asubident idbuf;
	for (int iSeg = 0; iSeg < m_pleftd->getCount(); ++iSeg)
	{
        PumpPaintMsg();
        int iCount = m_pleftd->firstbypos(b, idbuf, iSeg);
		for (int i = iCount; i--; )
		{
			char sb[BUFFER_SIZE * 2 + 3];
			m_pleftd->expfunc(sb, b[i]);
			size_t nPos  = strlen(sb);
			sb[nPos++] = '\t';
			m_prightd->findbyident(sb + nPos, idbuf[i]);
			nPos  = strlen(sb);
			sb[nPos++] = 13; sb[nPos++] = 10; 
			DWORD dwWritten = 0;
			::WriteFile(hFile, sb, nPos,&dwWritten, NULL);
		}
        if (progressRenderer.SetPos(iSeg, m_pleftd->getCount()))
        {
			HDC dc = GetDC();
			RECT rect;
			GetClientRect(&rect);
            progressRenderer.Render(dc, rect);
			ReleaseDC( dc );
        }
	}
	delete[] b;
	::CloseHandle(hFile);

    m_pRenderer = NULL;

	SetCursor(hCursorOld);
	m_bufptr=0;
	putfirst("");
}

BOOL MainWindow::SearchDlg()
{
	g_bCyrillic = m_pleftd->IsCyrillic();
	if(g_bUseHook && g_bCyrillic)
		g_hhook = SetWindowsHookEx(WH_MSGFILTER,MsgHookProc,
			NULL, GetCurrentThreadId());
	BOOL bSearch = DialogBox(g_hInstance,
		MAKEINTRESOURCE(SEARCH), m_hWnd, SearchStrDlgProc);
	if(g_hhook != NULL)
		UnhookWindowsHookEx(g_hhook);
	g_hhook = NULL;
	return bSearch;
}

const char szContentsSel[] = "|Contents|";
const char szContents[] = "Contents";
const char szSearchSel[] = "|Search|";
const char szSearch[] = "Search";

void MainWindow::SetFilterMode(bool filter)
{
	if (m_filterMode != filter)
	{
		MENUITEMINFO info = { 0 };
		info.cbSize = sizeof(MENUITEMINFO);
		info.fMask = MIIM_STRING;
		info.dwTypeData = const_cast<char*>(filter? szContents : szContentsSel);
		SetMenuItemInfo(GetMenu(), IDM_CONTENTS_MODE, false, &info);
		info.dwTypeData = const_cast<char*>(filter? szSearchSel : szSearch);
		SetMenuItemInfo(GetMenu(), IDM_SEARCH_MODE, false, &info);
		DrawMenuBar();
		m_filterMode = filter;
	}
}


static POINT g_cursorPos;
static bool g_bStillCursor;
static volatile HWND g_hwndToolTip;


class CSelfDestroyingToolTip: public CWindowImpl< CSelfDestroyingToolTip >
{
	enum { IDEVT_TIMEOUT = 1 };

   BEGIN_MSG_MAP( CSelfDestroyingToolTip )
      MESSAGE_HANDLER( WM_NCHITTEST, OnNcMouse )
      MESSAGE_HANDLER( WM_TIMER, OnTimer )
   END_MSG_MAP()

	void SetTimeout(unsigned int timeout)
	{
		SetTimer(IDEVT_TIMEOUT, timeout);
	}

private:
   LRESULT OnNcMouse( UINT, WPARAM, LPARAM lParam, BOOL& bHandled )
   {
		if (g_cursorPos.x != LOWORD(lParam) || g_cursorPos.y != HIWORD(lParam))
		{
			g_bStillCursor = false;
			g_cursorPos.x = LOWORD(lParam);
			g_cursorPos.y = HIWORD(lParam);
			PostMessage(WM_CLOSE);
		}
		bHandled = FALSE;
		return 0;
   }

   void OnFinalMessage( HWND hWnd )
   {
		g_hwndToolTip = NULL;
		::KillTimer(hWnd, IDEVT_TIMEOUT);
		delete this;
   }

	LRESULT OnTimer( UINT, WPARAM, LPARAM, BOOL& )
	{
		PostMessage(WM_CLOSE);
		return 0;
	}
};

const char g_arrVKMouseButtons[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };

bool IsMouseButtonDown()
{
	for (int i = 0
		; i < sizeof(g_arrVKMouseButtons) / sizeof(g_arrVKMouseButtons[0])
		; ++i)
	{
		if (GetAsyncKeyState(g_arrVKMouseButtons[i]) < 0)
			return true;
	}

	return false;
}

void HideBalloon()
{
	if (g_hwndToolTip != NULL)
	{
		::DestroyWindow(g_hwndToolTip);
		g_hwndToolTip = NULL;
	}
}


enum { MAX_TIP_ROWS = 20 };

static HWND hWndChild;

LRESULT MainWindow::OnTimer( UINT, WPARAM, LPARAM, BOOL& )
{
	if (!IsOleAccAvailable())
		return 0;

	POINT pt;
	if (!GetCursorPos(&pt))
		return 0;
	if (g_cursorPos.x != pt.x || g_cursorPos.y != pt.y)
	{
		g_bStillCursor = false;
		g_cursorPos = pt;
		HideBalloon();

		return 0;
	}
	else if (g_bStillCursor)
	{
		if (g_hwndToolTip != NULL 
				&& (IsMouseButtonDown() || hWndChild != WindowFromPoint(pt)))
			HideBalloon();
		return 0;
	}

	g_bStillCursor = true;

	if (IsMouseButtonDown())
		return 0;
            
	hWndChild = WindowFromPoint(pt);

	char* pszWord = 0;
	char* pszText = 0;

	if (!ExtractText(hWndChild, g_nTranslationBalloon, pt, pszWord, pszText))
		return 0;


	mydict* pleftd = m_pleftd;
	mydict* prightd = m_prightd;
	if (m_pleftd->getMatchingCount(pszWord) < m_prightd->getMatchingCount(pszWord))
	{
		pleftd = m_prightd;
		prightd = m_pleftd;
	}

	char output[2000] = {0};
	int len = strlen(pszText);
	Ident id = 0;
	while (len > 0 && (id = pleftd->firstident(pszText, output)) >= EMPTY_STRING)
	{
		while (--len >= 0 && IsAlpha(pszText[len]))
			;
		while (len > 0 && !IsAlpha(pszText[len - 1]))
			--len;
		if (len >= 0)
			pszText[len] = 0;
	}

	if (len <= 0)
		id = pleftd->first_similar(pszWord, output);

	if (id < EMPTY_STRING)
	{
		int iRowCnt = MAX_TIP_ROWS - 1;
		do
		{
			Ident idNext = pleftd->nextident();
			if (0 == iRowCnt && END_OF_DATA != idNext)
			{
				strcat(output, "\r\n...");
				break;

			}
			char buf[BUFFER_SIZE];
			prightd->findbyident(buf, id);
			strcat(output, "\r\n* ");
			strcat(output, buf);
			id = idNext;
		}
		while (--iRowCnt >= 0 && id != END_OF_DATA 
			&& 0 == output[sizeof(output) / sizeof(output[0]) - BUFFER_SIZE - 3]);

		// setup balloon
		g_hwndToolTip = CreateWindow(TOOLTIPS_CLASS,
				NULL,
				WS_POPUP | TTS_NOPREFIX | TTS_BALLOON | TTS_ALWAYSTIP,
				0, 0, 0, 0,
				NULL, NULL,
				g_hInstance,
				NULL);
		if (g_hwndToolTip)
		{				
			::SetWindowPos(g_hwndToolTip,
				HWND_TOPMOST,
				0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		// Do the standard tooltip coding. 
			TTTOOLINFOW	ti = { 
				offsetof(TTTOOLINFOW, lParam),
				TTF_TRANSPARENT | TTF_CENTERTIP,
				hWndChild
			};

			ti.lpszText = A2W_(output);

			((POINT&)ti.rect) = g_cursorPos;
			::ScreenToClient(hWndChild, (POINT*)&ti.rect);
			ti.rect.right = ti.rect.left;
			ti.rect.bottom = ti.rect.top;
			
			SendMessage(g_hwndToolTip, TTM_ADDTOOLW, 0, (LPARAM) &ti );

			SendMessage(g_hwndToolTip, TTM_SETMAXTIPWIDTH, 0, 600 );
			SendMessage(g_hwndToolTip, WM_SETFONT, (WPARAM) g_hFont, 0);
			SendMessage(g_hwndToolTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

			free(ti.lpszText);

			CSelfDestroyingToolTip* pSelfDestroyingToolTip
				= new (nothrow) CSelfDestroyingToolTip;
			if (pSelfDestroyingToolTip != NULL)
			{
				pSelfDestroyingToolTip->SubclassWindow(g_hwndToolTip);
				pSelfDestroyingToolTip->SetTimeout(20000);
			}
		}
	}
	free(pszText);
	free(pszWord);

	return 0;
}


#if _ATL_VER < 0x0700
CComModule _Module;
#endif


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    ::CreateMutex(NULL, FALSE, "9B46C260-D121-461d-ACA9-D15681658A95");
    DWORD dwLastError = ::GetLastError();
    if (dwLastError == ERROR_ALREADY_EXISTS)
    {
        HWND hWndOther = FindWindow(MainWindow::szClassName, MainWindow::szClassName);
        if (hWndOther != NULL)
        {
            ShowWindow(hWndOther, SW_SHOW);
            SetForegroundWindow(hWndOther);
        }
        return 0;
    }

    g_hInstance = hInstance;

    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF);

    const bool coUnitialize = S_OK == CoInitialize(NULL);
    InitCommonControls();

#if _ATL_VER < 0x0700
    _Module.Init( NULL, hInstance );
#endif

    MainWindow* pWnd = new(nothrow)MainWindow;

    pWnd->ShowWindow(nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    delete pWnd;

#if _ATL_VER < 0x0700
    _Module.Term();
#endif

    if (coUnitialize)
    {
        CoUninitialize();
    }

    _CrtDumpMemoryLeaks();

    return msg.wParam;
}
