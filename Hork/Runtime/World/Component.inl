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

HK_NAMESPACE_BEGIN

HK_FORCEINLINE ComponentHandle Component::GetHandle() const
{
    return m_Handle;
}

HK_FORCEINLINE GameObject* Component::GetOwner()
{
    return m_Owner;
}

HK_FORCEINLINE GameObject const* Component::GetOwner() const
{
    return m_Owner;
}

HK_FORCEINLINE ComponentManagerBase* Component::GetManager()
{
    return m_Manager;
}

HK_FORCEINLINE bool Component::IsDynamic() const
{
    return m_Flags.IsDynamic;
}

HK_FORCEINLINE bool Component::IsInitialized() const
{
    return m_Flags.IsInitialized;
}

HK_NAMESPACE_END
