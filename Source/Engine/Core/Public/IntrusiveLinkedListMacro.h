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

/*

Intrusive linked list macro

*/

#define IntrusiveIsInList( _object, _next, _prev, _head, _tail ) \
    ( _object->_prev || _object->_next || _head == _object )

#define IntrusiveAddToList( _object, _next, _prev, _head, _tail ) \
{ \
    _object->_prev = _tail; \
    _object->_next = nullptr; \
    _tail = _object; \
    if ( _object->_prev ) { \
        _object->_prev->_next = _object; \
    } else { \
        _head = _object; \
    } \
}

#define IntrusiveRemoveFromList( _object, _next, _prev, _head, _tail ) \
{ \
    auto * __next = _object->_next; \
    auto * __prev = _object->_prev; \
     \
    if ( __next || __prev || _object == _head ) { \
        if ( __next ) { \
            __next->_prev = __prev; \
        } else { \
            _tail = __prev; \
        } \
        if ( __prev ) { \
            __prev->_next = __next; \
        } else { \
            _head = __next; \
        } \
        _object->_next = nullptr; \
        _object->_prev = nullptr; \
    } \
}