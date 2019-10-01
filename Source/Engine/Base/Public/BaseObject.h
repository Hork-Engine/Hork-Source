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
#include <Engine/Core/Public/Document.h>

struct FWeakRefCounter;


/*

FBaseObject

Base object class.
Cares of reference counting, garbage collecting and little basic functionality.

*/
class ANGIE_API FBaseObject : public FDummy {
    AN_CLASS( FBaseObject, FDummy )

    friend class FGarbageCollector;
    friend class FWeakReference;

public:
    // Serialize object to document data
    virtual int Serialize( FDocument & _Doc );

    // Initialize default object representation
    void InitializeDefaultObject();

    // Initialize object from file
    virtual bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true );

    // Initialize internal resource
    virtual void InitializeInternalResource( const char * _InternalResourceName );

    // Load attributes form document data
    void LoadAttributes( FDocument const & _Document, int _FieldsHead );

    // Add reference
    void AddRef();

    // Remove reference
    void RemoveRef();

    virtual void SetName( FString const & _Name ) { Name = _Name; }

    FString const & GetName() const { return Name; }

    const char * GetNameConstChar() const { return Name.ToConstChar(); }

    // Get total existing objects
    static uint64_t GetTotalObjects() { return TotalObjects; }

    // Set weakref counter. Used by TWeakRef
    void SetWeakRefCounter( FWeakRefCounter * _RefCounter ) { WeakRefCounter = _RefCounter; }

    // Get weakref counter. Used by TWeakRef
    FWeakRefCounter * GetWeakRefCounter() { return WeakRefCounter; }

protected:
    FBaseObject();

    virtual ~FBaseObject();

    FString Name;

private:

    // Total existing objects
    static uint64_t TotalObjects;

    // Current refs count for this object
    int RefCount;

    FWeakRefCounter * WeakRefCounter;

    // Used by garbage collector to add this object to remove list
    FBaseObject * NextGarbageObject;
    FBaseObject * PrevGarbageObject;
};

/*

FGarbageCollector

Cares of garbage collecting and removing

*/
class FGarbageCollector final {
    AN_FORBID_COPY( FGarbageCollector )

public:
    // Initialize garbage collector
    static void Initialize();

    // Deinitialize garbage collector
    static void Deinitialize();

    // Add object to remove it at next DeallocateObjects() call
    static void AddObject( FBaseObject * _Object );
    static void RemoveObject( FBaseObject * _Object );

    // Deallocates all collected objects
    static void DeallocateObjects();

private:
    static FBaseObject * GarbageObjects;
    static FBaseObject * GarbageObjectsTail;
};

/*

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
        AN_ASSERT( Object, "TRef" );
        return *Object;
    }

    T * operator->() {
        AN_ASSERT( Object, "TRef" );
        return Object;
    }

    T const * operator->() const {
        AN_ASSERT( Object, "TRef" );
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

/*

TWeakRef

Weak pointer

*/

struct FWeakRefCounter {
    FBaseObject * Object;
    int RefCount;
};

class FWeakReference {
protected:
    FWeakReference() : WeakRefCounter( nullptr ) {}

    void ResetWeakRef( FBaseObject * _Object );

    void RemoveWeakRef();

    FWeakRefCounter * WeakRefCounter;

private:
    FWeakRefCounter * AllocateWeakRefCounter();

    void DeallocateWeakRefCounter( FWeakRefCounter * _Counter );
};

template< typename T >
class TWeakRef final : public FWeakReference {
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
        AN_ASSERT( !IsExpired(), "TWeakRef" );
        return *GetObject();
    }

    T * operator->() {
        AN_ASSERT( !IsExpired(), "TWeakRef" );
        return GetObject();
    }

    T const * operator->() const {
        AN_ASSERT( !IsExpired(), "TWeakRef" );
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



/*

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
        , Method( (void (FBaseObject::*)(TArgs...))_Method )
    {
    }

    template< typename T >
    void Set( T * _Object, TReturn ( T::*_Method )(TArgs...) ) {
        Object = _Object;
        Method = (void (FBaseObject::*)(TArgs...))_Method;
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
        FBaseObject * pObject = Object;
        if ( pObject ) {
            return (pObject->*Method)(StdForward< TArgs >( _Args )...);
        }
        return TReturn();
    }

    FBaseObject * GetObject() { return Object.GetObject(); }

private:
    TWeakRef< FBaseObject > Object;
    TReturn ( FBaseObject::*Method )(TArgs...);
};


/*

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

private:
    TVector< Callback > Callbacks;
};
