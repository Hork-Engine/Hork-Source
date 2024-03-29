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

#pragma once

#include "Factory.h"
#include "GarbageCollector.h"

HK_NAMESPACE_BEGIN

/**

BaseObject

Base object class.

*/
class BaseObject : public GCObject
{
public:
    typedef BaseObject                                          ThisClass;
    typedef Allocators::HeapMemoryAllocator<HEAP_WORLD_OBJECTS> Allocator;
    class ThisClassMeta : public ClassMeta
    {
    public:
        ThisClassMeta() :
            ClassMeta(ClassMeta::DummyFactory(), "BaseObject"s, nullptr)
        {}

        BaseObject* CreateInstance() const override
        {
            return NewObj<ThisClass>();
        }
    };

    _HK_GENERATED_CLASS_BODY()

    /** Object unique identifier */
    const uint64_t Id;

    BaseObject();
    ~BaseObject();

    void SetProperties(TStringHashMap<String> const& Properties);

    bool SetProperty(StringView PropertyName, StringView PropertyValue);

    //bool SetProperty(StringView PropertyName, Variant const& PropertyValue);

    Property const* FindProperty(StringView PropertyName, bool bRecursive) const;

    void GetProperties(PropertyList& Properties, bool bRecursive = true) const;

    /** Get total existing objects */
    static uint64_t GetTotalObjects() { return m_TotalObjects; }

    static BaseObject* FindObject(uint64_t _Id);

    template <typename T>
    static T* FindObject(uint64_t _Id)
    {
        BaseObject* object = FindObject(_Id);
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
    void SetProperties_r(ClassMeta const* Meta, TStringHashMap<String> const& Properties);

    /** Object global list */
    BaseObject* m_NextObject{};
    BaseObject* m_PrevObject{};

    /** Total existing objects */
    static uint64_t m_TotalObjects;

    static BaseObject* m_Objects;
    static BaseObject* m_ObjectsTail;
};

/**

Utilites

*/

template <typename T>
T* Upcast(BaseObject* pObject)
{
    if (pObject && pObject->FinalClassMeta().IsSubclassOf<T>())
    {
        return static_cast<T*>(pObject);
    }
    return nullptr;
}

template <typename T>
T const* Upcast(BaseObject const* pObject)
{
    if (pObject && pObject->FinalClassMeta().IsSubclassOf<T>())
    {
        return static_cast<T const*>(pObject);
    }
    return nullptr;
}

/**

TCallback

Template callback class

*/
template <typename T>
struct TCallback;

template <typename TReturn, typename... TArgs>
struct TCallback<TReturn(TArgs...)>
{
    TCallback() = default;

    template <typename T>
    TCallback(T* _Object, TReturn (T::*_Method)(TArgs...)) :
        m_Object(_Object), m_Method((void(GCObject::*)(TArgs...))_Method)
    {
    }

    template <typename T>
    TCallback(TRef<T>& _Object, TReturn (T::*_Method)(TArgs...)) :
        m_Object(_Object), m_Method((void(GCObject::*)(TArgs...))_Method)
    {
    }

    template <typename T>
    void Set(T* _Object, TReturn (T::*_Method)(TArgs...))
    {
        m_Object = _Object;
        m_Method = (void(GCObject::*)(TArgs...))_Method;
    }

    void Clear()
    {
        m_Object.Reset();
    }

    bool IsValid() const
    {
        return !m_Object.IsExpired();
    }

    void operator=(TCallback<TReturn(TArgs...)> const& _Callback)
    {
        m_Object = _Callback.m_Object;
        m_Method = _Callback.m_Method;
    }

    TReturn operator()(TArgs... _Args) const
    {
        GCObject* pObject = m_Object;
        if (pObject)
        {
            return (pObject->*m_Method)(std::forward<TArgs>(_Args)...);
        }
        return TReturn();
    }

    GCObject* GetObject() { return m_Object.GetObject(); }

protected:
    TWeakRef<GCObject> m_Object;
    TReturn (GCObject::*m_Method)(TArgs...);
};


/**

TEvent

*/
template <typename... TArgs>
struct TEvent : public Noncopyable
{
    using Callback = TCallback<void(TArgs...)>;

    TEvent() = default;

    TEvent(TEvent&& rhs) :
        m_Callbacks(std::move(rhs.m_Callbacks))
    {}

    TEvent& operator=(TEvent&& rhs)
    {
        m_Callbacks = std::move(rhs);
        return *this;
    }

    template <typename T>
    void Add(T* _Object, void (T::*_Method)(TArgs...))
    {
        // Add callback
        m_Callbacks.EmplaceBack(_Object, _Method);
    }

    template <typename T>
    void Remove(T* _Object)
    {
        if (!_Object)
        {
            return;
        }
        for (int i = (int)m_Callbacks.Size() - 1; i >= 0; i--)
        {
            Callback& callback = m_Callbacks[i];

            if (callback.GetObject() == _Object)
            {
                callback.Clear();
            }
        }
    }

    void RemoveAll()
    {
        m_Callbacks.Clear();
    }

    bool HasCallbacks() const
    {
        return !m_Callbacks.IsEmpty();
    }

    operator bool() const
    {
        return HasCallbacks();
    }

    void Dispatch(TArgs... _Args)
    {
        for (int i = 0; i < m_Callbacks.Size();)
        {
            if (m_Callbacks[i].IsValid())
            {
                // Invoke
                m_Callbacks[i++](std::forward<TArgs>(_Args)...);
            }
            else
            {
                // Cleanup
                m_Callbacks.Erase(m_Callbacks.begin() + i);
            }
        }
    }

    template <typename TConditionalCallback>
    void DispatchConditional(TConditionalCallback const& _Condition, TArgs... _Args)
    {
        for (int i = 0; i < m_Callbacks.Size();)
        {
            if (m_Callbacks[i].IsValid())
            {
                // Invoke
                if (_Condition())
                {
                    m_Callbacks[i](std::forward<TArgs>(_Args)...);
                }
                i++;
            }
            else
            {
                // Cleanup
                m_Callbacks.Erase(m_Callbacks.begin() + i);
            }
        }
    }

private:
    TVector<Callback> m_Callbacks;
};

HK_NAMESPACE_END
