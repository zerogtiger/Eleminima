#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <map>
#include <string>

using namespace llvm;

enum Token {
    tok_eof = -1,

    tok_id = -2,
    tok_number = -3,
    tok_string = -4,

    tok_arrow = -5,
    tok_scope_res_op = -6,
};

static std::string id_name, str_val;
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
        while (isalnum(last_char = getchar()) || last_char == '_') {
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

    if (last_char == '"') {
        str_val = "";
        last_char = getchar();
        while (last_char != '"') {
            if (last_char == '\\') {
                str_val += last_char;
                last_char = getchar();
            }
            str_val += last_char;
            last_char = getchar();
        }
        return tok_string;
    }

    if (last_char == EOF) {
        return tok_eof;
    }

    int curr = last_char;
    last_char = getchar();
    return curr;
}

int main()
{
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
            std::cerr << "String: " << str_val;
            break;
        case -5:
            std::cerr << "Arrow";
            break;
        case -6:
            std::cerr << "Scope res op";
            break;
        default:
            std::cerr << (char) tmp;
        }
        std::cerr << "\n";
    }
}

// namespace {
// class expr_ast {
//
//   public:
//     virtual Value* code_gen() = 0;
//
//     virtual ~expr_ast() = default;
// };
//
// class number_expr_ast : public expr_ast {
//
//     double val;
//
//   public:
//     number_expr_ast(double val) : val{val} {}
//
//     Value* code_gen() override;
// };
//
// class string_expr_ast : public expr_ast {
//
//     std::string str;
//
//   public:
//     string_expr_ast(std::string str) : str{str} {}
//
//     Value* code_gen() override;
// };
//
// class call_expr_ast : public expr_ast {
//     std::string callee;
//     std::vector<std::unique_ptr<expr_ast>> args;
//
//   public:
//     call_expr_ast(const std::string& callee, std::vector<std::unique_ptr<expr_ast>> args)
//         : callee(callee), args(std::move(args))
//     {
//     }
//
//     Value* code_gen() override;
// };
//
// class list_expr_ast : public expr_ast {
//     std::vector<std::unique_ptr<expr_ast>> content;
//
//   public:
//     list_expr_ast(std::vector<std::unique_ptr<expr_ast>> content) : content(std::move(content))
//     {}
//
//     Value* code_gen() override;
// };
//
// class list_expr_ast : public expr_ast {
//     std::vector<std::unique_ptr<expr_ast>> content;
//
//   public:
//     list_expr_ast(std::vector<std::unique_ptr<expr_ast>> content) : content(std::move(content))
//     {}
//
//     Value* code_gen() override;
// };
//
// class node_expr_ast : public expr_ast {
//     std::map<std::string, std::vector<std::unique_ptr<expr_ast>>> fields;
// }:
//
// } // namespace
