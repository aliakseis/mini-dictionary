#if !defined( __DICTLIB_H )
#define __DICTLIB_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <new>
#include <assert.h>
using std::nothrow;

#include "thcoll.h"


void GetExeDir(LPSTR szPath);


inline unsigned char ToLowerCase(char ch)
{
	return char(ch | 0x20);
}

inline bool IsAlpha(char ch)
{
	if (BYTE(ch) >= 192)
		return true;
	BYTE b = ToLowerCase(ch);
	return b >= 'a' && b <= 'z';
}


enum dictmodetype { none, memo, medi };

typedef unsigned long Ident;

const Ident EMPTY_STRING = 0xFFFFFFFE;
const Ident END_OF_DATA  = 0xFFFFFFFF;

enum { DIFF_FACTOR = 224 };

enum { 
	MYSTRING_SIZE = 128,
	MYSTRING_MAX  = MYSTRING_SIZE - 1
};

enum { 
	BUFFER_SIZE = 255,
	BUFFER_MAX  = BUFFER_SIZE - 1
};

typedef BYTE mystring[MYSTRING_SIZE];
typedef mystring mybuf[258];

typedef Ident asubident[258];

typedef BYTE tbuf[256 * MYSTRING_SIZE];

typedef BYTE hash_buf[256];


inline int GetNumPlanes(Ident mask)
{
	int numplanes = 0;
	if (mask >= 0x80000)
	{
		numplanes = 4;
		mask >>= 4;
	}
	if (mask >= 0x20000)
	{
		numplanes += 2;
		mask >>= 2;
	}
	if (mask >= 0x10000)
		numplanes += 1;
	return numplanes;
}

#pragma pack(push, 1)

enum {
	VERSION_PREHISTORIC = -1,
	VERSION_CURRENT = 0,
};

struct recbase 
{
	enum { FLAG_SEQUENTIAL_IDS = 0x80 };

	int get_version()
	{
		return (m_version > 1 || m_dataSize == 0 && m_idnr > 1)
			? VERSION_PREHISTORIC : m_version;
	}
	int get_datasize()  
	{ 
		return (VERSION_PREHISTORIC == get_version())
			? get_oldDatasize() : m_dataSize; 
	}
	int get_numplanes() 
	{ 
		return (VERSION_PREHISTORIC == get_version())
			? get_oldNumplanes() : m_numPlanes;
	}
	int get_idsize()	
	{ 
		return is_sequential()
			? sizeof(Ident) : get_idnr() * 2 + get_planessize(); 
	}

	int get_oldDatasize()  { return m_diskpos >> 19; }
	int get_oldNumplanes() { return (m_diskpos >> 17) & 3; }

	void set(int numplanes, int datasize, int nIds)	
	{
		m_version = VERSION_CURRENT;
		m_numPlanes = (unsigned char)numplanes;
		m_dataSize = (unsigned short)datasize;
		m_idnr = (BYTE) nIds;
	}

	void set_sequential()
	{
		m_numPlanes = FLAG_SEQUENTIAL_IDS;
	}
	bool is_sequential()
	{
		return (VERSION_PREHISTORIC != get_version()) 
			&& (FLAG_SEQUENTIAL_IDS & m_numPlanes);
	}

	int get_idnr()		
	{ 
		return (VERSION_PREHISTORIC == get_version())
			? (m_oldIdnr < 2 && get_oldDatasize() != 0)? m_oldIdnr + 256 : m_oldIdnr
			: (m_idnr < 2 && m_dataSize != 0)? m_idnr + 256 : m_idnr; 
	}

protected:
	int get_planesize() { return (get_idnr() + 7) >> 3; }
	int get_planessize(){ return get_planesize() * get_numplanes(); }

private:
	union
	{
		struct
		{
			unsigned char m_version;
			unsigned short m_dataSize;
			unsigned char m_idnr;
			unsigned char m_numPlanes;
		};
		struct
		{
			 unsigned char m_oldIdnr;
			 unsigned long m_diskpos;
		};
	};
};

struct get_helper 
{
	Ident id;
	void do_word(WORD w) { id = w; }
	void do_bit(int byte, int bit, int id_bit)
	{
		if (byte & bit)
			id |= id_bit;
	}
};

struct set_helper 
{
	Ident id;
	void do_word(WORD& w) { w = (WORD)id; }
	void do_bit(BYTE& byte, int bit, int id_bit)
	{
		if (id & id_bit)
			byte |= bit;
		else
			byte &= ~bit;
	}
};

struct bucket_rec : public recbase 
{

	tbuf buf;

	Ident get_id(int idx)
	{
		if (is_sequential())
		{
			BYTE* p = buf + buf[0] + 1;
			return *((Ident*)p) + idx;
		}
		get_helper h;
		HandleId(idx, h);
		return h.id;
	}
	void set_id(int idx, Ident id)
	{
		set_helper h;
		h.id = id;
		HandleId(idx, h);
	}
	void set_sequential_origin(Ident origin)
	{
		set_sequential();
		BYTE* p = buf + buf[0] + 1;
		*((Ident*)p) = origin;
	}

private:
	template<typename T> void HandleId(int idx, T& helper)
	{
		BYTE* p = buf + buf[0] + 1;
		helper.do_word(*((WORD*)p + idx));
		int numplanes = get_numplanes();
		if (numplanes > 0)
		{
			p += get_idnr() * sizeof(WORD) + (idx >> 3);
			int bit = 128 >> (idx & 7);
			int id_bit = 0x10000;
			int i = 0;
			for(;;)
			{
				helper.do_bit(*p, bit, id_bit);
				if (++i >= numplanes)
					break;
				p += get_planesize();
				id_bit <<= 1;
			}
		}
	}

};

struct myprefix : public recbase 
{
	BYTE b0;
};

#pragma pack(pop)

struct myinsertrec 
{
	mybuf mystrbuf;
	asubident myidbuf;
	int bufsize,bptr,bpid;
	int myp,myidp;
	mystring myssample;
	bool firstbuf;
};

class mycollection: public THSortedCollection
{
 public:
	   mycollection(int aLimit):
		  THSortedCollection(aLimit) {}
	   ~mycollection()
	   {
			freeAll();
			setLimit(0);
	   }
	   void atsubst(int i, void* p)
		{
			 delete[] (BYTE*)at(i);
			 atPut(i, p);
		}
 protected:
	  dictmodetype dictmode;
		void DestructElement(void* item) { delete[] (BYTE*)item; }
	  virtual void* keyOf(void FAR* item)
	  { return &((bucket_rec*) item)->buf; }
	  virtual int compare(void* key1,void* key2);
};

class mydict:public mycollection
{
 public:
	mydict(const char* pth);
	mydict() : mycollection(512) 
	{
		CommonConstruct();
		dictpath[0]='\0';
		dictmode=memo;
	}
	~mydict() 
	{ 
		free(); 
		if (bAutoDeleteCumFreqs)
			delete[] cumFreqs; 
	}
	void free();
	Ident memo_load();
	Ident medi_load();
	bool store(unsigned short flags_ = 0, bool bSequentialIds = false);
	void memo2medi();
	void medi2memo();
	virtual void expfunc(char *dest,mystring s);
	void findbyident(char *dest, Ident ident);
	void insertstr(char *ss, Ident ident);
	void deletebyident(Ident ident);
	void correctident(Ident oldidnt, Ident newidnt);
	void findstartrec(mybuf sbuf, asubident asidnt, char *ss, int& nCount, int& nPos);
	int firstbypos(mybuf sbuf,asubident asidnt,int pos);
	int findnextrec(mybuf sbuf,asubident asidnt);
	int findprevrec(mybuf sbuf,asubident asidnt,int vol);

	Ident firstident(const char *ss, char *substring = NULL);
	Ident nextident();
	void insertbyfind(Ident ident);
	int getcurp() { return curp; }

	virtual Ident first_similar(char *ss, char *substring)
	{
		return firstident(ss, substring);
	}

	virtual mydict *newsimilar() { return new(nothrow) mydict; }
	virtual bool IsCyrillic() {return FALSE;}

	void compress(Ident* pSubstIds = NULL);
	bool differs(unsigned int idx)
	{
		assert(idx <= 258);
		return diffs[idx];
	}
	bool MakeHash(const char* str, hash_buf pHash);
	long GetDataSize();

	int getMatchingCount(const char *s);

	unsigned short getFlags() { return flags; }
	bool is_sequential()
	{
		for (int i = 0; i < count; i++) 
			if (!((bucket_rec*)at(i))->is_sequential())
				return false;
		return true;
	}

 protected:
	char dictpath[_MAX_PATH];
	int curp;
	int startshift;
	myinsertrec *pins;
	long myoffset;
	int fhandle;

	const BYTE* lookupChars;

	int numSymbols;
	bool bAutoDeleteCumFreqs;
	BYTE* cumFreqs;
	const BYTE* defCumFreqs;
	long* offsets;

	bool diffs[258];

	unsigned short flags;

 private:
	void CommonConstruct()
	{
		lookupChars = NULL;
		numSymbols = 0;
		bAutoDeleteCumFreqs = true;
		cumFreqs = NULL;
		defCumFreqs = NULL;
		pins = NULL;
		offsets = NULL;
		flags = 0;
	}
	int subextract(BYTE* buf,mybuf psbuf,int mysize, int nVersion);
	int extractrec(mybuf psbuf, asubident pasidnt, int nr);
	void extractstr(mystring dest,int nr,int shift);

	void compfunc(mystring dest, const char *s);

	void insertrec(mybuf psbuf,asubident pasidnt, int bsize, int nr);

	void ReadCumFreqs(int f);
};

class myengdict:public mydict
{
 public:
	enum { NUM_SYMBOLS = 31 };
	myengdict(const char *pth):mydict(pth) { CommonConstruct(); }
	myengdict(myengdict* other):mydict() 
	{ 
		CommonConstruct(); 
		bAutoDeleteCumFreqs = false;
		cumFreqs = other->cumFreqs;
	}
	virtual void expfunc(char *dest,mystring s);

	virtual Ident first_similar(char *ss, char *substring);
	
	virtual mydict *newsimilar() { return new(nothrow) myengdict(this); }

 protected:
	static const BYTE g_lookupChars[256];

 private:
	void CommonConstruct();
};

class myrusdict:public mydict
{
 public:
	enum { NUM_SYMBOLS = 37 };
	myrusdict(const char *pth):mydict(pth) { CommonConstruct(); }
	myrusdict(myrusdict* other):mydict()
	{ 
		CommonConstruct(); 
		bAutoDeleteCumFreqs = false;
		cumFreqs = other->cumFreqs;
	}
	virtual void expfunc(char *dest,mystring s);
	virtual mydict *newsimilar() { return new(nothrow) myrusdict(this); }
	virtual bool IsCyrillic() { return TRUE; }

 protected:
	static const BYTE g_lookupChars[256];

 private:
	void CommonConstruct();
};

#endif
