/////////////////////////////////////////////////////////////////////////////
// Name:        hashmap.cpp
// Purpose:     wxHashMap implementation
// Author:      Mattia Barbon
// Modified by:
// Created:     29/01/2002
// RCS-ID:      $Id$
// Copyright:   (c) Mattia Barbon
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "hashmap.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "wx/hashmap.h"

/* FYI: This is the "One-at-a-Time" algorithm by Bob Jenkins */
/* from requirements by Colin Plumb. */
/* (http://burtleburtle.net/bob/hash/doobs.html) */
/* adapted from Perl sources ( hv.h ) */
unsigned long wxStringHash::wxCharStringHash( const wxChar* k )
{
    unsigned long hash = 0;

    while( *k )
    {
        hash += *k++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);

    return hash + (hash << 15);
}

#if wxUSE_UNICODE
unsigned long wxStringHash::charStringHash( const char* k )
{
    unsigned long hash = 0;

    while( *k )
    {
        hash += *k++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);

    return hash + (hash << 15);
}
#endif

/* from SGI STL */
const unsigned long _wxHashTableBase2::ms_primes[prime_count] =
{
    7ul,          13ul,         29ul,
    53ul,         97ul,         193ul,       389ul,       769ul,
    1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
    49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
    1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
    50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
    1610612741ul, 3221225473ul, 4294967291ul
};

unsigned long _wxHashTableBase2::GetNextPrime( unsigned long n )
{
    const unsigned long* ptr = &ms_primes[0];
    for( size_t i = 0; i < prime_count; ++i, ++ptr )
    {
        if( n < *ptr )
            return *ptr;
    }

    /* someone might try to alloc a 2^32-element hash table */
    wxFAIL_MSG( _T("hash table too big?") );

    /* quiet warning */
    return 0;
}

unsigned long _wxHashTableBase2::GetPreviousPrime( unsigned long n )
{
    const unsigned long* ptr = &ms_primes[prime_count - 1];

    for( size_t i = 0; i < prime_count; ++i, --ptr )
    {
        if( n > *ptr )
            return *ptr;
    }

    /* quiet warning */
    return 1;
}

void _wxHashTableBase2::DeleteNodes( size_t buckets,
                                     _wxHashTable_NodeBase** table,
                                     NodeDtor dtor )
{
    size_t i;

    for( i = 0; i < buckets; ++i )
    {
        _wxHashTable_NodeBase* node = table[i];
        _wxHashTable_NodeBase* tmp;

        while( node )
        {
            tmp = node->m_nxt;
            dtor( node );
            node = tmp;
        }
    }

    memset( table, 0, buckets * sizeof(void*) );
}

void _wxHashTableBase2::CopyHashTable( _wxHashTable_NodeBase** srcTable,
                                       size_t srcBuckets,
                                       _wxHashTableBase2* dst,
                                       _wxHashTable_NodeBase** dstTable,
                                       size_t dstBuckets,
                                       BucketFromNode func, ProcessNode proc )
{
    // for compatibility with wxHashTable (to avoid reimplementig it
    // from scratch), we need to preserve the order of nodes in a
    // source bucket when copying the table, hence, to avoid
    // allocating an auxiliary table we use a circular list for each
    // bucket, and we keep the *tail* of each list in dstTable[i], to
    // be able to append nodes in O(1) time. Wen we're done copying,
    // we adjust dstTable[i] to point at the head of the list and we
    // break the circular list into a linear one.
    size_t i;

    for( i = 0; i < srcBuckets; ++i )
    {
        _wxHashTable_NodeBase* nextnode;

        for( _wxHashTable_NodeBase* node = srcTable[i]; node; node = nextnode )
        {
            size_t bucket = func( dst, node );

            nextnode = node->m_nxt;
            _wxHashTable_NodeBase* newnode = proc( node );
            if( dstTable[bucket] )
            {
                newnode->m_nxt = dstTable[bucket]->m_nxt; // head of the list
                dstTable[bucket]->m_nxt = newnode;
                dstTable[bucket] = newnode;
            }
            else
                dstTable[bucket] = newnode->m_nxt = newnode;
        }
    }

    for( i = 0; i < dstBuckets; ++i )
    {
        if( dstTable[i] )
        {
            _wxHashTable_NodeBase* tmp = dstTable[i];
            dstTable[i] = dstTable[i]->m_nxt;
            tmp->m_nxt = NULL;
        }
    }
}

_wxHashTable_NodeBase* _wxHashTableBase2::DummyProcessNode(_wxHashTable_NodeBase* node)
{
    return node;
}

