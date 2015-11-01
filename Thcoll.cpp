#include "stdafx.h"


#include <new>
using std::nothrow;

#include "thcoll.h"

#pragma intrinsic(memcpy)

THCollection::THCollection(int aLimit) :
    count( 0 ),
    items( 0 ),
    limit( 0 )
{
    setLimit( aLimit );
}

THCollection::~THCollection()
{
	count = 0;
    setLimit(0);
}

void* THCollection::at( int index )
{
    return (index>=0 && index<count)? items[index] : NULL;
}

void THCollection::atRemove( int index )
{
    if ( index >= count )
        error(1,0);
    count--;
    memmove( &items[index], &items[index+1], (count-index)*sizeof(void*) );
}

void THCollection::atFree( int index )
{
    DestructElement(at( index ));
    atRemove( index );
}

void THCollection::atInsert(int index, void* item)
{
    if ( index < 0 )
        error(1,0);
    if ( count == limit )
		setLimit((count + 1) * 3 / 2);

    memmove( &items[index+1], &items[index], (count-index)*sizeof(void*) );
    count++;

    items[index] = item;
}

void THCollection::atPut( int index, void* item )
{
    if ( index >= count )
		error(1,0);

    items[index] = item;
}

void THCollection::removeAll()
{
    count = 0;
}

void THCollection::error( int code, int )
{
    exit(212 - code);
}

int THCollection::firstThat( ccTestFunc Test, void* arg )
{
   for( int i = 0; i < count; i++ )
   {
		void* data = items[i];
		if(data && Test( data, arg ))
			 return i;
   }
   return -1;
}

int THCollection::lastThat( ccTestFunc Test, void* arg )
{
   for( int i = count; i > 0; i-- )
   {
		void* data = items[i-1];
		if(data && Test( data, arg ))
			 return i-1;
   }
   return -1;
}

void THCollection::forEach( ccAppFunc action, void* arg )
{
   for( int i = 0; i < count; i++ )
   {
		 void* data = items[i];
		 if(data) 
		 {
			 action( data, arg );
		 }
   }
}

void THCollection::freeAll()
{
    for( int i =  0; i < count; i++ )
		DestructElement(at(i));
    count = 0;
}

void THCollection::pack()
{
    void* *curDst = items;
    void* *curSrc = items;
    void* *last = items + count;
    while( curSrc < last )
	{
		if ( *curSrc != 0 )
			 *curDst++ = *curSrc;
		++curSrc;
	}
}

void THCollection::setLimit(int aLimit)
{
    if ( aLimit < count )
		aLimit =  count;
    if ( aLimit > maxCollectionSize)
		aLimit = maxCollectionSize;
    if ( aLimit != limit )
	{
		void* *aItems;
		if (aLimit == 0 )
			 aItems = 0;
		else
		 {
			 aItems = (aLimit <= FAST_BUFFER_SIZE) 
				 ? buffer
				 : new(nothrow) void*[aLimit];

			 if ( count !=  0 )
				memcpy( aItems, items, count*sizeof(void*) );
		 }

		if (items != buffer)
			delete[] items;

		items =  aItems;
		limit =  aLimit;
	}
}

int THSortedCollection::insert( void* item )
{
    int  i=-1;
	if ( search( keyOf(item), i ) == 0 || duplicates )//order dependency!
			atInsert( i, item );                         //must do Search
    return i;
}

bool THSortedCollection::search( void* key, int& index )
{
    int l = 0;
    int h = count - 1;
    int c = 0;
    bool res = false;
    while( l <= h )
	{
		int i = (l +  h) >> 1;
		void* data = items[i];
		 c = compare( keyOf( data ), key );
		if ( c < 0 )
			 l = i + 1;
		else
		 {
			 h = i - 1;
			 if ( c == 0 )
			{
				res = true;
				if ( !duplicates )
					 l = i;
			}
		 }
	}
    index = l;
    return res;
}
