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

#include <Engine/Core/Public/Document.h>
#include <Engine/Core/Public/Alloc.h>
#include <Engine/Core/Public/Logger.h>

FDocument::FDocument() {

}

FDocument::~FDocument() {
    Allocator::Inst().Dealloc( Fields );
    Allocator::Inst().Dealloc( Values );
}

void FDocument::Clear() {
    FieldsHead = -1;
    FieldsTail = -1;
    FieldsCount = 0;
    ValuesCount = 0;
}

int FDocument::AllocateField() {
    if ( FieldsCount == FieldsMemReserved ) {
        FieldsMemReserved = FieldsMemReserved ? FieldsMemReserved<<1 : 1024;
        Fields = ( FDocumentField * )Allocator::Inst().Extend< 1 >( Fields, FieldsCount * sizeof( FDocumentField ), sizeof( FDocumentField ) * FieldsMemReserved, true );
    }
    FDocumentField * field = &Fields[ FieldsCount ];
    field->ValuesHead = -1;
    field->ValuesTail = -1;
    field->Next = -1;
    field->Prev = -1;
    FieldsCount++;
    return FieldsCount - 1;
}

int FDocument::AllocateValue() {
    if ( ValuesCount == ValuesMemReserved ) {
        ValuesMemReserved = ValuesMemReserved ? ValuesMemReserved<<1 : 1024;
        Values = ( FDocumentValue * )Allocator::Inst().Extend1( Values, ValuesCount * sizeof( FDocumentValue ), sizeof( FDocumentValue ) * ValuesMemReserved, true );
    }
    FDocumentValue * value = &Values[ ValuesCount ];
    value->FieldsHead = -1;
    value->FieldsTail = -1;
    value->Next = -1;
    value->Prev = -1;
    ValuesCount++;
    return ValuesCount - 1;
}

FTokenBuffer::FTokenBuffer() {
    Cur = Start = (char *)"";
    LineNumber = 1;
    bInSitu = true;
}

FTokenBuffer::~FTokenBuffer() {
    if ( !bInSitu ) {
        Allocator::Inst().Dealloc( Start );
    }
}

void FTokenBuffer::Initialize( const char * _String, bool _InSitu ) {
    Deinitialize();

    bInSitu = _InSitu;
    if ( bInSitu ) {
        Start = const_cast< char * >( _String );
    } else {
        Start = ( char * )Allocator::Inst().Alloc1( FString::Length( _String ) + 1 );
        FString::Copy( Start, _String );
    }
    Cur = Start;
    LineNumber = 1;
}

void FTokenBuffer::Deinitialize() {
    if ( !bInSitu ) {
        Allocator::Inst().Dealloc( Start );
        Cur = Start = (char *)"";
        LineNumber = 1;
        bInSitu = true;
    }
}

FDocumentProxyBuffer::FDocumentProxyBuffer()
    : StringList( nullptr ) {

}

FDocumentProxyBuffer::~FDocumentProxyBuffer() {
    FStringList * next;
    for ( FStringList * node = StringList ; node ; node = next ) {
        next = node->Next;
        node->~FStringList();
        Allocator::Inst().Dealloc( node );
    }
}

FString & FDocumentProxyBuffer::NewString() {
    void * pMemory = Allocator::Inst().Alloc1( sizeof( FStringList ) );
    FStringList * node = new ( pMemory ) FStringList;
    node->Next = StringList;
    StringList = node;
    return node->Str;
}

FString & FDocumentProxyBuffer::NewString( const char * _String ) {
    void * pMemory = Allocator::Inst().Alloc1( sizeof( FStringList ) );
    FStringList * node = new ( pMemory ) FStringList( _String );
    node->Next = StringList;
    StringList = node;
    return node->Str;
}

FString & FDocumentProxyBuffer::NewString( const FString & _String ) {
    void * pMemory = Allocator::Inst().Alloc1( sizeof( FStringList ) );
    FStringList * node = new ( pMemory ) FStringList( _String );
    node->Next = StringList;
    StringList = node;
    return node->Str;
}

static const char * TokenType[] = {
    "Unknown token",
    "EOF",
    "Bracket",
    "Field",
    "String"
};

bool FToken::CompareToString( const char * _Str ) const {
    const char * p = Begin;
    while ( *_Str && p < End ) {
        if ( *_Str != *p ) {
            return false;
        }
        _Str++;
        p++;
    }
    return ( *_Str || p != End ) ? false : true;
}

void FToken::FromString( const char * _Str ) {
    Begin = _Str;
    End = Begin + FString::Length( _Str );
}

FString FToken::ToString() const {
    FString str;
    str.Resize( End - Begin );
    memcpy( str.ToPtr(), Begin, str.Length() );
    return str;
}

const char * FToken::NamedType() const {
    return TokenType[ Type ];
}

struct FTokenizer {
    FToken CurToken;

    const FToken & GetToken() const;
    void NextToken( FTokenBuffer & buffer );
};

static void SkipWhiteSpaces( FTokenBuffer & _Buffer ) {
    const char *& s = _Buffer.Cur;
    while ( *s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' ) {
        if ( *s == '\n' ) {
            _Buffer.LineNumber++;
        }
        s++;
    }
    if ( *s == '/' ) {
        if ( *(s+1) == '/' ) {
            s += 2;
            // go to next line
            while ( *s && *s != '\n' )
                s++;
            SkipWhiteSpaces( _Buffer );
            return;
        }
        if ( *(s+1) == '*' ) {
            s += 2;
            while ( *s ) {
                if ( *s == '\n' ) {
                    _Buffer.LineNumber++;
                } else if ( *s == '*' && *(s+1) == '/' ) {
                    s += 2;
                    SkipWhiteSpaces( _Buffer );
                    return;
                }
                s++;
            }
            GLogger( "Warning: unclosed comment /* */\n" );
            return;
        }
    }
}

const FToken & FTokenizer::GetToken() const {
    return CurToken;
}

void FTokenizer::NextToken( FTokenBuffer & _Buffer ) {
    // Skip white spaces, tabs and comments
    SkipWhiteSpaces( _Buffer );

    const char *& s = _Buffer.Cur;

    // Check string
    if ( *s == '\"' ) {
        s++;
        CurToken.Begin = s;
        for (;;) {
            if ( *s == '\"' && *(s-1) != '\\' ) {
                break;
            }

            if ( *s == 0 ) {
                // unexpected eof
                CurToken.Begin = CurToken.End = "";
                CurToken.Type = TT_Unknown;
                return;
            }
            if ( *s == '\n' ) {
                // unexpected eol
                CurToken.Begin = CurToken.End = "";
                CurToken.Type = TT_Unknown;
                return;
            }
            s++;
        }
        CurToken.End = s++;
        CurToken.Type = TT_String;
        return;
    }

    // Check brackets
    if ( *s == '{' || *s == '}' || *s == '[' || *s == ']' ) {
        CurToken.Begin = s;
        CurToken.End = ++s;
        CurToken.Type = TT_Bracket;
        return;
    }

    // Check field
    CurToken.Begin = s;
    while ( ( *s >= 'a' && *s <= 'z' ) || ( *s >= 'A' && *s <= 'Z' ) || ( *s >= '0' && *s <= '9' ) ) {
        s++;
    }
    CurToken.End = s;
    if ( CurToken.Begin == CurToken.End ) {
        if ( *s ) {
            GLogger.Print( "undefined symbols\n" );
            CurToken.Type = TT_Unknown;
        } else {
            CurToken.Type = TT_EOF;
        }
        return;
    } else {
        CurToken.Type = TT_Field;
    }
}

static bool Expect( int _Type, const FToken & _Token ) {
    if ( _Token.Type != _Type ) {
        GLogger.Printf( "unexpected %s found, expected %s\n", _Token.NamedType(), TokenType[_Type] );
        return false;
    }
    return true;
}

static int ParseObject( FTokenizer & _Tokenizer, FDocument & _Doc );
static int ParseField( FTokenizer & _Tokenizer, FDocument & _Doc, FToken & _FieldToken );

static void ParseArray( FTokenizer & _Tokenizer, FDocument & _Doc, int & _ArrayHead, int & _ArrayTail ) {
    _ArrayHead = -1;
    _ArrayTail = -1;

    while ( 1 ) {
        FToken token = _Tokenizer.GetToken();

        if ( token.Type == TT_Bracket ) {
            if ( *token.Begin == ']' ) {
                _Tokenizer.NextToken( _Doc.Buffer );
                if ( _ArrayHead == -1 ) {
                    GLogger.Print( "empty array\n" );
                }
                break;
            }
            if ( *token.Begin != '{' ) {
                GLogger.Printf( "unexpected bracket %c\n", *token.Begin );
                _ArrayHead = -1;
                break;
            }

            _Tokenizer.NextToken( _Doc.Buffer );

            // array element is object
            int value = ParseObject( _Tokenizer, _Doc );
            if ( value == -1 ) {
                _ArrayHead = -1;
                break;
            }
            _Doc.Values[ value ].Prev = _ArrayTail;
            if ( _Doc.Values[ value ].Prev != -1 ) {
                _Doc.Values[ _Doc.Values[ value ].Prev ].Next = value;
            } else {
                _ArrayHead = value;
            }
            _ArrayTail = value;
            continue;
        }

        if ( token.Type == TT_String ) {
            // array element is string

            int value = _Doc.AllocateValue();
            _Doc.Values[ value ].Type = FDocumentValue::T_String;
            _Doc.Values[ value ].Token = token;
            _Doc.Values[ value ].Prev = _ArrayTail;
            if ( _Doc.Values[ value ].Prev != -1 ) {
                _Doc.Values[ _Doc.Values[ value ].Prev ].Next = value;
            } else {
                _ArrayHead = value;
            }
            _ArrayTail = value;

            _Tokenizer.NextToken( _Doc.Buffer );

            continue;
        }

        GLogger.Printf( "unexpected %s\n", token.NamedType() );
        _ArrayHead = -1;
        break;
    }
}

static int ParseObject( FTokenizer & _Tokenizer, FDocument & _Doc ) {

    int value = _Doc.AllocateValue();
    _Doc.Values[ value ].Type = FDocumentValue::T_Object;

    while ( 1 ) {
        FToken token = _Tokenizer.GetToken();

        if ( token.Type == TT_Bracket ) {
            if ( *token.Begin == '}' ) {
                if ( _Doc.Values[ value ].FieldsTail == -1 ) {
                    GLogger.Print( "empty object\n" );
                    return -1;
                }
                _Tokenizer.NextToken( _Doc.Buffer );
                return value;
            }

            GLogger.Printf( "unexpected bracket %c\n", *token.Begin );
            return -1;
        }

        if ( !Expect( TT_Field, token ) ) {
            //error
            return -1;
        }

        _Tokenizer.NextToken( _Doc.Buffer );

        int field = ParseField( _Tokenizer, _Doc, token );
        if ( field == -1 ) {
            return -1;
        }
        _Doc.Fields[ field ].Prev = _Doc.Values[ value ].FieldsTail;
        if ( _Doc.Fields[ field ].Prev != -1 ) {
            _Doc.Fields[ _Doc.Fields[ field ].Prev ].Next = field;
        } else {
            _Doc.Values[ value ].FieldsHead = field;
        }
        _Doc.Values[ value ].FieldsTail = field;
    }

    return -1;
}

static int ParseField( FTokenizer & _Tokenizer, FDocument & _Doc, FToken & _FieldToken ) {
    FToken token = _Tokenizer.GetToken();

    if ( token.Type == TT_Bracket ) {
        if ( *token.Begin == '[' ) {
            // value is array
            _Tokenizer.NextToken( _Doc.Buffer );

            int arrayHead, arrayTail;
            ParseArray( _Tokenizer, _Doc, arrayHead, arrayTail );
            if ( arrayHead == -1 ) {
                return -1;
            }

            int field = _Doc.AllocateField();
            _Doc.Fields[ field ].Name = _FieldToken;
            _Doc.Fields[ field ].ValuesHead = arrayHead;
            _Doc.Fields[ field ].ValuesTail = arrayTail;
            return field;
        }
        if ( *token.Begin == '{' ) {
            // value is object
            _Tokenizer.NextToken( _Doc.Buffer );

            int value = ParseObject( _Tokenizer, _Doc );
            if ( value == -1 ) {
                return -1;
            }

            int field = _Doc.AllocateField();
            _Doc.Fields[ field ].Name = _FieldToken;
            _Doc.Fields[ field ].ValuesHead = _Doc.Fields[ field ].ValuesTail = value;
            return field;
        }
        GLogger.Printf( "unexpected bracket %c\n", *token.Begin );
        return -1;
    }

    if ( token.Type == TT_String ) {
        _Tokenizer.NextToken( _Doc.Buffer );

        // value is string
        int value = _Doc.AllocateValue();
        _Doc.Values[ value ].Type = FDocumentValue::T_String;
        _Doc.Values[ value ].Token = token;

        int field = _Doc.AllocateField();
        _Doc.Fields[ field ].Name = _FieldToken;
        _Doc.Fields[ field ].ValuesHead = _Doc.Fields[ field ].ValuesTail = value;

        return field;
    }

    GLogger.Printf( "expected field value, found %s\n", token.NamedType() );
    return -1;
}

static void PrintField( const FDocument & _Doc, int i );

static void PrintValue( const FDocument & _Doc, int i ) {
    const FDocumentValue * value = &_Doc.Values[ i ];

    GLogger.Printf( "Type: %s\n", value->Type == FDocumentValue::T_String ? "STRING" : "OBJECT" );
    if ( value->Type == FDocumentValue::T_String ) {
        GLogger.Printf( "%s\n", value->Token.ToString().ToConstChar() );
        return;
    }

    for ( i = value->FieldsHead ; i != -1 ; i = _Doc.Fields[ i ].Next ) {
        PrintField( _Doc, i );
    }
}

static void PrintField( const FDocument & _Doc, int i ) {
    const FDocumentField * field = &_Doc.Fields[ i ];

    GLogger.Printf( "Field: %s\n", field->Name.ToString().ToConstChar() );

    for ( i = field->ValuesHead ; i != -1 ; i = _Doc.Values[ i ].Next ) {
        PrintValue( _Doc, i );
    }
}

void PrintDocument( FDocument const & _Doc ) {
    GLogger.Print( "-------------- Document ----------------\n" );

    for ( int i = _Doc.FieldsHead ; i != -1 ; i = _Doc.Fields[ i ].Next ) {
        PrintField( _Doc, i );
    }

    GLogger.Print( "----------------------------------------\n" );
}

void FDocument::FromString( const char * _Script, bool _InSitu ) {
    FTokenizer tokenizer;

    Clear();

    Buffer.Initialize( _Script, _InSitu );

    tokenizer.NextToken( Buffer );

    while ( 1 ) {
        FToken token = tokenizer.GetToken();
        if ( token.Type == TT_EOF || token.Type == TT_Unknown ) {
            break;
        }

        if ( !Expect( TT_Field, token ) ) {
            //error
            Clear();
            break;
        }

        tokenizer.NextToken( Buffer );

        int field = ParseField( tokenizer, *this, token );
        if ( field == -1 ) {
            // error
            Clear();
            break;
        }
        Fields[ field ].Prev = FieldsTail;
        if ( Fields[ field ].Prev != -1 ) {
            Fields[ Fields[ field ].Prev ].Next = field;
        } else {
            FieldsHead = field;
        }
        FieldsTail = field;
    }

    //PrintDocument( *this );
}

FDocumentField * FDocument::FindField( int _FieldsHead, const char * _Name ) const {
    for ( int i = _FieldsHead ; i != -1 ; i = Fields[ i ].Next ) {
        FDocumentField * field = &Fields[ i ];
        if ( field->Name.CompareToString( _Name ) ) {
            return field;
        }
    }
    return nullptr;
}

int FDocument::CreateField( const char * _FieldName ) {
    int field = AllocateField();
    Fields[ field ].Name.FromString( _FieldName );
    return field;
}

int FDocument::CreateStringValue( const char * _Value ) {
    int value = AllocateValue();
    Values[ value ].Type = FDocumentValue::T_String;
    Values[ value ].Token.FromString( _Value );
    return value;
}

int FDocument::CreateObjectValue() {
    int value = AllocateValue();
    Values[ value ].Type = FDocumentValue::T_Object;
    return value;
}

void FDocument::AddValueToField( int _FieldOrArray, int _Value ) {
    Values[ _Value ].Prev = Fields[ _FieldOrArray ].ValuesTail;
    if ( Values[ _Value ].Prev != -1 ) {
        Values[ Values[ _Value ].Prev ].Next = _Value;
    } else {
        Fields[ _FieldOrArray ].ValuesHead = _Value;
    }
    Fields[ _FieldOrArray ].ValuesTail = _Value;
}

int FDocument::CreateStringField( const char * _FieldName, const char * _FieldValue ) {
    int field = CreateField( _FieldName );
    int value = CreateStringValue( _FieldValue );
    AddValueToField( field, value );
    return field;
}

void FDocument::AddFieldToObject( int _Object, int _Field ) {
    AN_Assert( Values[ _Object ].Type == FDocumentValue::T_Object );
    Fields[ _Field ].Prev = Values[ _Object ].FieldsTail;
    if ( Fields[ _Field ].Prev != -1 ) {
        Fields[ Fields[ _Field ].Prev ].Next = _Field;
    } else {
        Values[ _Object ].FieldsHead = _Field;
    }
    Values[ _Object ].FieldsTail = _Field;
}

int FDocument::AddStringField( int _Object, const char * _FieldName, const char * _FieldValue ) {
    int field = CreateStringField( _FieldName, _FieldValue );
    AddFieldToObject(  _Object, field );
    return field;
}

int FDocument::AddArray( int _Object, const char * _ArrayName ) {
    int array = CreateField( _ArrayName );
    AddFieldToObject( _Object, array );
    return array;
}

void FDocument::AddField( int _Field ) {
    Fields[ _Field ].Prev = FieldsTail;
    if ( Fields[ _Field ].Prev != -1 ) {
        Fields[ Fields[ _Field ].Prev ].Next = _Field;
    } else {
        FieldsHead = _Field;
    }
    FieldsTail = _Field;
}


static int DocumentStack = 0;
static FString ObjectToString( const FDocument & _Doc, int _FieldsHead, bool & singleField );

static FString InserSpaces() {
    FString s;
    for ( int i = 0 ; i < DocumentStack ; i++ ) {
        s += " ";
    }
    return s;
}

static FString ValueToString( const FDocument & _Doc, int i ) {
    const FDocumentValue * value = &_Doc.Values[ i ];

    FString s;

    if ( value->Type == FDocumentValue::T_String ) {
        s += "\"" + value->Token.ToString() + "\"";
        return s;
    }

    s += "{";

    DocumentStack++;
    bool singleField;
    s += ObjectToString( _Doc, value->FieldsHead, singleField );
    DocumentStack--;

    if ( !singleField ) {
        s += InserSpaces();
    }
    s += "}";

    return s;
}

static FString FieldToString( const FDocument & _Doc, int i ) {
    const FDocumentField * field = &_Doc.Fields[ i ];

    FString s;

    //s += InserSpaces();
    s += field->Name.ToString();
    s += " ";

    bool singleValue = true;

    for ( i = field->ValuesHead ; i != -1 ;  ) {

        int next = _Doc.Values[ i ].Next;

        if ( singleValue && next != -1 ) {
            singleValue = false;
            s += /*"\n" + InserSpaces() + */"[\n";
            DocumentStack++;
        }

        if ( !singleValue ) {
            s += InserSpaces();
        }
        s += ValueToString( _Doc, i );
        if ( !singleValue ) {
            s += "\n";
        }

        i = next;
    }

    if ( !singleValue ) {
        DocumentStack--;
        s += InserSpaces() + "]";
    }

    return s;
}

static FString ObjectToString( const FDocument & _Doc, int _FieldsHead, bool & singleField ) {
    FString s;
    singleField = true;
    for ( int i = _FieldsHead ; i != -1 ; ) {
        int next = _Doc.Fields[ i ].Next;

        if ( singleField && next != -1 ) {
            singleField = false;
            s += "\n";
        }

        if ( !singleField ) {
            s += InserSpaces();
        }
        s += FieldToString( _Doc, i );

        if ( !singleField ) {
            s += "\n";
        }

        i = next;
    }
    return s;
}

static FString ObjectToStringCompact( const FDocument & _Doc, int _FieldsHead );

static FString ValueToStringCompact( const FDocument & _Doc, int i ) {
    const FDocumentValue * value = &_Doc.Values[ i ];
    if ( value->Type == FDocumentValue::T_String ) {
        return FString( "\"" ) + value->Token.ToString() + "\"";
    }
    return FString( "{" ) + ObjectToStringCompact( _Doc, value->FieldsHead ) + "}";
}

static FString FieldToStringCompact( const FDocument & _Doc, int i ) {
    const FDocumentField * field = &_Doc.Fields[ i ];
    FString s = field->Name.ToString();
    bool singleValue = true;
    for ( i = field->ValuesHead ; i != -1 ;  ) {
        int next = _Doc.Values[ i ].Next;
        if ( singleValue && next != -1 ) {
            singleValue = false;
            s += "[";
        }
        s += ValueToStringCompact( _Doc, i );
        i = next;
    }
    if ( !singleValue ) {
        s += "]";
    }
    return s;
}

static FString ObjectToStringCompact( const FDocument & _Doc, int _FieldsHead ) {
    FString s;
    for ( int i = _FieldsHead ; i != -1 ; i = _Doc.Fields[ i ].Next ) {
        s += FieldToStringCompact( _Doc, i );
    }
    return s;
}

FString FDocument::ToString() const {
    if ( bCompactStringConversion ) {
        return ObjectToStringCompact( *this, FieldsHead );
    }

    DocumentStack = 0;
    bool singleField;
    return ::ObjectToString( *this, FieldsHead, singleField );
}

FString FDocument::ObjectToString( int _Object ) const {
    if ( _Object < 0 ) {
        return "";
    }

    if ( bCompactStringConversion ) {
        return ::ObjectToStringCompact( *this, Values[ _Object ].FieldsHead );
    }

    DocumentStack = 0;
    bool singleField;
    return ::ObjectToString( *this, Values[ _Object ].FieldsHead, singleField );
}
