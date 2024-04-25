/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

This file is part of the Hork Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "GarbageCollector.h"
#include "IntrusiveLinkedListMacro.h"
#include "Profiler.h"

HK_NAMESPACE_BEGIN

GCObject*          GarbageCollector::m_GarbageObjects     = nullptr;
GCObject*          GarbageCollector::m_GarbageObjectsTail = nullptr;
TVector<GCObject*> GarbageCollector::m_KeepAlivePtrs;

GCObject::GCObject()
{
    GarbageCollector::AddObject(this);
}

GCObject::~GCObject()
{
    if (m_WeakRefCounter)
    {
        m_WeakRefCounter->RawPtr = nullptr;
    }
}

void GCObject::AddRef()
{
    HK_ASSERT_(m_RefCount != -666, "Calling AddRef() in destructor");
    ++m_RefCount;
    if (m_RefCount == 1)
    {
        GarbageCollector::RemoveObject(this);
    }
}

void GCObject::RemoveRef()
{
    HK_ASSERT_(m_RefCount != -666, "Calling RemoveRef() in destructor");
    if (--m_RefCount == 0)
    {
        GarbageCollector::AddObject(this);
        return;
    }
    HK_ASSERT(m_RefCount > 0);
}

void GarbageCollector::AddObject(GCObject* pObject)
{
    INTRUSIVE_ADD(pObject, m_NextGarbageObject, m_PrevGarbageObject, m_GarbageObjects, m_GarbageObjectsTail)
}

void GarbageCollector::RemoveObject(GCObject* pObject)
{
    INTRUSIVE_REMOVE(pObject, m_NextGarbageObject, m_PrevGarbageObject, m_GarbageObjects, m_GarbageObjectsTail)
}

void GarbageCollector::Shutdown()
{
    ClearPointers();
    DeallocateObjects();
    m_KeepAlivePtrs.Free();
}

void GarbageCollector::DeallocateObjects()
{
    HK_PROFILER_EVENT("Garbage collector");

    while (m_GarbageObjects)
    {
        GCObject* object = m_GarbageObjects;

        // Mark RefCount to prevent using of AddRef/RemoveRef in the object destructor
        object->m_RefCount = -666;

        RemoveObject(object);

        delete object;
    }

    m_GarbageObjectsTail = nullptr;

    ClearPointers();
}

void GarbageCollector::KeepPointerAlive(GCObject* pObject)
{
    m_KeepAlivePtrs.Add(pObject);
    pObject->AddRef();
}

void GarbageCollector::ClearPointers()
{
    for (GCObject* ptr : m_KeepAlivePtrs)
    {
        ptr->RemoveRef();
    }
    m_KeepAlivePtrs.Clear();
}

HK_NAMESPACE_END
