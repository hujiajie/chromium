// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebProcessMemoryDump_h
#define WebProcessMemoryDump_h

#include "WebCommon.h"
#include "WebMemoryAllocatorDump.h"
#include "WebMemoryDumpProvider.h"
#include "WebString.h"
#include "base/trace_event/heap_profiler_allocation_context.h"

class SkTraceMemoryDump;

namespace base {

class DiscardableMemory;

namespace trace_event {

class ProcessMemoryDump;
class TraceEventMemoryOverhead;

} // namespace trace_event
} // namespace base

namespace blink {

// A container which holds all the dumps for the various allocators for a given
// process. Embedders of WebMemoryDumpProvider are expected to populate a
// WebProcessMemoryDump instance with the stats of their allocators.
class BLINK_PLATFORM_EXPORT WebProcessMemoryDump {
public:
    virtual ~WebProcessMemoryDump();

    // Creates a new MemoryAllocatorDump with the given name and returns the
    // empty object back to the caller. |absoluteName| uniquely identifies the
    // dump within the scope of a ProcessMemoryDump. It is possible to express
    // nesting by means of a slash-separated path naming (e.g.,
    // "allocator_name/arena_1/subheap_X").
    // |guid| is  an optional identifier, unique among all processes within the
    // scope of a global dump. This is only relevant when using
    // addOwnershipEdge(). If omitted, it will be automatically generated.
    virtual WebMemoryAllocatorDump* createMemoryAllocatorDump(const WebString& absoluteName, WebMemoryAllocatorDumpGuid) = 0;

    virtual WebMemoryAllocatorDump* createMemoryAllocatorDump(const WebString& absoluteName) = 0;

    // Gets a previously created MemoryAllocatorDump given its name.
    virtual WebMemoryAllocatorDump* getMemoryAllocatorDump(const WebString& absoluteName) const = 0;

    // Removes all the WebMemoryAllocatorDump(s) contained in this instance.
    // This WebProcessMemoryDump can be safely reused as if it was new once this
    // method returns.
    virtual void clear() = 0;

    // Merges all WebMemoryAllocatorDump(s) contained in |other| inside this
    // WebProcessMemoryDump, transferring their ownership to this instance.
    // |other| will be an empty WebProcessMemoryDump after this method returns
    // and can be reused as if it was new.
    virtual void takeAllDumpsFrom(WebProcessMemoryDump* other) = 0;

    // Adds an ownership relationship between two MemoryAllocatorDump(s) with
    // the semantics: |source| owns |target|, and has the effect of attributing
    // the memory usage of |target| to |source|. |importance| is optional and
    // relevant only for the cases of co-ownership, where it acts as a z-index:
    // the owner with the highest importance will be attributed |target|'s
    // memory.
    virtual void addOwnershipEdge(WebMemoryAllocatorDumpGuid source, WebMemoryAllocatorDumpGuid target, int importance) = 0;

    virtual void addOwnershipEdge(WebMemoryAllocatorDumpGuid source, WebMemoryAllocatorDumpGuid target) = 0;

    // Utility method to add a suballocation relationship with the following
    // semantics: |source| is suballocated from |target_node_name|.
    // This creates a child node of |target_node_name| and adds an ownership
    // edge between |source| and the new child node. As a result, the UI will
    // not account the memory of |source| in the target node.
    virtual void addSuballocation(WebMemoryAllocatorDumpGuid source, const WebString& targetNodeName) = 0;

    // Returns the SkTraceMemoryDump proxy interface that can be passed to Skia
    // to dump into this WebProcessMemoryDump. Multiple SkTraceMemoryDump
    // objects can be created using this method. The created dumpers are owned
    // by WebProcessMemoryDump and cannot outlive the WebProcessMemoryDump
    // object owning them. |dumpNamePrefix| is prefix appended to each dump
    // created by the SkTraceMemoryDump implementation, if the dump should be
    // placed under different namespace and not "skia".
    virtual SkTraceMemoryDump* createDumpAdapterForSkia(const WebString& dumpNamePrefix) = 0;

    virtual blink::WebMemoryAllocatorDump* createDiscardableMemoryAllocatorDump(const std::string& name, base::DiscardableMemory*) = 0;

    // Dumps heap memory usage. |allocatorName| is used as an absolute name for
    // base::trace_event::ProcessMemoryDump::AddHeapDump.
    virtual void dumpHeapUsage(const base::hash_map<base::trace_event::AllocationContext, size_t>& bytesByContext, base::trace_event::TraceEventMemoryOverhead&, const char* allocatorName) = 0;
};

} // namespace blink

#endif // WebProcessMemoryDump_h
