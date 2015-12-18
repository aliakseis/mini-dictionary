#include "stdafx.h"

#include "dictlib.h"

#include <algorithm>

#pragma intrinsic(memcpy, memset, memcmp, strlen, strcat)

static const char id[]="AS";


inline size_t strlenx(const char *string)
{
	const char *s1 = string;
	while (BYTE(*s1) > 31)
		++s1;
	return s1 - string;
}

void GetExeDir(LPSTR szPath)
{
	if (!szPath) 
		return;
	GetModuleFileName(NULL, szPath, _MAX_PATH);
	for(int nPos = int(strlen(szPath)) - 1;
		 nPos >= 0;
		 nPos--)
	{
		CHAR ch = szPath[nPos];
		if ('\\' == ch || '/' == ch || ':' == ch) 
		{
			szPath[nPos + 1] = 0;
			return;
		}
	}
}

LPCSTR GetTempFilePath()
{
	static char g_szTempPath[_MAX_PATH];
	if (0 == g_szTempPath[0])
	{
		GetExeDir(g_szTempPath);
		strcat(g_szTempPath, "DCT.TMP");
	}
	return g_szTempPath;
}

int CompareStrings(const unsigned char *key1, const unsigned char *key2) 
{
	size_t size1 = *key1++, size2 = *key2++;
	int result = memcmp(key1, key2, min(size1, size2));
	if (0 == result)
		result = size1 - size2;
	return result;
}


void DecodeDifference(unsigned int nEncodedDiff, unsigned int nPrevLen,
						unsigned int& nLength, unsigned int& nMatchLen)
{
	unsigned int nThreshold = (nPrevLen * (nPrevLen + 1)) / 2;
	if (nEncodedDiff >= nThreshold)
	{
		nEncodedDiff -= nThreshold;
		nLength = nPrevLen + nEncodedDiff / (nPrevLen + 1);
		nMatchLen = nEncodedDiff % (nPrevLen + 1);
	}
	else
	{	// Newton method
		int l = 0;
		int h = nPrevLen - 1;
		while( l <= h )
		{
			int i = (l + h) >> 1;
			nThreshold = (i * (i + 1)) / 2;

			if ( nThreshold < nEncodedDiff )
				l = i + 1;
			else
			{
				if ( nThreshold == nEncodedDiff )
				{
					l = i + 1;
					break;
				}
				h = i - 1;
			}
		}
		nLength = l - 1;
		nMatchLen = nEncodedDiff - (nLength * (nLength + 1)) / 2;
	}

	assert(nMatchLen <= nPrevLen);
	assert(nMatchLen <= nLength);
}

inline int EncodeDifference(unsigned int nPrevLen, unsigned int nLength, unsigned int nMatchLen)
{
	assert(nMatchLen <= nPrevLen);
	assert(nMatchLen <= nLength);

	int nResult = (nLength > nPrevLen)
		? (nPrevLen * (nPrevLen + 1)) / 2 
					+ (nLength - nPrevLen) * (nPrevLen + 1) + nMatchLen
		: (nLength * (nLength + 1)) / 2 + nMatchLen;

	return nResult;
}

inline bool IsGreater(const mystring& left, const mystring& right)
{
	return CompareStrings(left, right) > 0;
}

int mycollection::compare(void *key1, void *key2) 
{
	return CompareStrings((const unsigned char*) key1, (const unsigned char*) key2);
}

mydict::mydict(const char* pth):mycollection(512) 
{
	CommonConstruct();
	if (pth == NULL || pth[0] == 0) 
	{
		dictpath[0] = 0;
		dictmode=memo;
	}
	else
	{
		GetExeDir(dictpath);
		strcat(dictpath, pth);
		dictmode=none;
	}
}


void mydict::free()
{
	if ((dictmode==medi)&&(fhandle!=-1)) 
		_lclose(fhandle);
	delete pins; pins = NULL;
	delete[] offsets; offsets = NULL;
	if (dictmode != none) 
		freeAll();
	dictmode=none;
}

Ident mydict::memo_load()
{
	int f;
	Ident wb = 0;
	int i;
	myprefix pref;
	int realsize = 0;

	dictmode = memo;
	f=_lopen(dictpath,OF_READ|OF_SHARE_EXCLUSIVE);
	if (f==-1) return 0;
	_llseek(f, 2, 0);
	_lread(f, (LPSTR)&flags, 2);
	_lread(f, (LPSTR)&realsize, 2);

	// backward compatibility
	if (realsize & 0xE000)
		flags = 0;
	realsize &= ~0xE000;

	for (i=0;i<realsize;i++) 
	{
		_lread(f,(LPSTR)&pref,sizeof(pref));
		int recsize = pref.get_idsize() + pref.b0;
		bucket_rec* p = (bucket_rec*) 
				new(nothrow) BYTE[recsize + pref.get_datasize() + 6];
		if (p==NULL)
			return 0;
		memcpy(p,&pref,sizeof(pref));
		_lread(f,(LPSTR)&p->buf[1],recsize);
		atInsert(i, p);

		wb += p->get_idnr();
	}
	myoffset=_llseek(f,0,1);
	for (i=0;i<realsize;i++) 
	{
		bucket_rec* p=(bucket_rec*)at(i);
		if (p==NULL)
			return 0;
		_lread(f,(LPSTR)&p->buf[p->get_idsize() + p->buf[0] + 1],
			p->get_datasize());
	}
	if (!cumFreqs)
		ReadCumFreqs(f);
	_lclose(f);
	return wb;
}

Ident mydict::medi_load()
{
	Ident wb = 0;
	int i, recsize, realsize = 0;
	myprefix pref;
	dictmode=medi;
	delete[] offsets; offsets = NULL;
	fhandle = _lopen(dictpath, OF_READ|OF_SHARE_EXCLUSIVE);
	if (fhandle == -1) 
	{
		MoveFile(GetTempFilePath(), dictpath);
		fhandle = _lopen(dictpath, OF_READ|OF_SHARE_EXCLUSIVE);
		if (fhandle == -1) 
			return 0;
	}
	_llseek(fhandle, 2, 0);
	_lread(fhandle, (LPSTR)&flags, 2);
	_lread(fhandle, (LPSTR)&realsize, 2);

	// backward compatibility
	if (realsize & 0xE000)
		flags = 0;
	realsize &= ~0xE000;

	long offs = 0;
	if (realsize > 0)
			offsets = new(nothrow) long[realsize];
	for (i=0; i < realsize; i++) 
	{
		_lread(fhandle, (LPSTR)&pref, sizeof(pref));
		recsize = pref.get_idsize() + pref.b0;
		bucket_rec* p = (bucket_rec*) new(nothrow) BYTE[recsize+6];
		if (p==NULL)
			return 0;
		memcpy(p,&pref,sizeof(pref));
		_lread(fhandle,(LPSTR)&p->buf[1],recsize);

		offsets[i] = offs;
		offs += p->get_datasize();

		atInsert(i, p);

		wb += p->get_idnr();
	}
	myoffset=_llseek(fhandle,0,1);

	if (!cumFreqs)
	{
		int nCumFreqOffset = myoffset + offs;
		_llseek(fhandle, nCumFreqOffset, 0);
		ReadCumFreqs(fhandle);
	}
	return wb;
}

bool mydict::store(unsigned short flags_, bool bSequentialIds /*= false*/)
{
	int i,size;
	bucket_rec* p;
	long l2;
	UINT result = 0;
	int f = _lcreat(GetTempFilePath(), 0);
	if (f==HFILE_ERROR) 
		return FALSE;
	_lwrite(f, id, 2);
	_lwrite(f, (LPSTR)&flags_, 2);

	_lwrite(f, (LPSTR)&count, 2);
	l2=6;
	Ident idx = 0;
	for (i=0;i<count;i++) 
	{
		p = (bucket_rec*) at(i);
		if (p==NULL)
			return FALSE;

		BYTE buf[6 + 256 + sizeof(Ident)];

		size = p->get_idsize() + p->buf[0] + 6;
		l2+=size;

		if (bSequentialIds)
		{
			size = p->buf[0] + 6 + sizeof(Ident);
			memcpy(buf, p, size);
			p = (bucket_rec*) buf;

			p->set_sequential_origin(idx);

			assert(p->get_idsize() + p->buf[0] + 6 == size);
			idx += p->get_idnr();
		}

		result=_lwrite(f,(LPSTR)p,size);
		if (result==(UINT)HFILE_ERROR) 
		{
			_lclose(f); 
			return FALSE;
		}
	}
	myoffset=l2;
	for (i=0;i<count;i++) 
	{
		p=(bucket_rec*)at(i);
		if (p==NULL)return FALSE;
		result=_lwrite(f,(LPSTR)&p->buf[p->get_idsize() + p->buf[0] + 1],
				p->get_datasize());
		if (result==(UINT)HFILE_ERROR) 
		{
			_lclose(f); 
			return FALSE;
		}
	}
	if (cumFreqs)
		result = _lwrite(f, (LPSTR)cumFreqs, numSymbols * numSymbols * sizeof(BYTE));
	_lclose(f);
	if (result == (UINT)HFILE_ERROR) 
		return false;
	DeleteFile(dictpath);
	MoveFile(GetTempFilePath(), dictpath);
	flags = flags_;
	return TRUE;
}

void mydict::memo2medi()
{
	int i,newsize;
	if (dictmode!=memo)
		exit(EXIT_FAILURE);

	delete pins; 
	pins = NULL;
	delete[] offsets; 
	offsets = NULL;

	long offs = 0;
	if (count > 0)
			offsets = new(nothrow) long[count];
	for (i = 0; i < count; i++) 
	{
		bucket_rec* p = (bucket_rec*)at(i);
		if (p==NULL)
			continue;

		offsets[i] = offs;
		offs += p->get_datasize();

		newsize = p->get_idsize() + p->buf[0] + 6;
		void* pBuf = new(nothrow) BYTE[newsize];
		if (pBuf)
		{
			memcpy(pBuf, p, newsize);
			DestructElement(p);
			atPut(i,pBuf);
		}
	}
	fhandle=_lopen(dictpath,OF_READ|OF_SHARE_EXCLUSIVE);
	dictmode=medi;
	if (fhandle == -1)
		if (count == 0) 
			freeAll(); 
		else 
			exit(EXIT_FAILURE);
}

void mydict::medi2memo()
{
	int i;
	if (dictmode!=medi) 
		exit(EXIT_FAILURE);

	delete pins; 
	pins=NULL;
	delete[] offsets; 
	offsets = NULL;

	dictmode=memo;
	if (fhandle==-1) 
		if (count==0) 
			return;
		else 
			exit(EXIT_FAILURE);
	_llseek(fhandle,myoffset,0);
	for (i=0; i<count; i++) 
	{
		bucket_rec* p = (bucket_rec*)at(i);
		if (p==NULL)
			exit(EXIT_FAILURE);
		int oldsize = p->get_idsize() + p->buf[0] + 6;
		int newsize = oldsize + p->get_datasize();
		void* pBuf = new(nothrow) BYTE[newsize];
		if (!pBuf)
			exit(EXIT_FAILURE);
		memcpy(pBuf, p, oldsize);
		DestructElement(p);
		p = (bucket_rec*)pBuf;
		atPut(i, p);
		_lread(fhandle,(LPSTR)&p->buf[p->get_idsize() + p->buf[0] + 1],
				p->get_datasize());
	}
	_lclose(fhandle);
}


struct subrec { BYTE prm, step; };

inline void oldextract(subrec &dest, BYTE b)
{
	dest.prm = b / 23;
	dest.step = b % 23;
	if (dest.prm + dest.step > 21)
	{
		dest.prm = 21 - dest.prm;
		dest.step = 22 - dest.step;
	}
}


void GetDiffCounters(BYTE*& buf, unsigned int nPrevLen,
			unsigned int& nLength, unsigned int& nMatchLen, int nVersion)
{
	if (VERSION_PREHISTORIC == nVersion)
	{
		subrec sr;
		BYTE b = *buf++;
		if (b >= 253)
		{
			oldextract(sr, *buf++);
			if (b == 253)
				sr.prm += 22;
			else if (b == 255)
				sr.step += 22;
			else 
			{ 
				sr.prm = 21 - sr.prm; 
				sr.step = 21 - sr.step; 
			}
			assert (sr.prm + sr.step <= 43);
		}
		else
			oldextract(sr, b);
		nLength = sr.prm + sr.step;
		nMatchLen = sr.prm;
	}
	else
	{
		int nEncodedDiff = *buf++;
		if (nEncodedDiff >= DIFF_FACTOR)
			nEncodedDiff = ((nEncodedDiff % DIFF_FACTOR) << 8) + *buf++ + DIFF_FACTOR;
		DecodeDifference(nEncodedDiff, nPrevLen, nLength, nMatchLen);
	}
}

int mydict::subextract(BYTE *buf, mybuf psbuf, int mysize, int nVersion)
{
	if (mysize==0 ) 
		return 0;
	int bs=1;
	memset(diffs, 0, sizeof(diffs));
	diffs[0] = true;
	diffs[mysize] = true;
	while (bs<mysize)
		if (*buf == 0) 
		{
			buf++;
			int cnt, prm;
			if (*buf < 252)
			{
				cnt = *buf / 21 + 2;
				prm = *buf % 21 + 1;
				if (psbuf[bs-1][0] != prm)
					diffs[bs] = true;
			}
			else
			{
				cnt = *buf - 250;
				prm = psbuf[bs-1][0];
			}
			psbuf[bs][0]=(BYTE)prm;
			memmove(&psbuf[bs][1],&psbuf[bs-1][1],prm);
			for (int i=1;i<=cnt;i++)
				memmove(psbuf[bs+i],psbuf[bs],prm+1);
			buf++;
			bs+=cnt+1;

			assert(bs <= 257);
		}
		else 
		{
			diffs[bs] = true;

			unsigned int nLength, nMatchLen;
			GetDiffCounters(buf, psbuf[bs - 1][0],
				nLength, nMatchLen, nVersion);

			psbuf[bs][0] = (BYTE) nLength;
			memmove(&psbuf[bs][1], &psbuf[bs-1][1], nMatchLen);
			nLength -= nMatchLen;
			memcpy(&psbuf[bs][nMatchLen + 1], buf, nLength);
			buf += nLength;
			bs++;
		}
	startshift=mysize - bs;
	diffs[bs] = true;
	return bs;
}

int mydict::extractrec(mybuf psbuf, asubident pasidnt, int nr)
{
	int extractres;
	int subsize,b;
	BYTE *p;
	if (count==0) 
		return 0;
	bucket_rec* pr = (bucket_rec*)at(nr);
	if (!pr) 
		return 0;
	b=pr->buf[0];
	psbuf[0][0]=(BYTE)b;
	memcpy(&psbuf[0][1],&pr->buf[1],b);
	b++;
	switch (dictmode) 
	{
	case memo:
		extractres = subextract(&pr->buf[pr->get_idsize() + b],
			psbuf, pr->get_idnr(), pr->get_version());
		break;
	case medi:
		_llseek(fhandle, offsets[nr] + myoffset, 0);
		subsize = pr->get_datasize();
		p=new(nothrow) BYTE[subsize];
		_lread(fhandle,(LPSTR)p,subsize);
		extractres=subextract(p,psbuf, pr->get_idnr(), pr->get_version());
		delete[] p;
		break;
	default: 
		exit(EXIT_FAILURE);
	}
	//ident array handling
	for (int i = 0; i < extractres; i++)
		*(pasidnt++) = pr->get_id(i);
	return extractres;
}


void subextractstr(mystring& mybuf,BYTE* buf, int shift, int nVersion)
{
	int bs=0;
	while (bs<shift)
		if (*buf == 0) 
		{
			buf++;
			if (*buf < 252)
			{
				mybuf[0] = BYTE(*buf % 21 + 1);
				bs += *buf / 21 + 3;
			}
			else
				bs += *buf - 249;
			buf++;
		}
		else 
		{
			unsigned int nLength, nMatchLen;
			GetDiffCounters(buf, mybuf[0],
				nLength, nMatchLen, nVersion);

			mybuf[0] = (BYTE) nLength;
			nLength -= nMatchLen;
			memcpy(&mybuf[nMatchLen + 1], buf, nLength);
			buf += nLength;
			bs++;
		}
}

void mydict::extractstr(mystring& dest, int nr, int shift)
{
	int subsize;
	BYTE *pp;
	if (count==0) { dest[0]=0; return; }
	bucket_rec* p=(bucket_rec*)at(nr);
	if (!p) return;
	dest[0]=p->buf[0];
	memcpy(dest+1,p->buf+1,dest[0]);
	if (shift>0 ) 
		switch (dictmode) 
		{
		case memo:
			subextractstr(dest,
				&p->buf[p->get_idsize()+dest[0]+1], shift, p->get_version());
			break;
		case medi:
			_llseek(fhandle, offsets[nr] + myoffset, 0);
			subsize = p->get_datasize();
			pp=new(nothrow) BYTE[subsize];
			_lread(fhandle,(LPSTR)pp,subsize);
			subextractstr(dest, pp, shift, p->get_version());
			delete[] pp;
			break;
		default: 
			exit(EXIT_FAILURE);
		}
}

void mydict::expfunc(char *dest, mystring& s)
{
	BYTE b = s[0];
	memmove(dest, &s[1], b);
	dest[b] = '\0';
}


bool mysearch(void *data,void *arg)
{
	bucket_rec* p = (bucket_rec*) data;
	Ident id = *(Ident*) arg;

	if (p->is_sequential())
	{
		long offset = id - p->get_id(0);
		if (offset >= 0 && offset < p->get_idnr())
		{
			*(int*)arg = offset;
			return true;
		}
		return false;
	}

	WORD* pIds = (WORD*)(p->buf + p->buf[0] + 1);
	WORD* pos = pIds + p->get_idnr();

	for (;;)
	{
		switch ((pos - pIds) & 7)
		{
		case 0:
			while (pos != pIds)
			{
				if (*(--pos) == (WORD)id) goto found;
		case 7: if (*(--pos) == (WORD)id) goto found;
		case 6: if (*(--pos) == (WORD)id) goto found;
		case 5: if (*(--pos) == (WORD)id) goto found;
		case 4: if (*(--pos) == (WORD)id) goto found;
		case 3: if (*(--pos) == (WORD)id) goto found;
		case 2: if (*(--pos) == (WORD)id) goto found;
		case 1: if (*(--pos) == (WORD)id) goto found;
			}
		}
		return false;
found:
		if (p->get_id(pos - pIds) == id) 
		{
			*(int*)arg = pos - pIds;
			return true;
		}

	}
}

void mydict::findbyident(char *dest, Ident ident)
{
	mystring s;
	Ident col=ident;
	int row=firstThat(mysearch,&col);
	if (row!=-1) 
	{
		extractstr(s,row,col);
		expfunc(dest,s);    
	}
	else dest[0]='\0';
}

void wrdoubl(BYTE* buf, int& bp, WORD cnt, int lastprm, int lastlen)
{
	if (cnt == 0)
		return;

	if (cnt > 2)
	{
		buf[bp++]=0;
		if (lastprm < 22)
			buf[bp++] = BYTE((cnt-3)*21 + lastprm - 1);
		else
		{
			assert(cnt < 7);
			assert(lastlen == lastprm);
			buf[bp++] = cnt + 249;
		}
		return;
	}

	while(cnt-- > 0) 
	{
		int nEncodedDiff 
			= EncodeDifference(lastlen, lastprm, lastprm);
		if (nEncodedDiff >= DIFF_FACTOR)
		{
			nEncodedDiff -= DIFF_FACTOR;
			buf[bp++] = BYTE (nEncodedDiff >> 8) + DIFF_FACTOR;
		}
		buf[bp++] = BYTE (nEncodedDiff & 255);
		lastlen = lastprm;
	}
}

void mydict::insertrec(mybuf psbuf,asubident pasidnt,int bsize,int nr)
{
	BYTE* buf = new(nothrow) tbuf;
	int difflen;
	mystring diff;
	int bp=0;
	WORD cnt=0; 
	int lastprm = 0;
	int lastlen = 0;
	int i;
	for (i=1;i<bsize;i++) 
	{
		int prm = 1;
		WORD max = psbuf[i-1][0];
		if (max > psbuf[i][0]) 
			max = psbuf[i][0];
		while (psbuf[i-1][prm] == psbuf[i][prm] && prm <= max) 
			prm++;
		difflen = psbuf[i][0] + 1 - prm;
		memmove(diff, &psbuf[i][prm], difflen);
		prm--;

		if (difflen == 0 && prm == lastprm 
				&& (cnt < 6 && lastlen == prm || prm < 22 && cnt < 14)) 
			cnt++;
		else 
		{
			wrdoubl(buf, bp, cnt, lastprm, lastlen);
			lastlen = psbuf[i-1][0];
			lastprm = prm;
			if (difflen == 0 && (prm < 22 || lastlen == prm)) 
			{
				cnt = 1;
			}
			else 
			{
				cnt = 0;

				int nEncodedDiff 
					= EncodeDifference(lastlen, psbuf[i][0], prm);
				if (nEncodedDiff >= DIFF_FACTOR)
				{
					nEncodedDiff -= DIFF_FACTOR;
					buf[bp++] = BYTE (nEncodedDiff >> 8) + DIFF_FACTOR;
				}
				buf[bp++] = BYTE (nEncodedDiff & 255);

				memmove(&buf[bp], diff, difflen);
				bp += difflen;
			}
		}
	}
	wrdoubl(buf, bp, cnt, lastprm, lastlen);
	WORD l=psbuf[0][0];

	Ident mask = 0;
	for (i = 0; i < bsize; i++)
		mask |= pasidnt[i];

	int numplanes = GetNumPlanes(mask);

	int planessize = (bsize<<1) + ((bsize + 7) >> 3) * numplanes;

	bucket_rec* p=(bucket_rec*)new(nothrow) BYTE[bp + planessize + l + 6];
	if (p==NULL)
		exit(EXIT_FAILURE);

	p->set(numplanes, bp, bsize);

	memcpy(p->buf,psbuf,l+1);
	//ident array handling
	for (i = 0; i < bsize; i++)
		p->set_id(i, *(pasidnt++));

	memcpy(&p->buf[l + planessize + 1], buf, bp);

	delete[] buf;
	atsubst(nr, p);
}


void mydict::correctident(Ident oldidnt, Ident newidnt)
{
	if (oldidnt==newidnt) 
		return;
	Ident col = oldidnt;
	int row = firstThat(mysearch,&col);
	if (row == -1)
		exit(EXIT_FAILURE);

	bucket_rec* p=(bucket_rec*)at(row);
	if (!p->is_sequential())
		p->set_id(col, newidnt);
	else
	{
		mystring* b = new(nothrow) mybuf;
		asubident idbuf;

		//ident array handling
		int n = extractrec(b, idbuf, row);
		idbuf[col] = newidnt;
		insertrec(b, idbuf, n, row);
		delete[] b; 
	}
}

inline WORD get_idnr(void* p) { return p? (WORD)((recbase*)p)->get_idnr() : (WORD)0; }

enum addtype { no_add, lower_add, upper_add };

void mydict::deletebyident(Ident ident)
{
	addtype add;
	WORD n0(0), n1(0);
	int col=ident;
	int myrow=firstThat(mysearch,&col);
	if (myrow==-1)
        exit(EXIT_FAILURE);

	mystring *b=new(nothrow) mybuf;
	asubident idbuf;

	int n=extractrec(b, idbuf, myrow) - 1;
	if (n==0 ) 
	{
		delete[] b; 
		atFree(myrow);
		return;
	}
	memmove(&b[col],&b[col+1],(n-col)*sizeof(mystring));
	memmove(&idbuf[col],&idbuf[col+1],(n-col) * sizeof(Ident));
	if (myrow>0)
		if (myrow<count-1 ) 
		{
			n0 = get_idnr(at(myrow-1));
			n1 = get_idnr(at(myrow+1));
			add = (n0 <= n1)? lower_add : upper_add;
		}
		else 
		{
			n0 = get_idnr(at(myrow-1));
			add = lower_add;
		}
	else if (myrow<count-1 ) 
	{
		n1 = get_idnr(at(myrow+1));
		add = upper_add;
	}
	else 
		add = no_add;

	switch(add) 
	{
	case lower_add:
		if (n+n0 > 257) 
			break;
		if (n0 != extractrec(&b[n], &idbuf[n], myrow-1))
			exit(EXIT_FAILURE);
		n+=n0;
		myrow--;
		atFree(myrow);
		break;
	case upper_add:
		if (n+n1 > 257) 
			break;
		memmove(&b[n1],b,n*sizeof(mystring));
		memmove(&idbuf[n1],idbuf,n * sizeof(Ident));
		if (n1 != extractrec(b, idbuf, myrow+1)) 
			exit(EXIT_FAILURE);
		n+=n1;
		atFree(myrow+1);
		break;
	case no_add:
		break;
	}
	insertrec(b,idbuf,n,myrow);
	delete[] b; 
}



void mydict::findstartrec(mybuf sbuf, asubident asidnt, char *ss, int& nCount, int& nPos)
{
	mystring ssample;
	compfunc(ssample,ss);
	search(ssample,curp);
	if (curp >= count)
		nCount = nPos = 0;
	else
	{
		nCount = extractrec(sbuf, asidnt, curp);
		nPos = std::upper_bound(sbuf, sbuf + nCount, ssample, IsGreater) - sbuf;
	}
}

int mydict::firstbypos(mybuf sbuf,asubident asidnt,int pos)
{
	curp=pos;
	if (curp >= count) 
		curp=count-1;
	if (curp < 0) 
		curp=0;
	return extractrec(sbuf, asidnt, curp);
}

int mydict::findnextrec(mybuf sbuf,asubident asidnt)
{
	if (curp >= count) 
	{
		startshift=0;
		return 0;
	}
	curp++;
	if (curp == count)
	{
		startshift=0;
		return 0;
	}
	return extractrec(sbuf, asidnt, curp);
}

int mydict::findprevrec(mybuf sbuf,asubident asidnt,int vol)
{
	int n,savstartshift;
	if (curp == 0) 
	{
		if (startshift != 0) 
		{
			savstartshift=startshift;
			n = extractrec(sbuf, asidnt, 0);
			if (savstartshift > vol) 
			{
				savstartshift-=vol;
				startshift=savstartshift;
				return n-savstartshift;
			}
			else return n;
		}
		else return 0;
	}
	if (vol <= startshift) 
	{
		savstartshift=startshift;
		n = extractrec(sbuf, asidnt, curp);
		savstartshift-=vol;
		startshift=savstartshift;
		return n-savstartshift;
	}
	vol -= startshift;
	n = 0;
	do 
	{
		vol -= n;
		curp--;
		n = extractrec(sbuf, asidnt, curp);
	} 
	while((vol>n) && (curp!=0));
	if (vol > n) 
		return n;
	else 
	{
		startshift = n-vol;
		return vol;
	}
}

Ident mydict::firstident(const char *ss, char *substring /*= NULL*/)
{
	if (pins == NULL) 
		pins = new(nothrow) myinsertrec;
    if (pins == NULL)
        exit(EXIT_FAILURE);
	compfunc(pins->myssample,ss);
	if (pins->myssample[0] == 0) 
		return EMPTY_STRING;
	pins->firstbuf=TRUE;
	if (count == 0) 
	{
		pins->bufsize=0;
		pins->myp=0;
		pins->myidp=0;
	}
	else 
	{
		search(pins->myssample, pins->myp);
		if (pins->myp >= count) 
			pins->myp = count - 1;
		pins->myidp = pins->myp;
		pins->bufsize = extractrec(pins->mystrbuf,
			pins->myidbuf, pins->myp);
	}

	pins->bptr = std::upper_bound(
			pins->mystrbuf, pins->mystrbuf + pins->bufsize, pins->myssample, IsGreater) 
		- pins->mystrbuf;

	pins->bpid=0;
	if (pins->bptr != 0
		&& compare(pins->mystrbuf[pins->bpid = pins->bptr-1], pins->myssample) == 0)
	{
		if (substring != NULL)
			expfunc(substring, pins->myssample);
		return pins->myidbuf[pins->bpid];
	}
	else
	{
		if (substring != NULL)
		{
            //mystring prev;
			//if (pins->bptr < pins->bufsize)
			//	prev = pins->mystrbuf[pins->bptr];
			//else if (pins->myp > 0)
			//	prev = ((bucket_rec*) at(pins->myp - 1))->buf;
			//else
			//	return END_OF_DATA;

            if (pins->bptr >= pins->bufsize && pins->myp <= 0)
                return END_OF_DATA;

            mystring& prev = *((pins->bptr < pins->bufsize) 
                ? pins->mystrbuf + pins->bptr
                : &((bucket_rec*)at(pins->myp - 1))->ms);

			if (prev[0] <= pins->myssample[0])
			{
				char buf[BUFFER_SIZE];
				expfunc(buf, pins->myssample);
				expfunc(substring, prev);
				if (0 == memcmp(buf, substring, strlen(substring)))
				{
					memcpy(pins->myssample, prev, prev[0] + 1);
					int index = 0;
					search(pins->myssample, index);
					if (index != pins->myp)
					{
						pins->myidp = pins->myp = index;
						pins->bufsize = extractrec(pins->mystrbuf,
							pins->myidbuf, pins->myp);
					}
					pins->bptr = std::upper_bound(
							pins->mystrbuf, pins->mystrbuf + pins->bufsize, pins->myssample, IsGreater) 
						- pins->mystrbuf;
					assert(pins->bptr > 0);
					pins->bpid = pins->bptr-1;
					return pins->myidbuf[pins->bpid];
				}
			}
		}
		return END_OF_DATA;
	}
}

Ident mydict::nextident()
{
	if (pins->myssample[0] == 0) 
		return EMPTY_STRING;
	if (pins->bpid == 0) 
	{
		if (pins->myidp >= count-1) 
			return END_OF_DATA;
		pins->myidp++;
		pins->firstbuf = FALSE;
		pins->bpid = extractrec(pins->mystrbuf, pins->myidbuf, pins->myidp);
		if (pins->bpid == 0) 
			return END_OF_DATA;
	}
	pins->bpid--;
	if (compare(pins->mystrbuf[pins->bpid],pins->myssample)==0)
		return pins->myidbuf[pins->bpid];
	else return END_OF_DATA;
}

void mydict::insertbyfind(Ident ident)
{
	assert (pins->myssample[0] != 0);

	if (count == 0) 
		atInsert(0,0);
	if (!pins->firstbuf)
		extractrec(pins->mystrbuf, pins->myidbuf, pins->myp);
	memmove(pins->mystrbuf[pins->bptr+1],pins->mystrbuf[pins->bptr],
		(pins->bufsize-pins->bptr) * sizeof(mystring));
	memmove(pins->mystrbuf[pins->bptr],pins->myssample,pins->myssample[0]+1);
	memmove(&pins->myidbuf[pins->bptr+1],&pins->myidbuf[pins->bptr],
		(pins->bufsize-pins->bptr) * sizeof(Ident));
	pins->myidbuf[pins->bptr] = ident;
	pins->bufsize++;
	if (pins->bufsize < 258) 
		insertrec(pins->mystrbuf,pins->myidbuf,pins->bufsize,pins->myp);
	else 
	{
		atInsert(pins->myp,NULL);
		insertrec(&pins->mystrbuf[129],&pins->myidbuf[129],129,pins->myp);
		insertrec(pins->mystrbuf,pins->myidbuf,129,pins->myp+1);
	}
}

void mydict::insertstr(char *ss,Ident ident)
{
	if (firstident(ss)!=EMPTY_STRING) 
		insertbyfind(ident);
}

inline unsigned short bswap_16(unsigned short x) {
	return (x>>8) | (x<<8);
}

inline DWORD ByteSwap(DWORD x) {
	return (bswap_16(x&0xffff)<<16) | (bswap_16(x>>16));
}

int mycomp(BYTE *res,BYTE *source, const BYTE *table_, int nSymbols)
{
	if (!source[0])
	{
		res[0] = 0;
		return 0;
	}

	const BYTE *table = table_;

	int pos = 8;

	for(int i = 1; i <= source[0] && pos < MYSTRING_SIZE * 8; i++)
	{
		BYTE b = source[i];
		DWORD inc = 0x80000000 >> (pos & 7);
		DWORD cum = 0;
		int j;
		for (j = 0; j < b; ++j)
			cum += inc >> table[j];

		cum <<= 1;

		*((DWORD*)(res + (pos >> 3))) += ByteSwap(cum);

		pos += table[b];
		if (nSymbols)
			table = table_ + b * nSymbols;
	}
	res[0] = BYTE(min(pos >> 3, MYSTRING_MAX));
	while (res[0] && res[res[0]] == 0)
		--res[0];

	return pos - 8;
}


inline const BYTE* GetShiftPtr(DWORD val, const BYTE *pByte, int nSymbols)
{
	long cum = (long) (val >> 1);

	if (cum & 0x40000000)
	{
		pByte += nSymbols;
		for (cum = 0x7FFFFFFF - cum; (cum -= (0x80000000 >> *(--pByte))) >= 0; );
	}
	else
	{
		for (; (cum -= (0x80000000 >> *pByte)) >= 0; ++pByte);
	}

	return pByte;
}



void myexp(BYTE *res,BYTE *source, const BYTE *table_, int nSymbols, bool bHasCumFreqs)
{
	assert(nSymbols > 0);

	if (!source[0])
	{
		res[0] = 0;
		return;
	}

	const BYTE *table = table_;
	int nOffset = 0;
	DWORD val = 0;
	int pos = 0;
	BYTE ch = 0;

	int ofs;
	while (ofs = pos >> 3, source[0] - ofs > 4)
	{
		val = ByteSwap(*(DWORD*)(source + ofs + 1)) << (pos & 7);
		const BYTE* pByte = GetShiftPtr(val, table, nSymbols);

		ch = res[++nOffset] = BYTE(pByte - table);
		pos += *pByte;

		if (bHasCumFreqs)
			table = table_ + ch * nSymbols;
	}

	switch(source[0] - ofs)
	{
	case 1:
		val = source[ofs+1] << ((pos & 7) + 24);
		break;
	case 2:
		val = ByteSwap(*(WORD*)(source + ofs + 1)) << (pos & 7);
		break;
	case 3:
		val = ByteSwap(*(DWORD*)(source + ofs)) << ((pos & 7) + 8);
		break;
	case 4:
		val = ByteSwap(*(DWORD*)(source + ofs + 1)) << (pos & 7);
		break;
	default:
		assert(false);
	}

	do
	{
		const BYTE* pByte = GetShiftPtr(val, table, nSymbols);

		ch = res[++nOffset] = BYTE(pByte - table);

		val <<= *pByte;

		if (bHasCumFreqs)
			table = table_ + ch * nSymbols;
	}
	while (val && nOffset < BUFFER_MAX);

	while ((res[nOffset]==0) && (nOffset!=0)) 
		nOffset--;
	res[0] = BYTE(nOffset);
}

static const char achar[]=" /,-.";

void correct(BYTE* s, int nSize)
{
	int i;
	for (i = nSize; i > 0; i--)
		if (s[i] >= 5)
			break;
	if (i == 0)
	{
		s[0] = 0;
		return;
	}


	int nOldSize;
	do
	{
		nOldSize = nSize;
		i = 1;
		while (i < nSize) 
		{
			if ((s[i] < 5) && (s[i + 1] == 0)) 
			{
				nSize--;
				memmove(&s[i + 1], &s[i + 2], nSize - i);
			}
			else if ((s[i + 1]<5) && (s[i] == 0)) 
			{
				memmove(&s[i], &s[i + 1], nSize - i);
				nSize--;
			}
			else i++;
		}

		// Remove // 
		for (i = nSize-1; i > 0; --i)
			if (1 == s[i] && 1 == s[i+1])
			{
				if (i > 1 && s[i-1] < 5 || i < nSize-1 && s[i+2] < 5)
				{
					nSize -= 2;
					memmove(&s[i], &s[i + 2], nSize - i + 1);
				}
				else
				{
					--nSize;
					memmove(&s[i + 1], &s[i + 2], nSize - i);
					s[i] = 0;
				}
				--i;
			}

		// Remove double symbols
		for (i = nSize-1; i > 0; --i)
			if (s[i] < 5 && s[i] == s[i+1])
			{
				--nSize;
				memmove(&s[i + 1], &s[i + 2], nSize - i);
			}

		while (nSize > 1 && s[nSize - 1] == 2 && (s[nSize] == 3 || s[nSize] == 4))
			nSize -= 2;

		while ((nSize!=0) && (s[nSize]==0 || s[nSize]==2)) 
			nSize--;

		while ((nSize!=0) && (s[1]==0 || s[1]==2 || s[1]==3 || s[1]==4)) 
		{
			nSize--;
			memmove(s + 1, s + 2, nSize);
		}

		// Remove single / at the beginning or the end
		bool bFirstSlash = s[1] == 1;
		if (bFirstSlash != (s[nSize] == 1))
		{
			for (i = nSize; --i > 1; )
				if (s[i] == 1)
					break;

			if (1 == i)
			{
				nSize--;
				if (bFirstSlash)
					memmove(s + 1, s + 2, nSize);
			}
		}

		// Remove garbage
		if (nOldSize == nSize)
		{
			for (i = 0; i < nSize && s[i+1] < 5; ++i)
				;
			if (i > 1)
			{
				nSize -= i;
				memmove(&s[1], &s[i + 1], nSize);
			}
		}
	}
	while (nOldSize != nSize);

	s[0] = min(nSize, 254);
}

void mydict::compfunc(mystring& dest, const char *s)
{
	if (lookupChars)
	{
		BYTE sb[1024];
		BYTE sb1[MYSTRING_SIZE + 3] = {0};
		int nSize = min(strlenx(s), sizeof(sb) - 1);
		for (int i = 1; i <= nSize; i++) 
			sb[i] = lookupChars[(BYTE) s[i - 1]];
		 
		correct(sb, nSize);
		if (cumFreqs)
			mycomp(sb1, sb, cumFreqs, numSymbols);
		else
			mycomp(sb1, sb, defCumFreqs, 0);
		if (sb1[0] > MYSTRING_MAX)
			sb1[0] = MYSTRING_MAX;
		memmove(dest,sb1,sb1[0]+1);
	}
	else
	{
		int b=strlenx(s);
		if (b > MYSTRING_MAX) 
			b = MYSTRING_MAX;
		dest[0] = (BYTE)b;
		memmove(&dest[1],s,b);
	}
}

int mydict::getMatchingCount(const char *s)
{
	int nCount = 0;
	if (lookupChars)
	{
		for (; *s != 0; ++s)
			if (lookupChars[(BYTE)*s])
				++nCount;
	}
	return nCount;
}


struct TreeHelper
{
	long freq;
	int begin, end;
};

void mydict::compress(Ident* pSubstIds /*= NULL*/)
{
	long* freqs = new(nothrow) long[numSymbols * numSymbols];
	memset(freqs, 0, numSymbols * numSymbols * sizeof(long));
	int i;
	mystring *b = new(nothrow) mybuf;
	asubident idbuf;
	bool bOldVersion = false;
	for (int iSeg = 0; iSeg < getCount(); ++iSeg)
	{
		bucket_rec* pr = (bucket_rec*) at(iSeg);
		if (pr->get_version() != VERSION_CURRENT)
			 bOldVersion = true;

		int iCount = extractrec(b, idbuf, iSeg);
		for (int i = 0; i < iCount; ++i)
		{
			BYTE sb[BUFFER_SIZE];
			if (cumFreqs)
				myexp(sb, b[i], cumFreqs, numSymbols, true);
			else
				myexp(sb, b[i], defCumFreqs, numSymbols, false);
			BYTE chPrev = 0;
			for (int j = 1; j <= sb[0]; j++)
			{
				BYTE ch = sb[j];
				++freqs[chPrev * numSymbols + ch];
				chPrev = ch;
			}
		}
	}

	BYTE* newCumFreqs = new(nothrow) BYTE[numSymbols * numSymbols];
	memset(newCumFreqs, 1, numSymbols * numSymbols * sizeof(BYTE));
	TreeHelper* helpers = new(nothrow) TreeHelper[numSymbols];

	for (i = 0; i < numSymbols; ++i)
	{
		int j;
		for(j = 0; j < numSymbols; ++j)
		{
			helpers[j].freq = freqs[i * numSymbols + j];
			helpers[j].begin = j;
			helpers[j].end = j + 1;
		}
		BYTE* curFreqs = newCumFreqs + i * numSymbols;

		for(int count = numSymbols; count > 2; --count)
		{
			long minFreq = 0x7FFFFFFF;
			int cur = 0, diff = 0;
			for(j = count-1; j--; )
			{
				long freq = helpers[j].freq + helpers[j+1].freq;
				if (freq < minFreq 
				|| freq == minFreq && diff > helpers[j+1].end - helpers[j].begin)
				{
					minFreq = freq;
					cur = j;
					diff = helpers[j+1].end - helpers[j].begin;
				}
			}
			helpers[cur].end = helpers[cur+1].end;
			helpers[cur].freq += helpers[cur+1].freq;
			for(j = helpers[cur].begin; j < helpers[cur].end; ++j)
				++curFreqs[j];
			memmove(helpers + cur + 1, helpers + cur + 2
				, (count - cur - 2) * sizeof(TreeHelper));
		}
	}
	delete[] helpers;

	if (!cumFreqs || pSubstIds || bOldVersion
		|| memcmp(newCumFreqs, cumFreqs, numSymbols * numSymbols * sizeof(BYTE)))
	{
		for (int iSeg = 0; iSeg < getCount(); ++iSeg)
		{
			int iCount = extractrec(b, idbuf, iSeg);
			for (int i = 0; i < iCount; ++i)
			{
				BYTE sb[BUFFER_SIZE] = {0};
				if (cumFreqs)
					myexp(sb, b[i], cumFreqs, numSymbols, true);
				else
					myexp(sb, b[i], defCumFreqs, numSymbols, false);

				BYTE sb1[MYSTRING_SIZE + 3] = {0};
				mycomp(sb1, sb, newCumFreqs, numSymbols);
				if (sb1[0] > MYSTRING_MAX)
					sb1[0] = MYSTRING_MAX;
				memmove(b[i],sb1,sb1[0]+1);
			}

			if (pSubstIds)
				for (int i = 0; i < iCount; ++i)
					idbuf[i] = pSubstIds[idbuf[i]];

			insertrec(b, idbuf, iCount, iSeg);
		}
		delete[] cumFreqs;
		cumFreqs = newCumFreqs;
	}
	else
		delete[] newCumFreqs;
	delete[] b;
	delete[] freqs;
}

void mydict::ReadCumFreqs(int f)
{
	size_t freqsSize = numSymbols * numSymbols * sizeof(BYTE);
	BYTE* newCumFreqs = new(nothrow) BYTE[numSymbols * numSymbols];
	if (freqsSize == _lread(f, newCumFreqs, freqsSize))		
		cumFreqs = newCumFreqs;
	else
		delete[] newCumFreqs;
}

bool mydict::MakeHash(const char* s, hash_buf& pHash)
{
	if (!lookupChars || !cumFreqs)
		return false;

	memset(pHash, 0, sizeof(hash_buf));

	BYTE sb[256];
	sb[1] = (BYTE) min(strlenx(s), 254);
	sb[0] = sb[1] + 1;
	int i;
	for (i=2; i<=sb[0]; i++) 
		sb[i] = lookupChars[(BYTE) s[i - 2]];

	BYTE sb1[MYSTRING_SIZE + 3] = {0};
	int nBits = mycomp(sb1, &sb[1], cumFreqs, numSymbols);
	if (nBits < 8)
		return false;
	BYTE b = sb1[(nBits >> 3) + 1];
	pHash[b] = true;

	for (i = 0; i < numSymbols; ++i)
	{
		sb[1] = i;
		BYTE sb1[MYSTRING_SIZE + 3] = {0};
		int nBits = mycomp(sb1, sb, cumFreqs, numSymbols);
		if (nBits < 15)
			return false;

		int nIdx = (nBits - 1) >> 3;
		BYTE b = sb1[nIdx];
		pHash[b] = true;

		int j;
		for (j = 1; j <= (nBits & 7); ++j)
		{
			b = (sb1[nIdx] << j) | (sb1[nIdx + 1] >> (8 - j));
			pHash[b] = true;
		}
		for (; j < 8; ++j)
		{
			b = (sb1[nIdx - 1] << j) | (sb1[nIdx] >> (8 - j));
			pHash[b] = true;
		}
	}

	return true;
}

long mydict::GetDataSize()
{
	long size = 0;
	for (int iSeg = 0; iSeg < getCount(); ++iSeg)
	{
		bucket_rec* pr = (bucket_rec*) at(iSeg);
		size += pr->get_datasize() + pr->buf[0];
	}
	return size;
}

/////////////////////////////////////////////////////////////////////////

const BYTE myengdict::g_lookupChars[256] =
{
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  2,  3,  4,  1, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 0,  0,  0,  0,  0, 
	0,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
};

const BYTE myrusdict::g_lookupChars[256] =
{
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  2,  3,  4,  1, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,  0,  10, 0,  0,  0,  0,  0,  0,  0, 
	5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 
	5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 
};


#define COMPATIBLE

#ifdef COMPATIBLE

#define CUM(x) (x & 0x7F)

const BYTE g_tableEnglishFrequencies[myengdict::NUM_SYMBOLS]=
{
	CUM(0x0005), /* */ CUM(0x0806),/*slash*/ CUM(0x0C07), /*),*/ CUM(0x0E08), /*-*/ CUM(0x0F08), /*.*/
	CUM(0x1004), /*a*/ CUM(0x2005), /*b*/ CUM(0x2805), /*c*/ CUM(0x3004), /*d*/ CUM(0x4003), /*e*/
	CUM(0x6006), /*f*/ CUM(0x6406), /*g*/ CUM(0x6805), /*h*/ CUM(0x7004), /*i*/ CUM(0x8007), /*j*/
	CUM(0x8207), /*k*/ CUM(0x8406), /*l*/ CUM(0x8805), /*m*/ CUM(0x9004), /*n*/ CUM(0xA005), /*o*/
	CUM(0xA806), /*p*/ CUM(0xAC06), /*q*/ CUM(0xB005), /*r*/ CUM(0xB805), /*s*/ CUM(0xC003), /*t*/
	CUM(0xE004), /*u*/ CUM(0xF006), /*v*/ CUM(0xF407), /*w*/ CUM(0xF607), /*x*/ CUM(0xF806), /*y*/
	CUM(0xFC06), /*z*/ 
};

const BYTE g_tableRussianFrequencies[myrusdict::NUM_SYMBOLS]=
{
	CUM(0x0005), /* */ CUM(0x0806),/*slash*/ CUM(0x0C07), /*),*/ CUM(0x0E08), /*-*/ CUM(0x0F08), /*.*/
	CUM(0x1004), /* */ CUM(0x2005), /*¡*/ CUM(0x2805), /*¢*/ CUM(0x3005), /*£*/ CUM(0x3805), /*¤*/
	CUM(0x4003), /*¥*/ CUM(0x6005), /*¦*/ CUM(0x6805), /*§*/ CUM(0x7004), /*¨*/ CUM(0x8005), /*©*/
	CUM(0x8805), /*ª*/ CUM(0x9005), /*«*/ CUM(0x9805), /*¬*/ CUM(0xA004), /*­*/ CUM(0xB004), /*®*/
	CUM(0xC005), /*¯*/ CUM(0xC805), /*à*/ CUM(0xD005), /*á*/ CUM(0xD805), /*â*/ CUM(0xE006), /*ã*/
	CUM(0xE408), /*ä*/ CUM(0xE508), /*å*/ CUM(0xE607), /*æ*/ CUM(0xE807), /*ç*/ CUM(0xEA08), /*è*/
	CUM(0xEB09), /*é*/ CUM(0xEB89), /*ê*/ CUM(0xEC06), /*ë*/ CUM(0xF005), /*ì*/ CUM(0xF807), /*í*/
	CUM(0xFA07), /*î*/ CUM(0xFC06), /*ï*/ 
};

#undef CUM

#else

const BYTE g_tableUniversal[] =
{
		  1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
};

#endif //COMPATIBLE


inline BOOL UseCapitalLetters()
{
	extern BOOL g_bCapitalLetters;
	return g_bCapitalLetters;
}


inline bool IsVowel(char ch)
{
  ch = ToLowerCase(ch);
  return ch=='a' || ch=='e' || ch=='i' || ch=='o' || ch=='u';
}


void myengdict::CommonConstruct()
{
	lookupChars = g_lookupChars;
	numSymbols = NUM_SYMBOLS;
#ifdef COMPATIBLE
	defCumFreqs = g_tableEnglishFrequencies;
#else
	defCumFreqs = g_tableUniversal + NUM_SYMBOLS - 2;
#endif //COMPATIBLE
}

void myengdict::expfunc(char *dest, mystring& s)
{
	BYTE sb[BUFFER_SIZE];
	if (cumFreqs)
		myexp(sb, s, cumFreqs, NUM_SYMBOLS, true);
	else
		myexp(sb, s, defCumFreqs, NUM_SYMBOLS, false);
	for(int i=0;i<sb[0];i++) 
	{
		char ch=sb[i+1];
		if (ch > 30) 
			dest[i] = '?';
		else if (ch > 4) 
			dest[i] = BYTE(ch + (UseCapitalLetters()? 60 : 92));
		else dest[i]=achar[ch];
	}
	dest[sb[0]]='\0';
}

#define RETURN_VALID_IDENT(stmt)	\
{									\
	Ident id = (stmt);				\
	if (id < EMPTY_STRING)			\
		return id;					\
}

extern int stem(char * p, int i, int j);

Ident myengdict::first_similar(char *sWord, char *substring)
{
	RETURN_VALID_IDENT(firstident(sWord, substring));

    int iWordLen = strlen(sWord);
	if (iWordLen < 2)
		return END_OF_DATA;

    char sNewWord[1024];
	if (iWordLen >= sizeof(sNewWord) / sizeof(sNewWord[0]))
	{
		iWordLen = sizeof(sNewWord) / sizeof(sNewWord[0]) - 1;
		sWord[iWordLen] = 0;
	}

	while (iWordLen > 0 && !IsAlpha(sWord[iWordLen - 1]))
		iWordLen--;
	if (iWordLen < 2)
		return END_OF_DATA;
	sWord[iWordLen] = 0;

	for (int i = iWordLen; --i >= 0; )
		sWord[i] = ToLowerCase(sWord[i]);
    
    //cut one char "s" or "d"
    if (sWord[iWordLen-1]=='s' || (!memcmp(&sWord[iWordLen-2],"ed",2))) 
	{
		strcpy(sNewWord,sWord);
		sNewWord[iWordLen-1]='\0'; // cut "s" or "d"
		RETURN_VALID_IDENT(firstident(sNewWord, substring));
	}
    
    //cut "ly", "ed", "er"
    if (iWordLen>2 && (!memcmp(&sWord[iWordLen-2],"ly",2) 
			|| !memcmp(&sWord[iWordLen-2],"ed",2)
			|| !memcmp(&sWord[iWordLen-2],"er",2)))
	{
		strcpy(sNewWord,sWord);
		sNewWord[iWordLen-2]='\0';  
		RETURN_VALID_IDENT(firstident(sNewWord, substring));

		if(iWordLen>5 && (sNewWord[iWordLen-3]==sNewWord[iWordLen-4])
		   && !IsVowel(sNewWord[iWordLen-4]) && IsVowel(sNewWord[iWordLen-5]))
		{		  
			sNewWord[iWordLen-3]='\0';
		}					                    	
		else if (sNewWord[iWordLen-3] == 'i')	
		{
			sNewWord[iWordLen-3]='y';
		}
		else
		    return END_OF_DATA;

		RETURN_VALID_IDENT(firstident(sNewWord, substring));
	}
    
    //cut "ing"
    if (iWordLen>3 && !memcmp(&sWord[iWordLen-3],"ing",3)) 
	{
		strcpy(sNewWord,sWord);
		sNewWord[iWordLen-3]='\0';
		RETURN_VALID_IDENT(firstident(sNewWord, substring));

	    strcat(sNewWord,"e");
		RETURN_VALID_IDENT(firstident(sNewWord, substring));

		if(iWordLen>6 && (sNewWord[iWordLen-4]==sNewWord[iWordLen-5]) &&
		   !IsVowel(sNewWord[iWordLen-5]) && IsVowel(sNewWord[iWordLen-6])) 
		{
			sNewWord[iWordLen-4]='\0';
			RETURN_VALID_IDENT(firstident(sNewWord, substring));
		}

    }

    //cut two char "es"
    if (iWordLen>3 && (!memcmp(&sWord[iWordLen-2],"es",2) && 
	   (sWord[iWordLen-3] == 's' || sWord[iWordLen-3] == 'x' || sWord[iWordLen-3] == 'o'
	    || (iWordLen >4 && sWord[iWordLen-3] == 'h' && (sWord[iWordLen-4] == 'c' || sWord[iWordLen-4] == 's')))))
	{
		strcpy(sNewWord,sWord);
		sNewWord[iWordLen-2]='\0';
		RETURN_VALID_IDENT(firstident(sNewWord, substring));
    }    
    
    // cut "ies", add "y".
    if (iWordLen>3 && !memcmp(&sWord[iWordLen-3],"ies",3))
	{
		strcpy(sNewWord,sWord);
		sNewWord[iWordLen-3]='\0';
		strcat(sNewWord,"y"); 
		RETURN_VALID_IDENT(firstident(sNewWord, substring));
    }
    
    // cut "est".
    if(iWordLen>3 && !memcmp(&sWord[iWordLen-3],"est", 3))
	{
		strcpy(sNewWord,sWord);
		sNewWord[iWordLen-3]='\0';
		RETURN_VALID_IDENT(firstident(sNewWord, substring));

		if(iWordLen>6 && (sNewWord[iWordLen-4]==sNewWord[iWordLen-5]) &&
		   !IsVowel(sNewWord[iWordLen-5]) && IsVowel(sNewWord[iWordLen-6])) 
		{
			sNewWord[iWordLen-4]='\0';
		}
		else if (sNewWord[iWordLen-4] == 'i')	
		{
			sNewWord[iWordLen-4]='y';
		}
		else
		    return END_OF_DATA;

		RETURN_VALID_IDENT(firstident(sNewWord, substring));	
    }

    sWord[stem(sWord, 0, iWordLen - 1) + 1] = 0;
    RETURN_VALID_IDENT(firstident(sWord, substring));	

    return END_OF_DATA;
}

void myrusdict::CommonConstruct()
{
	lookupChars = g_lookupChars;
	numSymbols = NUM_SYMBOLS;
#ifdef COMPATIBLE
	defCumFreqs = g_tableRussianFrequencies;
#else
	defCumFreqs = g_tableUniversal + NUM_SYMBOLS - 2;
#endif //COMPATIBLE
}

void myrusdict::expfunc(char *dest, mystring& s)
{
	BYTE sb[BUFFER_SIZE];
	if (cumFreqs)
		myexp(sb, s, cumFreqs, NUM_SYMBOLS, true);
	else
		myexp(sb, s, defCumFreqs, NUM_SYMBOLS, false);
	for(int i=0;i<sb[0];i++) 
	{
		char ch = sb[i+1];
		if (ch > 36) 
			dest[i] = '?';
		else if (ch > 4) 
			dest[i] = BYTE(ch + (UseCapitalLetters()? 187 : 219));
		else dest[i]=achar[ch];
	}
	dest[sb[0]]='\0';
}
