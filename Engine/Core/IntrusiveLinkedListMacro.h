/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

/*

Intrusive linked list macro

*/

#define INTRUSIVE_EXISTS(_object, _next, _prev, _head, _tail) \
    (_object->_prev || _object->_next || _head == _object)

#define INTRUSIVE_ADD(_object, _next, _prev, _head, _tail) \
    {                                                      \
        _object->_prev = _tail;                            \
        _object->_next = nullptr;                          \
        _tail          = _object;                          \
        if (_object->_prev)                                \
        {                                                  \
            _object->_prev->_next = _object;               \
        }                                                  \
        else                                               \
        {                                                  \
            _head = _object;                               \
        }                                                  \
    }

#define INTRUSIVE_ADD_UNIQUE(_object, _next, _prev, _head, _tail)   \
    {                                                               \
        if (!INTRUSIVE_EXISTS(_object, _next, _prev, _head, _tail)) \
        {                                                           \
            INTRUSIVE_ADD(_object, _next, _prev, _head, _tail);     \
        }                                                           \
    }

#define INTRUSIVE_REMOVE(_object, _next, _prev, _head, _tail) \
    {                                                         \
        auto* __next = _object->_next;                        \
        auto* __prev = _object->_prev;                        \
                                                              \
        if (__next || __prev || _object == _head)             \
        {                                                     \
            if (__next)                                       \
            {                                                 \
                __next->_prev = __prev;                       \
            }                                                 \
            else                                              \
            {                                                 \
                _tail = __prev;                               \
            }                                                 \
            if (__prev)                                       \
            {                                                 \
                __prev->_next = __next;                       \
            }                                                 \
            else                                              \
            {                                                 \
                _head = __next;                               \
            }                                                 \
            _object->_next = nullptr;                         \
            _object->_prev = nullptr;                         \
        }                                                     \
    }

#define INTRUSIVE_MERGE(_next, _prev, _head1, _tail1, _head2, _tail2) \
    {                                                                 \
        if (_head2)                                                   \
        {                                                             \
            if (_tail1)                                               \
            {                                                         \
                _tail1->_next = (_head2);                             \
            }                                                         \
            _head2->_prev = _tail1;                                   \
            _tail1        = _tail2;                                   \
            if (!_head1)                                              \
            {                                                         \
                _head1 = _head2;                                      \
            }                                                         \
            _head2 = _tail2 = nullptr;                                \
        }                                                             \
    }

#define INTRUSIVE_FOREACH(object, head_or_tail, next_or_prev) \
    for (auto* object = head_or_tail; object; object = object->next_or_prev)


HK_NAMESPACE_BEGIN

template <typename T>
struct TLink
{
    T* Next{};
    T* Prev{};
};

template<typename T>
struct TListIterator;

template<typename T>
struct TListReverseIterator;

template <typename T>
struct TList : public Noncopyable
{
    friend struct TListIterator<T>;
    friend struct TListReverseIterator<T>;

private:
    T* Head{};
    T* Tail{};

public:
    void Add(T* node)
    {
        INTRUSIVE_ADD_UNIQUE(node, Link.Next, Link.Prev, Head, Tail);
    }

    void Remove(T* node)
    {
        INTRUSIVE_REMOVE(node, Link.Next, Link.Prev, Head, Tail)
    }

    void Merge(TList<T>& other)
    {
        INTRUSIVE_MERGE(Link.Next, Link.Prev, Head, Tail, other.Head, other.Tail);
    }

    bool IsExists(T* node) const
    {
        return INTRUSIVE_EXISTS(node, Link.next, Link.prev, Head, Tail);
    }

    ~TList()
    {
        T* next;
        for (auto* node = Head; node; node = next)
        {
            next = node->Link.Next;
            node->Link.Next = nullptr;
            node->Link.Prev = nullptr;
        }
    }
};

template<typename T>
struct TListIterator
{
    TListIterator(TList<T>& list) :
        Node(list.Head)
    {}

    operator bool() const
    {
        return Node != nullptr;
    }

    void operator++()
    {
        Node = Node->Link.Next;
    }

    void operator++(int)
    {
        Node = Node->Link.Next;
    }

    T* operator*() const
    {
        return Node;
    }

    T* operator->() const
    {
        return Node;
    }

    T* get()
    {
        return Node;
    }

private:
    T* Node;
};

template<typename T>
struct TListReverseIterator
{
    TListReverseIterator(TList<T>& list) :
        Node(list.Tail)
    {}

    operator bool() const
    {
        return Node != nullptr;
    }

    void operator++()
    {
        Node = Node->Link.Prev;
    }

    void operator++(int)
    {
        Node = Node->Link.Prev;
    }

    T* operator*() const
    {
        return Node;
    }

    T* operator->() const
    {
        return Node;
    }

private:
    T* Node;
};

HK_NAMESPACE_END
