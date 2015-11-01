#pragma once

enum { 
	EXTRACT_TYPE_INTERNET_EXPLORER = 1,
	EXTRACT_TYPE_MOZILLA_FIREFOX = 2,
	EXTRACT_TYPE_ADOBE_ACROBAT = 4,
	EXTRACT_TYPE_MS_WORD = 8,
	EXTRACT_TYPE_CHROME = 16,
};

bool IsOleAccAvailable();

bool ExtractText(
		HWND hWnd, 
		int nTypes,
		POINT pt,
		char*& rpszWord, 
		char*& rpszText);
