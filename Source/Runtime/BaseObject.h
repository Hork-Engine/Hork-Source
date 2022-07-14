/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#pragma once

#include "Factory.h"

#include <Core/Document.h>
#include <Core/Ref.h>
#include <Containers/Vector.h>

struct SWeakRefCounter;


/**

ABaseObject

Base object class.
Cares of reference counting, garbage collecting and little basic functionality.

*/
class ABaseObject : public ADummy
{
    HK_CLASS(ABaseObject, ADummy)

    friend class AGarbageCollector;
    friend class AWeakReference;

public:
    /** Object unique identifier */
    const uint64_t Id;

    ABaseObject();
    virtual ~ABaseObject();

    void SetProperties(TStringHashMap<AString> const& Properties);

    bool SetProperty(AStringView PropertyName, AStringView PropertyValue);

    //bool SetProperty(AStringView PropertyName, AVariant const& PropertyValue);

    AProperty const* FindProperty(AStringView PropertyName, bool bRecursive) const;

    void GetProperties(TPodVector<AProperty const*>& Properties, bool bRecursive = true) const;

    /** Add reference */
    void AddRef();

    /** Remove reference */
    void RemoveRef();

    int GetRefCount() const { return m_RefCount; }

    /** Set object debug/editor or ingame name */
    void SetObjectName(AStringView Name) { m_Name = Name; }

    /** Get object debug/editor or ingame name */
    AString const& GetObjectName() const { return m_Name; }

    /** Set weakref counter. Used by TWeakRef */
    void SetWeakRefCounter(SWeakRefCounter* _RefCounter) { m_WeakRefCounter = _RefCounter; }

    /** Get weakref counter. Used by TWeakRef */
    SWeakRefCounter* GetWeakRefCounter() { return m_WeakRefCounter; }

    /** Get total existing objects */
    static uint64_t GetTotalObjects() { return m_TotalObjects; }

    static ABaseObject* FindObject(uint64_t _Id);

    template <typename T>
    static T* FindObject(uint64_t _Id)
    {
        ABaseObject* object = FindObject(_Id);
        if (!object)
        {
            return nullptr;
        }
        if (object->FinalClassId() != T::ClassId())
        {
            return nullptr;
        }
        return static_cast<T*>(object);
    }

private:
    void SetProperties_r(AClassMeta const* Meta, TStringHashMap<AString> const& Properties);

    /** Custom object name */
    AString m_Name;

    /** Current refs count for this object */
    int m_RefCount{};

    SWeakRefCounter* m_WeakRefCounter{};

    /** Object global list */
    ABaseObject* m_NextObject{};
    ABaseObject* m_PrevObject{};

    /** Used by garbage collector to add this object to garbage list */
    ABaseObject* m_NextGarbageObject{};
    ABaseObject* m_PrevGarbageObject{};

    /** Total existing objects */
    static uint64_t m_TotalObjects;

    static ABaseObject* m_Objects;
    static ABaseObject* m_ObjectsTail;
};

/**

Utilites

*/
HK_FORCEINLINE bool IsSame(ABaseObject const* _First, ABaseObject const* _Second)
{
    return ((!_First && !_Second) || (_First && _Second && _First->Id == _Second->Id));
}

/**

AGarbageCollector

Cares of garbage collecting and removing

*/
class AGarbageCollector final
{
    HK_FORBID_COPY(AGarbageCollector)

    friend class ABaseObject;

public:
    /** Deallocates all collected objects */
    static void DeallocateObjects();

private:
    /** Add object to remove it at next DeallocateObjects() call */
    static void AddObject(ABaseObject* _Object);
    static void RemoveObject(ABaseObject* _Object);

    static ABaseObject* m_GarbageObjects;
    static ABaseObject* m_GarbageObjectsTail;
};

/**

TCallback

Template callback class

*/
template <typename T>
struct TCallback;

template <typename TReturn, typename... TArgs>
struct TCallback<TReturn(TArgs...)>
{
    TCallback() {}

    template <typename T>
    TCallback(T* _Object, TReturn (T::*_Method)(TArgs...)) :
        Object(_Object), Method((void (ABaseObject::*)(TArgs...))_Method)
    {
    }

    template <typename T>
    TCallback(TRef<T>& _Object, TReturn (T::*_Method)(TArgs...)) :
        Object(_Object), Method((void (ABaseObject::*)(TArgs...))_Method)
    {
    }

    template <typename T>
    void Set(T* _Object, TReturn (T::*_Method)(TArgs...))
    {
        Object = _Object;
        Method = (void (ABaseObject::*)(TArgs...))_Method;
    }

    void Clear()
    {
        Object.Reset();
    }

    bool IsValid() const
    {
        return !Object.IsExpired();
    }

    void operator=(TCallback<TReturn(TArgs...)> const& _Callback)
    {
        Object = _Callback.Object;
        Method = _Callback.Method;
    }

    TReturn operator()(TArgs... _Args) const
    {
        ABaseObject* pObject = Object;
        if (pObject)
        {
            return (pObject->*Method)(std::forward<TArgs>(_Args)...);
        }
        return TReturn();
    }

    ABaseObject* GetObject() { return Object.GetObject(); }

protected:
    TWeakRef<ABaseObject> Object;
    TReturn (ABaseObject::*Method)(TArgs...);
};


/**

TEvent

*/
template <typename... TArgs>
struct TEvent
{
    HK_FORBID_COPY(TEvent)

    using Callback = TCallback<void(TArgs...)>;

    TEvent() {}

    ~TEvent()
    {
        RemoveAll();
    }

    template <typename T>
    void Add(T* _Object, void (T::*_Method)(TArgs...))
    {
        // Add callback
        Callbacks.EmplaceBack(_Object, _Method);
    }

    template <typename T>
    void Remove(T* _Object)
    {
        if (!_Object)
        {
            return;
        }
        for (int i = Callbacks.Size() - 1; i >= 0; i--)
        {
            Callback& callback = Callbacks[i];

            if (callback.GetObject() == _Object)
            {
                callback.Clear();
            }
        }
    }

    void RemoveAll()
    {
        Callbacks.Clear();
    }

    bool HasCallbacks() const
    {
        return !Callbacks.IsEmpty();
    }

    operator bool() const
    {
        return HasCallbacks();
    }

    void Dispatch(TArgs... _Args)
    {
        for (int i = 0; i < Callbacks.Size();)
        {
            if (Callbacks[i].IsValid())
            {
                // Invoke
                Callbacks[i++](std::forward<TArgs>(_Args)...);
            }
            else
            {
                // Cleanup
                Callbacks.Erase(Callbacks.begin() + i);
            }
        }
    }

    template <typename TConditionalCallback>
    void DispatchConditional(TConditionalCallback const& _Condition, TArgs... _Args)
    {
        for (int i = 0; i < Callbacks.Size();)
        {
            if (Callbacks[i].IsValid())
            {
                // Invoke
                if (_Condition())
                {
                    Callbacks[i](std::forward<TArgs>(_Args)...);
                }
                i++;
            }
            else
            {
                // Cleanup
                Callbacks.Erase(Callbacks.begin() + i);
            }
        }
    }

private:
    TVector<Callback> Callbacks;
};
