#ifndef TOKEN_TYPES_H
#define TOKEN_TYPES_H

enum class TokenType {
    None,
    Keyword,
    String,
    Comment,
    Number,
    Operator,
    Identifier,
    Function,
    Type,
    Preprocessor
};

struct Token {
    int position = 0;
    int length = 0;
    TokenType type = TokenType::None;

    Token() = default;
    Token(int pos, int len, TokenType t) : position(pos), length(len), type(t) {}
};

#endif // TOKEN_TYPES_H