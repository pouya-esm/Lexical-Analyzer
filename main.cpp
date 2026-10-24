// A small hand-written lexical analyzer (DFA-based), written for a
// compilers course assignment.
//
// The core idea: rather than a single DFA, the scanner tries a sequence
// of independent "start states" for each new token (identifier -> number
// -> operator -> punctuation -> invalid). If one sub-DFA fails to match
// the current character, the file position is rewound with fseek() and
// the next start state in the chain is tried. fail() defines that chain.
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <cctype>

using namespace std;

// Returns the next "start state" to try when the current attempt fails.
int fail(int start) {
    int nextStart = 0;
    switch (start) {
        case 1: nextStart = 3; break; // identifier failed  -> try number
        case 3: nextStart = 7; break; // number failed      -> try operator
        case 7: nextStart = 8; break; // operator failed    -> try sign/punctuation
        case 8: nextStart = 9; break; // sign failed        -> invalid token
    }
    return nextStart;
}

// Safe wrappers around <cctype>: isalpha()/isalnum() have undefined
// behavior when called with a value that is neither EOF nor representable
// as unsigned char. getc()/ungetc() work with int, so we keep characters
// as int throughout and only normalize right before these calls.
bool isAsciiAlpha(int c) {
    return c != EOF && isalpha(static_cast<unsigned char>(c)) != 0;
}
bool isAsciiAlnum(int c) {
    return c != EOF && isalnum(static_cast<unsigned char>(c)) != 0;
}

int main(int argc, char *argv[]) {
    const char *filename = (argc > 1) ? argv[1] : "source.txt";

    long loc;
    int start = 1, state = 10;
    int ch = 0;           // int, not char: must represent EOF (-1) distinctly
                           // from every valid byte value 0-255.
    string buffer = "";

    FILE *fp = fopen(filename, "r");
    if (fp == nullptr) {
        cerr << "Error: could not open \"" << filename << "\" "
             << "(check that the file exists and is readable)." << endl;
        return 1;
    }

    while (true) {
        switch (state) {

            case 1: // try: identifier / keyword
                buffer = "";
                loc = ftell(fp);
                ch = getc(fp);
                if (isAsciiAlpha(ch)) {
                    buffer += static_cast<char>(ch);
                    state = 2;
                } else {
                    start = fail(start);
                    state = start;
                    fseek(fp, loc, SEEK_SET);
                }
                break;

            case 2: // continue reading identifier/keyword characters
                ch = getc(fp);
                if (isAsciiAlnum(ch)) {
                    buffer += static_cast<char>(ch);
                    state = 2;
                } else {
                    if (ch != EOF) ungetc(ch, fp);
                    if (buffer == "int" || buffer == "double" || buffer == "float" ||
                        buffer == "String" || buffer == "char" || buffer == "bool" ||
                        buffer == "if" || buffer == "else" || buffer == "switch" ||
                        buffer == "case" || buffer == "break" || buffer == "for" ||
                        buffer == "while") {
                        cout << "KEYWORD: " << buffer << endl;
                    } else {
                        cout << "IDENTIFIER: " << buffer << endl;
                    }
                    state = 10;
                }
                break;

            case 3: // try: number (integer / real)
                loc = ftell(fp);
                buffer = "";
                ch = getc(fp);
                if (ch >= '1' && ch <= '9') {
                    buffer += static_cast<char>(ch);
                    state = 4;
                } else if (ch == '0') {
                    buffer += static_cast<char>(ch);
                    state = 6;
                } else {
                    start = fail(start);
                    state = start;
                    fseek(fp, loc, SEEK_SET);
                }
                break;

            case 4: // integer part (first digit was 1-9)
                ch = getc(fp);
                if (ch >= '0' && ch <= '9') {
                    buffer += static_cast<char>(ch);
                    state = 4;
                } else if (ch == '.') {
                    buffer += static_cast<char>(ch);
                    state = 5;
                } else if (ch == 'e' || ch == 'E') {
                    buffer += static_cast<char>(ch);
                    state = 12;
                } else if (isAsciiAlpha(ch)) {
                    buffer += static_cast<char>(ch);
                    state = 9;
                } else {
                    if (ch != EOF) ungetc(ch, fp);
                    cout << "INTEGER: " << buffer << endl;
                    state = 10;
                }
                break;

            case 5: // fractional part of a real number
                ch = getc(fp);
                if (ch >= '0' && ch <= '9') {
                    buffer += static_cast<char>(ch);
                    state = 5;
                } else if (ch == 'e' || ch == 'E') {
                    buffer += static_cast<char>(ch);
                    state = 12;
                } else if (isAsciiAlpha(ch)) {
                    buffer += static_cast<char>(ch);
                    state = 9;
                } else {
                    if (ch != EOF) ungetc(ch, fp);
                    cout << "REAL: " << buffer << endl;
                    state = 10;
                }
                break;

            case 6: // integer part started with '0'
                ch = getc(fp);
                if (ch == '.') {
                    buffer += static_cast<char>(ch);
                    state = 5;
                } else if (ch == 'e' || ch == 'E') {
                    buffer += static_cast<char>(ch);
                    state = 12;
                } else if (isAsciiAlpha(ch)) {
                    buffer += static_cast<char>(ch);
                    state = 9;
                } else {
                    if (ch != EOF) ungetc(ch, fp);
                    cout << "INTEGER: " << buffer << endl;
                    state = 10;
                }
                break;

            case 12: { // exponent part of a real number: [eE][+-]?digit+
                ch = getc(fp);
                if (ch == '+' || ch == '-') {
                    buffer += static_cast<char>(ch);
                    ch = getc(fp);
                }
                bool sawDigit = false;
                while (ch >= '0' && ch <= '9') {
                    buffer += static_cast<char>(ch);
                    sawDigit = true;
                    ch = getc(fp);
                }
                if (!sawDigit || isAsciiAlpha(ch)) {
                    // malformed exponent (e.g. "1e", "1ex") -> report as invalid
                    if (ch != EOF) buffer += static_cast<char>(ch);
                    state = 9;
                } else {
                    if (ch != EOF) ungetc(ch, fp);
                    cout << "REAL: " << buffer << endl;
                    state = 10;
                }
                break;
            }

            case 7: // try: operators (also absorbs // and /* */ comments)
                loc = ftell(fp);
                ch = getc(fp);
                if (ch == '/') {
                    int nextCh = getc(fp);
                    if (nextCh == '/') {
                        while ((nextCh = getc(fp)) != '\n' && nextCh != EOF) { /* skip */ }
                        state = 10;
                    } else if (nextCh == '*') {
                        int prev = 0, cur;
                        bool closed = false;
                        while ((cur = getc(fp)) != EOF) {
                            if (prev == '*' && cur == '/') { closed = true; break; }
                            prev = cur;
                        }
                        if (!closed) {
                            cerr << "Warning: unterminated block comment" << endl;
                        }
                        state = 10;
                    } else {
                        if (nextCh != EOF) ungetc(nextCh, fp);
                        cout << "OPERATOR: /" << endl;
                        state = 10;
                    }
                }
                else if (ch == '+' || ch == '-' || ch == '%' || ch == '*') {
                    int nextCh = getc(fp);
                    if (nextCh == ch && (ch == '+' || ch == '-')) {
                        cout << "OPERATOR: " << static_cast<char>(ch) << static_cast<char>(ch) << endl;
                    } else {
                        if (nextCh != EOF) ungetc(nextCh, fp);
                        cout << "OPERATOR: " << static_cast<char>(ch) << endl;
                    }
                    state = 10;
                }
                else if (ch == '|') {
                    int nextCh = getc(fp);
                    if (nextCh == '|') {
                        cout << "OPERATOR: ||" << endl;
                    } else {
                        if (nextCh != EOF) ungetc(nextCh, fp);
                        cout << "BITWISE OR: |" << endl;
                    }
                    state = 10;
                }
                else if (ch == '&') {
                    int nextCh = getc(fp);
                    if (nextCh == '&') {
                        cout << "OPERATOR: &&" << endl;
                    } else {
                        if (nextCh != EOF) ungetc(nextCh, fp);
                        cout << "BITWISE AND: &" << endl;
                    }
                    state = 10;
                }
                else if (ch == '=') {
                    int nextCh = getc(fp);
                    if (nextCh == '=') {
                        cout << "OPERATOR: ==" << endl;
                    } else {
                        if (nextCh != EOF) ungetc(nextCh, fp);
                        cout << "ASSIGN: =" << endl;
                    }
                    state = 10;
                }
                else if (ch == '!') {
                    int nextCh = getc(fp);
                    if (nextCh == '=') {
                        cout << "OPERATOR: !=" << endl;
                    } else {
                        if (nextCh != EOF) ungetc(nextCh, fp);
                        cout << "OPERATOR: !" << endl;
                    }
                    state = 10;
                }
                else if (ch == '<') {
                    int nextCh = getc(fp);
                    if (nextCh == '=') {
                        cout << "OPERATOR: <=" << endl;
                    } else {
                        if (nextCh != EOF) ungetc(nextCh, fp);
                        cout << "OPERATOR: <" << endl;
                    }
                    state = 10;
                }
                else if (ch == '>') {
                    int nextCh = getc(fp);
                    if (nextCh == '=') {
                        cout << "OPERATOR: >=" << endl;
                    } else {
                        if (nextCh != EOF) ungetc(nextCh, fp);
                        cout << "OPERATOR: >" << endl;
                    }
                    state = 10;
                }
                else {
                    start = fail(start);
                    state = start;
                    fseek(fp, loc, SEEK_SET);
                }
                break;

            case 8: // try: punctuation / signs
                loc = ftell(fp);
                ch = getc(fp);
                if (ch == ':' || ch == ';' || ch == '{' || ch == '}' ||
                    ch == '[' || ch == ']' || ch == '(' || ch == ')' || ch == ',') {
                    cout << "SIGN: " << static_cast<char>(ch) << endl;
                    state = 10;
                } else {
                    start = fail(start);
                    state = start;
                    fseek(fp, loc, SEEK_SET);
                }
                break;

            case 9: // invalid token: consume the run of "word-like" characters
                if (buffer.empty()) {
                    ch = getc(fp);
                    if (ch != EOF) buffer += static_cast<char>(ch);
                }
                while (true) {
                    ch = getc(fp);
                    if (ch == ' ' || ch == '\n' || ch == '\t' || ch == EOF) {
                        break;
                    }
                    if (isAsciiAlnum(ch) || ch == '_') {
                        buffer += static_cast<char>(ch);
                    } else {
                        ungetc(ch, fp);
                        break;
                    }
                }
                cout << "INVALID TOKEN: " << buffer << endl;
                buffer = "";
                state = 10;
                break;

            case 11: { // string literal: "...."
                buffer = "";
                bool closed = false;
                while (true) {
                    ch = getc(fp);
                    if (ch == EOF || ch == '\n') {
                        break; // unterminated string literal
                    }
                    if (ch == '"') {
                        closed = true;
                        break;
                    }
                    if (ch == '\\') {
                        int esc = getc(fp);
                        buffer += static_cast<char>(ch);
                        if (esc != EOF) buffer += static_cast<char>(esc);
                        continue;
                    }
                    buffer += static_cast<char>(ch);
                }
                if (closed) {
                    cout << "STRING: \"" << buffer << "\"" << endl;
                } else {
                    cerr << "Warning: unterminated string literal" << endl;
                    cout << "INVALID TOKEN: \"" << buffer << endl;
                }
                state = 10;
                break;
            }

            case 10: // dispatcher: skip whitespace, detect strings, handle EOF
                ch = getc(fp);
                if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r') {
                    state = 10;
                } else if (ch == EOF) {
                    fclose(fp);
                    return 0;
                } else if (ch == '"') {
                    state = 11;
                } else {
                    ungetc(ch, fp);
                    start = 1;
                    state = 1;
                }
                break;
        }
    }
}
