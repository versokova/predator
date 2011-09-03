/*
 * Copyright (C) 2009-2011 Kamil Dudka <kdudka@redhat.com>
 *
 * This file is part of predator.
 *
 * predator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * predator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with predator.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef H_GUARD_SYMUTIL_H
#define H_GUARD_SYMUTIL_H

/**
 * @file symutil.hh
 * some generic utilities working on top of a symbolic heap
 * @todo API documentation
 */

#include "config.h"

#include <cl/code_listener.h>
#include <cl/clutil.hh>

#include "symheap.hh"
#include "util.hh"

#include <set>

#include <boost/foreach.hpp>
#include <boost/static_assert.hpp>

inline TValId boolToVal(const bool b) {
    return (b)
        ? VAL_TRUE
        : VAL_FALSE;
}

bool numFromVal(long *pDst, const SymHeap &, const TValId);

void moveKnownValueToLeft(const SymHeapCore &sh, TValId &valA, TValId &valB);

bool valInsideSafeRange(const SymHeapCore &sh, TValId val);

TObjId translateObjId(
        SymHeap                 &dst,
        SymHeap                 &src,
        const TValId            dstRootAt,
        const TObjId            srcObj);

inline TValId valOfPtrAt(SymHeap &sh, TValId at) {
    bool exclusive;
    const TObjId ptr = sh.ptrAt(at, &exclusive);
    const TValId val = sh.valueOf(ptr);

    if (0 < ptr && exclusive)
        sh.objReleaseId(ptr);

    return val;
}

inline TValId valOfPtrAt(SymHeap &sh, TValId at, TOffset off) {
    const TValId ptrAt = sh.valByOffset(at, off);
    return valOfPtrAt(sh, ptrAt);
}

inline bool isVarAlive(const SymHeap &sh, const TValId at) {
    TObjList live;
    sh.gatherLiveObjects(live, at);
    return !live.empty()
        || sh.pointedByCount(at);
}

inline bool isVarAlive(SymHeap &sh, const CVar &cv) {
    const TValId at = sh.addrOfVar(cv);
    return isVarAlive(sh, at);
}

void getPtrValues(TValList &dst, SymHeap &heap, TValId at);

inline TValId nextRootObj(SymHeap &sh, TValId root, TOffset offNext) {
    CL_BREAK_IF(sh.valOffset(root));
    const TValId valNext = valOfPtrAt(sh, root, offNext);
    return sh.valRoot(valNext);
}

template <class TDst, typename TInserter>
void gatherProgramVarsCore(
        TDst                    &dst,
        const SymHeap           &sh,
        const bool              liveOnly,
        TInserter               ins)
{
    TValList live;
    sh.gatherRootObjects(live, isProgramVar);

    BOOST_FOREACH(const TValId root, live) {
        if (VAL_ADDR_OF_RET == root)
            continue;

        if (!liveOnly || isVarAlive(sh, root))
            (dst.*ins)(sh.cVarByRoot(root));
    }
}

inline void gatherProgramVars(
        TCVarList               &dst,
        const SymHeap           &sh,
        const bool              liveOnly = false)
{
    void (TCVarList::*ins)(const CVar &) = &TCVarList::push_back;
    gatherProgramVarsCore(dst, sh, liveOnly, ins);
}

inline void gatherProgramVars(
        TCVarSet                &dst,
        const SymHeap           &sh,
        const bool              liveOnly = false)
{
    typedef std::pair<TCVarSet::iterator, bool> TRet;
    TRet (TCVarSet::*ins)(const CVar &) = &TCVarSet::insert;
    gatherProgramVarsCore(dst, sh, liveOnly, ins);
}

/// take the given visitor through all live pointers
template <class THeap, class TVisitor, typename TMethod>
bool /* complete */ traverseCore(
        THeap                       &sh,
        const TValId                at,
        TVisitor                    &visitor,
        TMethod                     method)
{
    // check that we got a valid root object
    CL_BREAK_IF(!isPossibleToDeref(sh.valTarget(at)));
    const TValId rootAt = sh.valRoot(at);
    const TOffset offRoot = sh.valOffset(at);

    TObjList objs;
    (sh.*method)(objs, rootAt);
    BOOST_FOREACH(const TObjId obj, objs) {
        const TOffset off = sh.valOffset(sh.placedAt(obj));
        if (off < offRoot)
            // do not go above the starting point
            continue;

        if (!visitor(sh, obj))
            // traversal cancelled by visitor
            return false;
    }

    // done
    return true;
}

/// take the given visitor through all live pointers
template <class THeap, class TVisitor>
bool /* complete */ traverseLivePtrs(
        THeap                       &sh,
        const TValId                rootAt,
        TVisitor                    &visitor)
{
    return traverseCore(sh, rootAt, visitor, &SymHeap::gatherLivePointers);
}

/// take the given visitor through all live objects
template <class THeap, class TVisitor>
bool /* complete */ traverseLiveObjs(
        THeap                       &sh,
        const TValId                rootAt,
        TVisitor                    &visitor)
{
    return traverseCore(sh, rootAt, visitor, &SymHeap::gatherLiveObjects);
}

/// take the given visitor through all uniform blocks
template <class THeap, class TVisitor>
bool /* complete */ traverseUniformBlocks(
        THeap                       &sh,
        const TValId                root,
        TVisitor                    &visitor)
{
    // check that we got a valid root object
    CL_BREAK_IF(sh.valOffset(root) || !isPossibleToDeref(sh.valTarget(root)));

    TUniBlockMap bMap;
    sh.gatherUniformBlocks(bMap, root);
    BOOST_FOREACH(TUniBlockMap::const_reference bItem, bMap) {
        if (!visitor(sh, /* UniformBlock */ bItem.second))
            // traversal cancelled by visitor
            return false;
    }

    // done
    return true;
}

/// take the given visitor through all live objects object-wise
template <unsigned N, class THeap, class TVisitor>
bool /* complete */ traverseLiveObjsGeneric(
        THeap                *const heaps[N],
        const TValId                at[N],
        TVisitor                    &visitor)
{
    // collect the starting points
    TValId roots[N];
    TOffset offs[N];
    for (unsigned i = 0; i < N; ++i) {
        SymHeap &sh = *heaps[i];

        const TValId addr = at[i];
        roots[i] = sh.valRoot(addr);
        offs[i] = sh.valOffset(addr);
    }

    // collect all live objects from everywhere
    typedef std::pair<TOffset, TObjType> TItem;
    std::set<TItem> all;
    for (unsigned i = 0; i < N; ++i) {
        SymHeap &sh = *heaps[i];
        const TValId root = roots[i];
        if (root < 0)
            continue;

        TObjList objs;
        sh.gatherLiveObjects(objs, root);
        BOOST_FOREACH(const TObjId obj, objs) {
            const TValId addr = sh.placedAt(obj);
            const TOffset off = sh.valOffset(addr) - offs[i];
            if (off < 0)
                // do not go above the starting point
                continue;

            const TObjType clt = sh.objType(obj);
            const TItem item(off, clt);
            all.insert(item);
        }
    }

    // go through all live objects
    BOOST_FOREACH(const TItem &item, all) {
        const TOffset  off = item.first;
        const TObjType clt = item.second;

        TObjId objs[N];
        for (unsigned i = 0; i < N; ++i) {
            SymHeap &sh = *heaps[i];

            const TValId addr = sh.valByOffset(roots[i], offs[i] + off);
            objs[i] = sh.objAt(addr, clt);
        }

        if (!visitor(objs))
            // traversal cancelled by visitor
            return false;
    }

    // done
    return true;
}

/// take the given visitor through all live objects object-wise
template <unsigned N, class THeap, class TVisitor>
bool /* complete */ traverseLiveObjs(
        THeap                       &sh,
        const TValId                at[N],
        TVisitor                    &visitor)
{
    THeap *heaps[N];
    for (unsigned i = 0; i < N; ++i)
        heaps[i] = &sh;

    return traverseLiveObjsGeneric<N>(heaps, at, visitor);
}

/// (VAL_INVALID != pointingFrom) means 'pointing from anywhere'
void redirectRefs(
        SymHeap                 &sh,
        const TValId            pointingFrom,
        const TValId            pointingTo,
        const TValId            redirectTo);

/// take the given visitor through all live program variables in all heaps
template <unsigned N_DST, unsigned N_SRC, class THeap, class TVisitor>
bool /* complete */ traverseProgramVarsGeneric(
        THeap                *const heaps[N_DST + N_SRC],
        TVisitor                    &visitor)
{
    const unsigned N_TOTAL = N_DST + N_SRC;
    BOOST_STATIC_ASSERT(N_DST < N_TOTAL);

    // start with all program variables of the first SRC heap
    TCVarSet all;
    gatherProgramVars(all, *heaps[/* src0 */ N_DST]);

    // try to match variables from the other heaps if possible
    for (unsigned i = /* src1 */ 1 + N_DST; i < N_TOTAL; ++i) {
        const SymHeap &sh = *heaps[i];

        TValList live;
        sh.gatherRootObjects(live, isProgramVar);
        BOOST_FOREACH(const TValId root, live) {
            if (VAL_ADDR_OF_RET == root)
                continue;

            const CVar cv(sh.cVarByRoot(root));
            if (!insertOnce(all, cv))
                // matches seamlessly
                continue;

            const EValueTarget code = sh.valTarget(root);
            if (VT_ON_STACK == code)
                // local variable mismatch, giving up
                // FIXME: this behavior should be better documented
                return false;
        }
    }

    // go through all program variables
    BOOST_FOREACH(const CVar &cv, all) {
        TValId roots[N_TOTAL];
        for (unsigned i = 0; i < N_TOTAL; ++i) {
            SymHeap &sh = *heaps[i];
            roots[i] = sh.addrOfVar(cv);
        }

        if (!visitor(roots))
            // traversal cancelled by visitor
            return false;
    }

    // done
    return true;
}

#endif /* H_GUARD_SYMUTIL_H */
