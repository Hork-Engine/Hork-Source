#pragma once

#include <Engine/Core/Handle.h>
#include <Engine/Core/Allocators/PageAllocator.h>
#include <Engine/Core/Containers/PageStorage.h>
#include <Engine/Core/CoreApplication.h>

HK_NAMESPACE_BEGIN

/*
TODO:

Add:
enum class ObjectStorageType
{
    Compact,
    Sparse
};

*/

template <typename T>
class ObjectStorage final
{
public:
                    ObjectStorage() = default;
                    ObjectStorage(ObjectStorage const& rhs) = delete;
                    ObjectStorage(ObjectStorage&& rhs) noexcept;
                    ~ObjectStorage();

    ObjectStorage&  operator=(ObjectStorage& rhs) = delete;
    ObjectStorage&  operator=(ObjectStorage&& rhs) noexcept;

    Handle32<T>     CreateObject(T*& newObject);

    template <typename HandleFetcher>
    void            DestroyObject(Handle32<T> handle);

    template <typename HandleFetcher>
    void            DestroyObject(Handle32<T> handle, T*& movedObject);

    T*              GetObject(Handle32<T> handle);

    T const*        GetObject(Handle32<T> handle) const;

    uint32_t        Size() const;

    T*              At(uint32_t index);

    T const*        At(uint32_t index) const;

    template <typename Visitor>
    void            Iterate(Visitor& visitor);

    template <typename Visitor>
    void            IterateBatches(Visitor& visitor);

    struct Iterator
    {
    private:
        uint32_t                m_Index;
        uint32_t                m_EndIndex;
        PageStorage<T>&         m_PageStorage;

    public:
                    Iterator(uint32_t startIndex, uint32_t endIndex, PageStorage<T>& pageStorage);

        T&          operator*();

        T*          operator->();

                    operator T*();

        bool        IsValid() const;

        uint32_t    GetIndex() const;

        void        operator++();
    };

    struct ConstIterator
    {
    private:
        uint32_t                m_Index;
        uint32_t                m_EndIndex;
        PageStorage<T> const&   m_PageStorage;

    public:
                    ConstIterator(uint32_t startIndex, uint32_t endIndex, PageStorage<T> const& pageStorage);

        T const&    operator*() const;

        T const*    operator->() const;

                    operator T const*() const;

        bool        IsValid() const;

        uint32_t    GetIndex() const;

        void        operator++();
    };

    Iterator        GetObjects();
    ConstIterator   GetObjects() const;

    TVector<T*> const& GetRandomAccessTable() const
    {
        return m_RandomAccess;
    }

private:
    PageStorage<T>          m_PageStorage;
    TVector<T*>             m_RandomAccess;
    uint32_t                m_FreeListHead = 0;
};

HK_NAMESPACE_END

#include "ObjectStorage.inl"
