/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include "Std.h"

/**

TRef

Shared pointer

*/
template< typename T >
class TRef final {
public:
    using ReferencedType = T;

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
    void * Object;
    int RefCount;
};

class AWeakReference {
protected:
    AWeakReference() : WeakRefCounter( nullptr ) {}

//    void ResetWeakRef( ABaseObject * _Object );

//    void RemoveWeakRef();

    template< typename T >
    void ResetWeakRef( T * _Object ) {
        T * Cur = WeakRefCounter ? (T*)WeakRefCounter->Object : nullptr;

        if ( Cur == _Object ) {
            return;
        }

        RemoveWeakRef< T >();

        if ( !_Object ) {
            return;
        }

        WeakRefCounter = _Object->GetWeakRefCounter();
        if ( !WeakRefCounter ) {
            WeakRefCounter = AllocateWeakRefCounter();
            WeakRefCounter->Object = _Object;
            WeakRefCounter->RefCount = 1;
            _Object->SetWeakRefCounter( WeakRefCounter );
        } else {
            WeakRefCounter->RefCount++;
        }
    }

    template< typename T >
    void RemoveWeakRef() {
        if ( WeakRefCounter ) {
            if ( --WeakRefCounter->RefCount == 0 ) {
                if ( WeakRefCounter->Object ) {
                    ((T*)WeakRefCounter->Object)->SetWeakRefCounter( nullptr );
                }
                DeallocateWeakRefCounter( WeakRefCounter );
            }
            WeakRefCounter = nullptr;
        }
    }

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
        RemoveWeakRef< T >();
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
        RemoveWeakRef< T >();
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

template< typename T, typename... _Args >
inline TRef< T > MakeRef( _Args &&... __args )
{
    return TRef< T >( new T( StdForward< _Args >( __args )... ) );
}
