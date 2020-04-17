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

#include <Core/Public/Document.h>
#include <Core/Public/Alloc.h>
#include <Core/Public/Logger.h>

ADocument::ADocument() {

}

ADocument::~ADocument() {
    Allocator::Inst().Free( Fields );
    Allocator::Inst().Free( Values );
}

void ADocument::Clear() {
    FieldsHead = -1;
    FieldsTail = -1;
    FieldsCount = 0;
    ValuesCount = 0;
}

int ADocument::AllocateField() {
    if ( FieldsCount == FieldsMemReserved ) {
        FieldsMemReserved = FieldsMemReserved ? FieldsMemReserved<<1 : 1024;
        Fields = ( SDocumentField * )Allocator::Inst().Realloc( Fields, sizeof( SDocumentField ) * FieldsMemReserved, true );
    }
    SDocumentField * field = &Fields[ FieldsCount ];
    field->ValuesHead = -1;
    field->ValuesTail = -1;
    field->Next = -1;
    field->Prev = -1;
    FieldsCount++;
    return FieldsCount - 1;
}

int ADocument::AllocateValue() {
    if ( ValuesCount == ValuesMemReserved ) {
        ValuesMemReserved = ValuesMemReserved ? ValuesMemReserved<<1 : 1024;
        Values = ( SDocumentValue * )Allocator::Inst().Realloc( Values, sizeof( SDocumentValue ) * ValuesMemReserved, true );
    }
    SDocumentValue * value = &Values[ ValuesCount ];
    value->FieldsHead = -1;
    value->FieldsTail = -1;
    value->Next = -1;
    value->Prev = -1;
    ValuesCount++;
    return ValuesCount - 1;
}

ATokenBuffer::ATokenBuffer() {
    Cur = Start = (char *)"";
    LineNumber = 1;
    bInSitu = true;
}

ATokenBuffer::~ATokenBuffer() {
    if ( !bInSitu ) {
        Allocator::Inst().Free( Start );
    }
}

void ATokenBuffer::Initialize( const char * _String, bool _InSitu ) {
    Deinitialize();

    bInSitu = _InSitu;
    if ( bInSitu ) {
        Start = const_cast< char * >( _String );
    } else {
        int n = Core::Strlen( _String ) + 1;
        Start = ( char * )Allocator::Inst().Alloc( n );
        Core::Memcpy( Start, _String, n );
    }
    Cur = Start;
    LineNumber = 1;
}

void ATokenBuffer::Deinitialize() {
    if ( !bInSitu ) {
        Allocator::Inst().Free( Start );
        Cur = Start = (char *)"";
        LineNumber = 1;
        bInSitu = true;
    }
}

ADocumentProxyBuffer::ADocumentProxyBuffer()
    : StringList( nullptr ) {

}

ADocumentProxyBuffer::~ADocumentProxyBuffer() {
    AStringList * next;
    for ( AStringList * node = StringList ; node ; node = next ) {
        next = node->Next;
        node->~AStringList();
        Allocator::Inst().Free( node );
    }
}

AString & ADocumentProxyBuffer::NewString() {
    void * pMemory = Allocator::Inst().Alloc( sizeof( AStringList ) );
    AStringList * node = new ( pMemory ) AStringList;
    node->Next = StringList;
    StringList = node;
    return node->Str;
}

AString & ADocumentProxyBuffer::NewString( const char * _String ) {
    void * pMemory = Allocator::Inst().Alloc( sizeof( AStringList ) );
    AStringList * node = new ( pMemory ) AStringList( _String );
    node->Next = StringList;
    StringList = node;
    return node->Str;
}

AString & ADocumentProxyBuffer::NewString( const AString & _String ) {
    void * pMemory = Allocator::Inst().Alloc( sizeof( AStringList ) );
    AStringList * node = new ( pMemory ) AStringList( _String );
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

bool SToken::CompareToString( const char * _Str ) const {
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

void SToken::FromString( const char * _Str ) {
    Begin = _Str;
    End = Begin + Core::Strlen( _Str );
}

AString SToken::ToString() const {
    AString str;
    str.Resize( End - Begin );
    Core::Memcpy( str.ToPtr(), Begin, str.Length() );
    return str;
}

const char * SToken::NamedType() const {
    return TokenType[ Type ];
}

struct ATokenizer {
    SToken CurToken;

    const SToken & GetToken() const;
    void NextToken( ATokenBuffer & buffer );
};

static void SkipWhitespaces( ATokenBuffer & _Buffer ) {
    const char *& s = _Buffer.Cur;
start:
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
            goto start;
        }
        if ( *(s+1) == '*' ) {
            s += 2;
            while ( *s ) {
                if ( *s == '\n' ) {
                    _Buffer.LineNumber++;
                } else if ( *s == '*' && *(s+1) == '/' ) {
                    s += 2;
                    goto start;
                }
                s++;
            }
            GLogger( "Warning: unclosed comment /* */\n" );
            return;
        }
    }
}

const SToken & ATokenizer::GetToken() const {
    return CurToken;
}

void ATokenizer::NextToken( ATokenBuffer & _Buffer ) {
    // Skip white spaces, tabs and comments
    SkipWhitespaces( _Buffer );

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
                CurToken.Type = TOKEN_TYPE_UNKNOWN;
                return;
            }
            if ( *s == '\n' ) {
                // unexpected eol
                CurToken.Begin = CurToken.End = "";
                CurToken.Type = TOKEN_TYPE_UNKNOWN;
                return;
            }
            s++;
        }
        CurToken.End = s++;
        CurToken.Type = TOKEN_TYPE_STRING;
        return;
    }

    // Check brackets
    if ( *s == '{' || *s == '}' || *s == '[' || *s == ']' ) {
        CurToken.Begin = s;
        CurToken.End = ++s;
        CurToken.Type = TOKEN_TYPE_BRACKET;
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
            CurToken.Type = TOKEN_TYPE_UNKNOWN;
        } else {
            CurToken.Type = TOKEN_TYPE_EOF;
        }
        return;
    } else {
        CurToken.Type = TOKEN_TYPE_FIELD;
    }
}

static bool Expect( int _Type, const SToken & _Token ) {
    if ( _Token.Type != _Type ) {
        GLogger.Printf( "unexpected %s found, expected %s\n", _Token.NamedType(), TokenType[_Type] );
        return false;
    }
    return true;
}

static int ParseObject( ATokenizer & _Tokenizer, ADocument & _Doc );
static int ParseField( ATokenizer & _Tokenizer, ADocument & _Doc, SToken & _FieldToken );

static void ParseArray( ATokenizer & _Tokenizer, ADocument & _Doc, int & _ArrayHead, int & _ArrayTail ) {
    _ArrayHead = -1;
    _ArrayTail = -1;

    while ( 1 ) {
        SToken token = _Tokenizer.GetToken();

        if ( token.Type == TOKEN_TYPE_BRACKET ) {
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

        if ( token.Type == TOKEN_TYPE_STRING ) {
            // array element is string

            int value = _Doc.AllocateValue();
            _Doc.Values[ value ].Type = SDocumentValue::TYPE_STRING;
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

static int ParseObject( ATokenizer & _Tokenizer, ADocument & _Doc ) {

    int value = _Doc.AllocateValue();
    _Doc.Values[ value ].Type = SDocumentValue::TYPE_OBJECT;

    while ( 1 ) {
        SToken token = _Tokenizer.GetToken();

        if ( token.Type == TOKEN_TYPE_BRACKET ) {
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

        if ( !Expect( TOKEN_TYPE_FIELD, token ) ) {
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

static int ParseField( ATokenizer & _Tokenizer, ADocument & _Doc, SToken & _FieldToken ) {
    SToken token = _Tokenizer.GetToken();

    if ( token.Type == TOKEN_TYPE_BRACKET ) {
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

    if ( token.Type == TOKEN_TYPE_STRING ) {
        _Tokenizer.NextToken( _Doc.Buffer );

        // value is string
        int value = _Doc.AllocateValue();
        _Doc.Values[ value ].Type = SDocumentValue::TYPE_STRING;
        _Doc.Values[ value ].Token = token;

        int field = _Doc.AllocateField();
        _Doc.Fields[ field ].Name = _FieldToken;
        _Doc.Fields[ field ].ValuesHead = _Doc.Fields[ field ].ValuesTail = value;

        return field;
    }

    GLogger.Printf( "expected field value, found %s\n", token.NamedType() );
    return -1;
}

static void PrintField( const ADocument & _Doc, int i );

static void PrintValue( const ADocument & _Doc, int i ) {
    const SDocumentValue * value = &_Doc.Values[ i ];

    GLogger.Printf( "Type: %s\n", value->Type == SDocumentValue::TYPE_STRING ? "STRING" : "OBJECT" );
    if ( value->Type == SDocumentValue::TYPE_STRING ) {
        GLogger.Printf( "%s\n", value->Token.ToString().CStr() );
        return;
    }

    for ( i = value->FieldsHead ; i != -1 ; i = _Doc.Fields[ i ].Next ) {
        PrintField( _Doc, i );
    }
}

static void PrintField( const ADocument & _Doc, int i ) {
    const SDocumentField * field = &_Doc.Fields[ i ];

    GLogger.Printf( "Field: %s\n", field->Name.ToString().CStr() );

    for ( i = field->ValuesHead ; i != -1 ; i = _Doc.Values[ i ].Next ) {
        PrintValue( _Doc, i );
    }
}

void PrintDocument( ADocument const & _Doc ) {
    GLogger.Print( "-------------- Document ----------------\n" );

    for ( int i = _Doc.FieldsHead ; i != -1 ; i = _Doc.Fields[ i ].Next ) {
        PrintField( _Doc, i );
    }

    GLogger.Print( "----------------------------------------\n" );
}

void ADocument::FromString( const char * _Script, bool _InSitu ) {
    ATokenizer tokenizer;

    Clear();

    Buffer.Initialize( _Script, _InSitu );

    tokenizer.NextToken( Buffer );

    while ( 1 ) {
        SToken token = tokenizer.GetToken();
        if ( token.Type == TOKEN_TYPE_EOF || token.Type == TOKEN_TYPE_UNKNOWN ) {
            break;
        }

        if ( !Expect( TOKEN_TYPE_FIELD, token ) ) {
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

SDocumentField * ADocument::FindField( int _FieldsHead, const char * _Name ) const {
    for ( int i = _FieldsHead ; i != -1 ; i = Fields[ i ].Next ) {
        SDocumentField * field = &Fields[ i ];
        if ( field->Name.CompareToString( _Name ) ) {
            return field;
        }
    }
    return nullptr;
}

int ADocument::CreateField( const char * _FieldName ) {
    int field = AllocateField();
    Fields[ field ].Name.FromString( _FieldName );
    return field;
}

int ADocument::CreateStringValue( const char * _Value ) {
    int value = AllocateValue();
    Values[ value ].Type = SDocumentValue::TYPE_STRING;
    Values[ value ].Token.FromString( _Value );
    return value;
}

int ADocument::CreateObjectValue() {
    int value = AllocateValue();
    Values[ value ].Type = SDocumentValue::TYPE_OBJECT;
    return value;
}

void ADocument::AddValueToField( int _FieldOrArray, int _Value ) {
    Values[ _Value ].Prev = Fields[ _FieldOrArray ].ValuesTail;
    if ( Values[ _Value ].Prev != -1 ) {
        Values[ Values[ _Value ].Prev ].Next = _Value;
    } else {
        Fields[ _FieldOrArray ].ValuesHead = _Value;
    }
    Fields[ _FieldOrArray ].ValuesTail = _Value;
}

int ADocument::CreateStringField( const char * _FieldName, const char * _FieldValue ) {
    int field = CreateField( _FieldName );
    int value = CreateStringValue( _FieldValue );
    AddValueToField( field, value );
    return field;
}

void ADocument::AddFieldToObject( int _Object, int _Field ) {
    AN_ASSERT( Values[ _Object ].Type == SDocumentValue::TYPE_OBJECT );
    Fields[ _Field ].Prev = Values[ _Object ].FieldsTail;
    if ( Fields[ _Field ].Prev != -1 ) {
        Fields[ Fields[ _Field ].Prev ].Next = _Field;
    } else {
        Values[ _Object ].FieldsHead = _Field;
    }
    Values[ _Object ].FieldsTail = _Field;
}

int ADocument::AddStringField( int _Object, const char * _FieldName, const char * _FieldValue ) {
    int field = CreateStringField( _FieldName, _FieldValue );
    AddFieldToObject(  _Object, field );
    return field;
}

int ADocument::AddArray( int _Object, const char * _ArrayName ) {
    int array = CreateField( _ArrayName );
    AddFieldToObject( _Object, array );
    return array;
}

void ADocument::AddField( int _Field ) {
    Fields[ _Field ].Prev = FieldsTail;
    if ( Fields[ _Field ].Prev != -1 ) {
        Fields[ Fields[ _Field ].Prev ].Next = _Field;
    } else {
        FieldsHead = _Field;
    }
    FieldsTail = _Field;
}


static int DocumentStack = 0;
static AString ObjectToString( const ADocument & _Doc, int _FieldsHead, bool & singleField );

static AString InsertSpaces() {
    AString s;
    for ( int i = 0 ; i < DocumentStack ; i++ ) {
        s += " ";
    }
    return s;
}

static AString ValueToString( const ADocument & _Doc, int i ) {
    const SDocumentValue * value = &_Doc.Values[ i ];

    AString s;

    if ( value->Type == SDocumentValue::TYPE_STRING ) {
        s += "\"" + value->Token.ToString() + "\"";
        return s;
    }

    s += "{";

    DocumentStack++;
    bool singleField;
    s += ObjectToString( _Doc, value->FieldsHead, singleField );
    DocumentStack--;

    if ( !singleField ) {
        s += InsertSpaces();
    }
    s += "}";

    return s;
}

static AString FieldToString( const ADocument & _Doc, int i ) {
    const SDocumentField * field = &_Doc.Fields[ i ];

    AString s;

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
            s += InsertSpaces();
        }
        s += ValueToString( _Doc, i );
        if ( !singleValue ) {
            s += "\n";
        }

        i = next;
    }

    if ( !singleValue ) {
        DocumentStack--;
        s += InsertSpaces() + "]";
    }

    return s;
}

static AString ObjectToString( const ADocument & _Doc, int _FieldsHead, bool & singleField ) {
    AString s;
    singleField = true;
    for ( int i = _FieldsHead ; i != -1 ; ) {
        int next = _Doc.Fields[ i ].Next;

        if ( singleField && next != -1 ) {
            singleField = false;
            s += "\n";
        }

        if ( !singleField ) {
            s += InsertSpaces();
        }
        s += FieldToString( _Doc, i );

        if ( !singleField ) {
            s += "\n";
        }

        i = next;
    }
    return s;
}

static AString ObjectToStringCompact( const ADocument & _Doc, int _FieldsHead );

static AString ValueToStringCompact( const ADocument & _Doc, int i ) {
    const SDocumentValue * value = &_Doc.Values[ i ];
    if ( value->Type == SDocumentValue::TYPE_STRING ) {
        return AString( "\"" ) + value->Token.ToString() + "\"";
    }
    return AString( "{" ) + ObjectToStringCompact( _Doc, value->FieldsHead ) + "}";
}

static AString FieldToStringCompact( const ADocument & _Doc, int i ) {
    const SDocumentField * field = &_Doc.Fields[ i ];
    AString s = field->Name.ToString();
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

static AString ObjectToStringCompact( const ADocument & _Doc, int _FieldsHead ) {
    AString s;
    for ( int i = _FieldsHead ; i != -1 ; i = _Doc.Fields[ i ].Next ) {
        s += FieldToStringCompact( _Doc, i );
    }
    return s;
}

AString ADocument::ToString() const {
    if ( bCompactStringConversion ) {
        return ObjectToStringCompact( *this, FieldsHead );
    }

    DocumentStack = 0;
    bool singleField;
    return ::ObjectToString( *this, FieldsHead, singleField );
}

AString ADocument::ObjectToString( int _Object ) const {
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
