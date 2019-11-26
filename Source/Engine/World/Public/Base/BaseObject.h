/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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
#include <Core/Public/Document.h>

struct SWeakRefCounter;


/**

ABaseObject

Base object class.
Cares of reference counting, garbage collecting and little basic functionality.

*/
class ANGIE_API ABaseObject : public ADummy {
    AN_CLASS( ABaseObject, ADummy )

    friend class AGarbageCollector;
    friend class AWeakReference;

public:
    /** Object unique identifier */
    const uint64_t Id;

    /** Serialize object to document data */
    virtual int Serialize( ADocument & _Doc );

    /** Load attributes form document data */
    void LoadAttributes( ADocument const & _Document, int _FieldsHead );

    /** Add reference */
    void AddRef();

    /** Remove reference */
    void RemoveRef();

    int GetRefCount() const { return RefCount; }

    /** Set object debug/editor or ingame name */
    void SetObjectName( AString const & _Name ) { Name = _Name; }

    /** Get object debug/editor or ingame name */
    AString const & GetObjectName() const { return Name; }

    /** Get object debug/editor or ingame name */
    const char * GetObjectNameCStr() const { return Name.CStr(); }

    /** Get total existing objects */
    static uint64_t GetTotalObjects() { return TotalObjects; }

    /** Set weakref counter. Used by TWeakRef */
    void SetWeakRefCounter( SWeakRefCounter * _RefCounter ) { WeakRefCounter = _RefCounter; }

    /** Get weakref counter. Used by TWeakRef */
    SWeakRefCounter * GetWeakRefCounter() { return WeakRefCounter; }

protected:
    ABaseObject();

    virtual ~ABaseObject();

private:

    /** Total existing objects */
    static uint64_t TotalObjects;

    /** Custom object name */
    AString Name;

    /** Current refs count for this object */
    int RefCount;

    SWeakRefCounter * WeakRefCounter;

    /** Used by garbage collector to add this object to remove list */
    ABaseObject * NextGarbageObject;
    ABaseObject * PrevGarbageObject;
};

/**

Utilites

*/
AN_FORCEINLINE bool IsSame( ABaseObject const * _First, ABaseObject const * _Second ) {
    return ( ( !_First && !_Second ) || (_First && _Second && _First->Id == _Second->Id) );
}

/**

AGarbageCollector

Cares of garbage collecting and removing

*/
class AGarbageCollector final {
    AN_FORBID_COPY( AGarbageCollector )

    friend class ABaseObject;

public:
    /** Initialize garbage collector */
    static void Initialize();

    /** Deinitialize garbage collector */
    static void Deinitialize();

    /** Deallocates all collected objects */
    static void DeallocateObjects();

private:
    /** Add object to remove it at next DeallocateObjects() call */
    static void AddObject( ABaseObject * _Object );
    static void RemoveObject( ABaseObject * _Object );

    static ABaseObject * GarbageObjects;
    static ABaseObject * GarbageObjectsTail;
};

/**

TRef

Shared pointer

*/
template< typename T >
class TRef final {
public:
    TRef() : Object( nullptr ) {}

    TRef( TRef< T > const & _Ref )
        : Object( _Ref.Object )
    {
        if ( Object ) {
            Object->AddRef();
        }
    }

    explicit TRef( T * _Object )
        : Object( _Object )
    {
        if ( Object ) {
            Object->AddRef();
        }
    }

    ~TRef() {
        if ( Object ) {
            Object->RemoveRef();
        }
    }

    T * GetObject() { return Object; }

    T const * GetObject() const { return Object; }

//    operator bool() const {
//        return Object != nullptr;
//    }

    operator T*() const {
        return Object;
    }

    T & operator *() const {
        AN_ASSERT_( Object, "TRef" );
        return *Object;
    }

    T * operator->() {
        AN_ASSERT_( Object, "TRef" );
        return Object;
    }

    T const * operator->() const {
        AN_ASSERT_( Object, "TRef" );
        return Object;
    }

    void Reset() {
        if ( Object ) {
            Object->RemoveRef();
            Object = nullptr;
        }
    }

    void operator=( TRef< T > const & _Ref ) {
        this->operator =( _Ref.Object );
    }

    void operator=( T * _Object ) {
        if ( Object == _Object ) {
            return;
        }
        if ( Object ) {
            Object->RemoveRef();
        }
        Object = _Object;
        if ( Object ) {
            Object->AddRef();
        }
    }
private:
    T * Object;
};

/**

TWeakRef

Weak pointer

*/

struct SWeakRefCounter {
    ABaseObject * Object;
    int RefCount;
};

class AWeakReference {
protected:
    AWeakReference() : WeakRefCounter( nullptr ) {}

    void ResetWeakRef( ABaseObject * _Object );

    void RemoveWeakRef();

    SWeakRefCounter * WeakRefCounter;

private:
    SWeakRefCounter * AllocateWeakRefCounter();

    void DeallocateWeakRefCounter( SWeakRefCounter * _Counter );
};

template< typename T >
class TWeakRef final : public AWeakReference {
public:
    TWeakRef() {}

    TWeakRef( TWeakRef< T > const & _Ref ) {
        ResetWeakRef( const_cast< T * >( _Ref.GetObject() ) );
    }

    TWeakRef( TRef< T > const & _Ref ) {
        ResetWeakRef( const_cast< T * >( _Ref.GetObject() ) );
    }

    explicit TWeakRef( T * _Object ) {
        ResetWeakRef( _Object );
    }

    ~TWeakRef() {
        RemoveWeakRef();
    }

    TRef< T > ToStrongRef() const {
        return TRef< T >( const_cast< T * >( GetObject() ) );
    }

    T * GetObject() {
        return WeakRefCounter ? static_cast< T * >( WeakRefCounter->Object ) : nullptr;
    }

    T const * GetObject() const {
        return WeakRefCounter ? static_cast< T * >( WeakRefCounter->Object ) : nullptr;
    }

//    operator bool() const {
//        return !IsExpired();
//    }

    operator T*() const {
        return const_cast< T * >( GetObject() );
    }

    T & operator *() const {
        AN_ASSERT_( !IsExpired(), "TWeakRef" );
        return *GetObject();
    }

    T * operator->() {
        AN_ASSERT_( !IsExpired(), "TWeakRef" );
        return GetObject();
    }

    T const * operator->() const {
        AN_ASSERT_( !IsExpired(), "TWeakRef" );
        return GetObject();
    }

    bool IsExpired() const {
        return !WeakRefCounter || static_cast< T * >( WeakRefCounter->Object ) == nullptr;
    }

    void Reset() {
        RemoveWeakRef();
    }

    void operator=( T * _Object ) {
        ResetWeakRef( _Object );
    }

    void operator=( TRef< T > const & _Ref ) {
        ResetWeakRef( const_cast< T * >( _Ref.GetObject() ) );
    }

    void operator=( TWeakRef< T > const & _Ref ) {
        ResetWeakRef( const_cast< T * >( _Ref.GetObject() ) );
    }
};

template< typename T >
AN_FORCEINLINE bool operator == ( TRef< T > const & _Ref, TRef< T > const & _Ref2 ) { return _Ref.GetObject() == _Ref2.GetObject(); }

template< typename T >
AN_FORCEINLINE bool operator != ( TRef< T > const & _Ref, TRef< T > const & _Ref2 ) { return _Ref.GetObject() != _Ref2.GetObject(); }

template< typename T >
AN_FORCEINLINE bool operator == ( TRef< T > const & _Ref, TWeakRef< T > const & _Ref2 ) { return _Ref.GetObject() == _Ref2.GetObject(); }

template< typename T >
AN_FORCEINLINE bool operator != ( TRef< T > const & _Ref, TWeakRef< T > const & _Ref2 ) { return _Ref.GetObject() != _Ref2.GetObject(); }

template< typename T >
AN_FORCEINLINE bool operator == ( TWeakRef< T > const & _Ref, TRef< T > const & _Ref2 ) { return _Ref.GetObject() == _Ref2.GetObject(); }

template< typename T >
AN_FORCEINLINE bool operator != ( TWeakRef< T > const & _Ref, TRef< T > const & _Ref2 ) { return _Ref.GetObject() != _Ref2.GetObject(); }

template< typename T >
AN_FORCEINLINE bool operator == ( TWeakRef< T > const & _Ref, TWeakRef< T > const & _Ref2 ) { return _Ref.GetObject() == _Ref2.GetObject(); }

template< typename T >
AN_FORCEINLINE bool operator != ( TWeakRef< T > const & _Ref, TWeakRef< T > const & _Ref2 ) { return _Ref.GetObject() != _Ref2.GetObject(); }



/**

TCallback

Template callback class

*/
template< typename T >
struct TCallback;

template< typename TReturn, typename... TArgs >
struct TCallback< TReturn( TArgs... ) > {
    TCallback() {}

    template< typename T >
    TCallback( T * _Object, TReturn ( T::*_Method )(TArgs...) )
        : Object( _Object )
        , Method( (void (ABaseObject::*)(TArgs...))_Method )
    {
    }

    template< typename T >
    void Set( T * _Object, TReturn ( T::*_Method )(TArgs...) ) {
        Object = _Object;
        Method = (void (ABaseObject::*)(TArgs...))_Method;
    }

    void Clear() {
        Object.Reset();
    }

    bool IsValid() const {
        return !Object.IsExpired();
    }

    void operator=( TCallback< TReturn( TArgs... ) > const & _Callback ) {
        Object = _Callback.Object;
        Method = _Callback.Method;
    }

    TReturn operator()( TArgs... _Args ) const {
        ABaseObject * pObject = Object;
        if ( pObject ) {
            return (pObject->*Method)(StdForward< TArgs >( _Args )...);
        }
        return TReturn();
    }

    ABaseObject * GetObject() { return Object.GetObject(); }

protected:
    TWeakRef< ABaseObject > Object;
    TReturn ( ABaseObject::*Method )(TArgs...);
};


/**

TEvent

*/
template< typename... TArgs >
struct TEvent {
    AN_FORBID_COPY( TEvent )

    using Callback = TCallback< void( TArgs... ) >;

    TEvent() {}

    ~TEvent() {
        RemoveAll();
    }

    template< typename T >
    void Add( T * _Object, void ( T::*_Method )(TArgs...) ) {
        // Add callback
        Callbacks.emplace_back( _Object, _Method );
    }

    template< typename T >
    void Remove( T * _Object ) {
        if ( !_Object ) {
            return;
        }
        for ( int i = Callbacks.size() - 1 ; i >= 0 ; i-- ) {
            Callback & callback = Callbacks[i];

            if ( callback.GetObject() == _Object ) {
                callback.Clear();
            }
        }
    }

    void RemoveAll() {
        Callbacks.clear();
    }

    bool HasCallbacks() const {
        return !Callbacks.empty();
    }

    operator bool() const {
        return HasCallbacks();
    }

    void Dispatch( TArgs... _Args ) {
        for ( int i = 0 ; i < Callbacks.size() ; ) {
            if ( Callbacks[ i ].IsValid() ) {
                // Invoke
                Callbacks[ i++ ]( StdForward< TArgs >( _Args )... );
            } else {
                // Cleanup
                Callbacks.erase( Callbacks.begin() + i );
            }
        }
    }

    template< typename TConditionalCallback >
    void DispatchConditional( TConditionalCallback const & _Condition, TArgs... _Args ) {
        for ( int i = 0 ; i < Callbacks.size() ; ) {
            if ( Callbacks[ i ].IsValid() ) {
                // Invoke
                if ( _Condition() ) {
                    Callbacks[ i++ ]( StdForward< TArgs >( _Args )... );
                }
            } else {
                // Cleanup
                Callbacks.erase( Callbacks.begin() + i );
            }
        }
    }

private:
    TStdVector< Callback > Callbacks;
};
