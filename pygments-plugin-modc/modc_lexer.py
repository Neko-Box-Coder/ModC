from pygments.lexer import RegexLexer, bygroups, words
from pygments.token import *

class ModCLexer(RegexLexer):
    """
    For ModC source.
    """
    name = 'ModC'
    # url = 'https://modc.dev/'
    filenames = ['*.modc', '*.modh']
    aliases = ['modc', 'modh']
    # mimetypes = ['text/x-modc']
    # version_added = '1.0'
    
    ident = r'(?!\d)[\w]+'
    
    # The trailing ?, rather than *, avoids a geometric performance drop here.
    #: only one /* */ style comment
    _ws1 = r'\s*(?:/[*].*?[*]/\s*)?'
    
    tokens = {
        'root': [
            (r'^(\s*)(#if\s+0)',
             bygroups(Whitespace, Comment.Preproc), 'if0'),
            
            (r'\n', Whitespace),
            (r'\s+', Whitespace),
            (r'(\\)(\n)', bygroups(Text, Whitespace)),  # line continuations
            (r'//(.*?)$', Comment.Single),
            (r'/(\\\n)?[*](.|\n)*?[*](\\\n)?/', Comment.Multiline),
            (r'(namespace)(\s+)((?:' + ident + r'(?:\s*\.)?)*)',
                bygroups(Keyword.Namespace, Whitespace, Name.Namespace)),
            
            (r'(struct|typed_union|union|enum)\b', Keyword.Declaration),
            
            
            (words((
                'break', 'default', 'case', 'do', 'while',
                'else', 'goto', 'switch', 'if', 'extern', 
                'continue', 'for', 'return', 'typedef',
                
                'ref', 'in', 'out', 'const', 'implicit', 'direct', 
                'prepile', 'defer', 'deferred', 'unique', 'lease',
                'bind', 'use', 'return_value'), suffix=r'\b'), Keyword),
            (r'(true|false|null)\b', Keyword.Builtin),
            
            
            
            (words((
                'uint', 'uint8', 'uint16', 'uint32', 'uint64', 
                'int', 'int8', 'int16', 'int32', 'int64', 'void',
                'char', 'float', 'double', 'string', 'bool', 'any', 
                'array'), suffix=r'\b'),
             Keyword.Type),
            # float_lit
            (r'\d+(\.\d+[eE][+\-]?\d+|'
             r'\.\d*|[eE][+\-]?\d+)', Number.Float),
            (r'\.\d+([eE][+\-]?\d+)?', Number.Float),
            # int_lit
            # -- octal_lit
            (r'0[0-7]+', Number.Oct),
            # -- hex_lit
            (r'0[xX][0-9a-fA-F]+', Number.Hex),
            # -- decimal_lit
            (r'(0|[1-9][0-9]*)', Number.Integer),
            # char_lit
            (r"""'(\\['"\\abfnrtv]|\\x[0-9a-fA-F]{2}|\\[0-7]{1,3}"""
             r"""|\\u[0-9a-fA-F]{4}|\\U[0-9a-fA-F]{8}|[^\\])'""",
             String.Char),
            # StringLiteral
            # -- raw_string_lit
            (r'`[^`]*`', String),
            # -- interpreted_string_lit
            (r'"(\\\\|\\[^\\]|[^"\\])*"', String),
            # Tokens
            (r'(<<=|>>=|<<|>>|<=|>=|&\^=|&\^|\+=|-=|\*=|/=|%=|&=|\|=|&&|\|\|'
             r'|<-|\+\+|--|==|!=|=|\.\.\.|[+\-*/%&]'
             r'|~|\|)', Operator),
            # Function Statement
            (r'((?:' + ident + r'\s*\.)*)?(' + ident + r')(\s*)(\()', 
                bygroups(Name, Name.Function, Whitespace, Punctuation)),
            
            (r'[|^<>=!()\[\]{}.,;:]', Punctuation),
            # identifier
            (r'[^\W\d]\w*', Name.Other),
        ],
        'if0': [
            (r'(^\s*)(#if.*?(?<!\\))(?!(?://.*))', bygroups(Whitespace, Comment.Preproc), '#push'),
            (r'(^\s*)(#el(?:se|if).*?(?<!\\))(?!(?://.*))', bygroups(Whitespace, Comment.Preproc), '#pop'),
            (r'(^\s*)(#endif.*?(?<!\\))(?!(?://.*))', bygroups(Whitespace, Comment.Preproc), '#pop'),
            (r'.*?\n', Comment),
        ],
    }
