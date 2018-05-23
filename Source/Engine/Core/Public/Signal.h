/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2018 Alexander Samusev.

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

#include <functional>
#include <map>

// A signal object may call multiple slots with the
// same signature. You can connect functions to the signal
// which will be called when the dispatch() method on the
// signal object is invoked. Any argument passed to dispatch()
// will be passed to the given functions.
// This class is based on code from http://simmesimme.github.io/tutorials/2015/09/20/signal-slot

template< typename... Args >
class TSignal {
public:
    TSignal();

    // Connects a method to this signal
    template< typename T >
    int ConnectMethod( T * _Inst, void (T::*_Func)(Args...) );

    // Connects a const method to this signal
    template< typename T >
    int ConnectMethod( T * _Inst, void (T::*_Func)(Args...) const );

    // Connects a std::function to the signal. The returned
    // value can be used to disconnect the function again
    int Connect( std::function< void( Args... ) > const & slot ) const;

    // Disconnects a previously connected function
    void Disconnect( int _Id ) const;

    // Disconnects all previously connected functions
    void DisconnectAll() const;

    // Calls all connected functions
    void Dispatch( Args... _Args );

    bool HasConnections() const;

    TSignal & operator=( TSignal const & _Other ) = delete;
    TSignal( TSignal const & _Other ) = delete;

private:
    mutable std::map< int, std::function< void(Args...) > > Slots;
    mutable int SlotId;
};

template< typename... Args >
TSignal< Args... >::TSignal() : SlotId(0) {}

template< typename... Args >
template< typename T >
int TSignal< Args... >::ConnectMethod( T * _Inst, void (T::*_Func)(Args...) ) {
    return Connect( [=](Args... args) {
        (_Inst->*_Func)(args...);
    } );
}

template< typename... Args >
template< typename T >
int TSignal< Args... >::ConnectMethod( T * _Inst, void (T::*_Func)(Args...) const ) {
    return Connect( [=](Args... args) {
        (_Inst->*_Func)(args...);
    } );
}

template< typename... Args >
int TSignal< Args... >::Connect( std::function< void( Args... ) > const & slot ) const {
    Slots.insert( std::make_pair( ++SlotId, slot ) );
    return SlotId;
}

template< typename... Args >
void TSignal< Args... >::Disconnect( int _Id ) const {
    Slots.erase( _Id );
}

template< typename... Args >
void TSignal< Args... >::DisconnectAll() const {
    Slots.clear();
}

template< typename... Args >
void TSignal< Args... >::Dispatch( Args... _Args ) {
    for ( auto it : Slots ) {
        it.second( _Args... );
    }
}

template< typename... Args >
bool TSignal< Args... >::HasConnections() const {
    return !Slots.empty();
}
