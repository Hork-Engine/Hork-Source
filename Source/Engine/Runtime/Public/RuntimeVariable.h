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

#include <Core/Public/String.h>
#include <Core/Public/CompileTimeString.h>

enum ERuntimeVariableFlags
{
    VAR_LATCHED     = AN_BIT(0),
    VAR_READONLY    = AN_BIT(1),
    VAR_NOSAVE      = AN_BIT(2),
    VAR_CHEAT       = AN_BIT(3),
    VAR_SERVERONLY  = AN_BIT(4),
    VAR_NOINGAME    = AN_BIT(5),

    // Internal
    VAR_MODIFIED    = AN_BIT(6)
};

class ARuntimeVariable final {
    AN_FORBID_COPY( ARuntimeVariable )

public:
    template< char... NameChars >
    ARuntimeVariable( TCompileTimeString<NameChars...> const & _Name )
      : ARuntimeVariable( _Name.CStr(), "0", 0, "" ) {}

    template< char... NameChars, char... ValueChars >
    ARuntimeVariable( TCompileTimeString<NameChars...> const & _Name, TCompileTimeString<ValueChars...> const & _Value )
      : ARuntimeVariable( _Name.CStr(), _Value.CStr(), 0, "" ) {}

    template< char... NameChars, char... ValueChars >
    ARuntimeVariable( TCompileTimeString<NameChars...> const & _Name, TCompileTimeString<ValueChars...> const & _Value, uint16_t _Flags )
      : ARuntimeVariable( _Name.CStr(), _Value.CStr(), _Flags, "" ) {}

    template< char... NameChars, char... ValueChars, char... CommentChars >
    ARuntimeVariable( TCompileTimeString<NameChars...> const & _Name, TCompileTimeString<ValueChars...> const & _Value, uint16_t _Flags, TCompileTimeString<CommentChars...> const & _Comment )
      : ARuntimeVariable( _Name.CStr(), _Value.CStr(), _Flags, _Comment.CStr() ) {}

    ~ARuntimeVariable();

    char const * GetName() const { return Name; }

    char const * GetComment() const { return Comment; }

    bool CanChangeValue() const;

    const char * GetDefaultValue() const { return DefaultValue; }

    AString const & GetValue() const { return Value; }

    AString const & GetLatchedValue() const { return LatchedValue; }

    bool GetBool() const { return !!I32; }

    int32_t GetInteger() const { return I32; }

    float GetFloat() const { return F32; }

    bool IsModified() const { return !!(Flags & VAR_MODIFIED); }

    void MarkModified() { Flags |= VAR_MODIFIED; }

    void UnmarkModified() { Flags &= ~VAR_MODIFIED; }

    bool IsReadOnly() const { return !!(Flags & VAR_READONLY); }

    bool IsNoSave() const { return !!(Flags & VAR_NOSAVE); }

    bool IsCheat() const { return !!(Flags & VAR_CHEAT); }

    bool IsServerOnly() const { return !!(Flags & VAR_SERVERONLY); }

    bool IsNoInGame() const { return !!(Flags & VAR_NOINGAME); }

    void SetString( const char * _String );

    void SetString( AString const & _String );

    void SetBool( bool _Bool );

    void SetInteger( int32_t _Integer );

    void SetFloat( float _Float );

    void ForceString( const char * _String );

    void ForceString( AString const & _String );

    void ForceBool( bool _Bool );

    void ForceInteger( int32_t _Integer );

    void ForceFloat( float _Float );

    void SetLatched();

    void Print();

    ARuntimeVariable const & operator=( const char * _String ) { SetString( _String ); return *this; }
    ARuntimeVariable const & operator=( AString const & _String ) { SetString( _String ); return *this; }
    ARuntimeVariable const & operator=( bool _Bool ) { SetBool( _Bool ); return *this; }
    ARuntimeVariable const & operator=( int32_t _Integer ) { SetInteger( _Integer ); return *this; }
    ARuntimeVariable const & operator=( float _Float ) { SetFloat( _Float ); return *this; }
    operator bool() const { return GetBool(); }

    ARuntimeVariable * GetNext() { return Next; }

    static ARuntimeVariable * GlobalVariableList();

    static ARuntimeVariable * FindVariable( const char * _Name );

    // Internal
    static void AllocateVariables();
    static void FreeVariables();

private:
    ARuntimeVariable( const char * _Name,
                      const char * _Value,
                      uint16_t _Flags,
                      const char * _Comment );

    char const * const Name;
    char const * const DefaultValue;
    char const * const Comment;
    AString Value;
    AString LatchedValue;
    int32_t I32;
    float F32;
    uint16_t Flags;
    ARuntimeVariable * Next;
};
