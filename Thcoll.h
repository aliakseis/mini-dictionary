#pragma once

typedef bool (*ccTestFunc)( void*, void* );
typedef void (*ccAppFunc)( void*, void* );

enum
{
	ccNotFound = -1,
	maxCollectionSize = ((65536uL - 16)/sizeof( void* )),
};

class THCollection
{
public:
    THCollection(int aLimit);
    ~THCollection();

    void* at( int index );

    void atFree( int index );
    void atRemove( int index );
    void removeAll();
    void freeAll();

    void atInsert( int index, void* item );
    void atPut( int index, void* item );

    static void error( int code, int info );

    int firstThat( ccTestFunc Test, void* arg );
    int lastThat( ccTestFunc Test, void* arg );
    void forEach( ccAppFunc action, void* arg );

    void pack();
    void setLimit( int aLimit );

    int getCount()	{ return count; }

protected:
	enum { FAST_BUFFER_SIZE = 8 };

	virtual void DestructElement(void* item) = 0;

    void* *items;
    int count;
    int limit;

	void* buffer[FAST_BUFFER_SIZE];
};

class THSortedCollection: public THCollection
{
public:
    THSortedCollection( int aLimit, bool duplicates_ = true) 
	: THCollection(aLimit), duplicates(duplicates_) {}

    bool search( void* key, int &index );

    int insert( void* item );

    bool duplicates;

protected:
    virtual void* keyOf( void* item ) { return item; }
    virtual int compare( void* key1, void* key2 ) 
	{
		if (key1 < key2)
			return -1;
		else 
			return (key1 > key2)? 1 : 0;
	}
};
