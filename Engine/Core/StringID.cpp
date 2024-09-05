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

#include "StringID.h"
#include "Logger.h"

HK_NAMESPACE_BEGIN

StringID::Pool& StringID::Pool::sInstance()
{
    static Pool instance;
    return instance;
}

StringID::Pool::Pool()
{
    Insert("");
}

StringID::ID StringID::Pool::Insert(StringView str)
{
    MutexGuard guard(m_Mutex);

    auto it = m_Storage.Find(str);
    if (it != m_Storage.End())
        return it->second;
    
    size_t numStrings = m_Strings.Size();
    if (numStrings > Math::MaxValue<ID>())
        CoreApplication::sTerminateWithError("StringID::Pool::Insert: Pool overflow - too many strings\n");

    ID id = ID(numStrings);

    auto result = m_Storage.Insert(str, id);
    m_Strings.Add(result.first.get_node()->mValue.first);
    return id;
}

HK_NAMESPACE_END
