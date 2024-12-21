#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <xlocale/_stdlib.h>

using namespace llvm;

enum Token {
    tok_eof = -1,

    tok_id = -2,
    tok_number = -3,

    tok_arrow = -4,
    tok_scope_res_op = -5,
};

static std::string id_name;
static double num_val;

static int get_tok()
{
    static int last_char = ' ';

    while (isspace(last_char)) {
        last_char = getchar();
    }

    // number
    if (isdigit(last_char) || last_char == '.') {
        std::string num_str;
        do {
            num_str += last_char;
            num_str = getchar();
        } while (isdigit(last_char) || last_char == '.');

        try {
            num_val = strtod(num_str.c_str(), nullptr);
            return tok_number;
        }
        catch (...) {
            std::cerr << "Error: unable to parse number " << num_str;
            throw;
        }
    }

    // identifier
    if (isalpha(last_char)) {
        id_name = last_char;
        while (isalnum(last_char = getchar())) {
            id_name += last_char;
        }
        return tok_id;
    }

    if (last_char == '/') {
        last_char = getchar();
        // comment
        if (last_char == '/') {
            do {
                last_char = getchar();
            } while (last_char != EOF && last_char != '\n' && last_char != '\r');

            if (last_char != EOF) {
                return get_tok();
            }
        }
        else {
            std::cin.putback(last_char);
            last_char = '/';
        }
    }

    if (last_char == '-') {
        last_char = getchar();
        // arrow
        if (last_char == '>') {
            last_char = getchar();
            return tok_arrow;
        }
        else {
            std::cin.putback(last_char);
            last_char = '-';
        }
    }

    if (last_char == ':') {
        last_char = getchar();
        // arrow
        if (last_char == ':') {
            last_char = getchar();
            return tok_scope_res_op;
        }
        else {
            std::cin.putback(last_char);
            last_char = ':';
        }
    }

    if (last_char == EOF) {
        return tok_eof;
    }

    int curr = last_char;
    last_char = getchar();
    return curr;

}

int main () {
    int tmp;
    while ((tmp = get_tok()) != tok_eof) {
        switch (tmp) {
            case -1: 
                std::cerr << "EOF";
                break;
            case -2: 
                std::cerr << "Identifier: " << id_name;
                break;
            case -3: 
                std::cerr << "Number: " << num_val;
                break;
            case -4: 
                std::cerr << "Arrow";
                break;
            case -5: 
                std::cerr << "Scope res op";
                break;
            default: 
                std::cerr << (char) tmp;
        }
        std::cerr << "\n";
    }
}   
