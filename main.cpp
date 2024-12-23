#include "llvm/ADT/ilist_node.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <ios>
#include <iostream>
#include <map>
#include <memory>
#include <string>

using namespace llvm;

enum Category {
    Node,
};

enum NodeName {
    Mix,
    ColorRamp,
};

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
    if (isdigit(last_char) || last_char == '.' || last_char == '+') {
        std::string num_str;
        do {
            num_str += last_char;
            last_char = getchar();
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
        else if (isdigit(last_char) || last_char == '.') { // negative number
            std::string num_str = "-";
            do {
                num_str += last_char;
                last_char = getchar();
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
        else {
            std::cin.putback(last_char);
            last_char = '-';
        }
    }

    if (last_char == ':') {
        last_char = getchar();
        // scope res op
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

namespace {

class expr_ast {

  public:
    virtual Value* code_gen() = 0;

    virtual ~expr_ast() = default;
};

class number_expr_ast : public expr_ast {

    double val;

  public:
    number_expr_ast(double val) : val{val} {}

    Value* code_gen() override;
};

class string_expr_ast : public expr_ast {

    std::string str;

  public:
    string_expr_ast(std::string str) : str{str} {}

    Value* code_gen() override;
};

class call_expr_ast : public expr_ast {
    std::string callee;
    std::vector<std::unique_ptr<expr_ast>> args;

  public:
    call_expr_ast(const std::string& callee, std::vector<std::unique_ptr<expr_ast>> args)
        : callee(callee), args(std::move(args))
    {
    }

    Value* code_gen() override;
};

class list_expr_ast : public expr_ast {
    std::vector<std::unique_ptr<expr_ast>> content;

  public:
    list_expr_ast(std::vector<std::unique_ptr<expr_ast>> content) : content(std::move(content)) {}

    Value* code_gen() override;
};

class node_expr_ast : public expr_ast {
    Category cat;
    NodeName name;
    std::map<std::string, std::unique_ptr<expr_ast>> fields;

  public:
    node_expr_ast(Category cat, NodeName name,
                  std::map<std::string, std::unique_ptr<expr_ast>> fields)
        : cat{cat}, name{name}, fields{std::move(fields)}
    {
    }

    Value* code_gen() override;
};

class var_expr_ast : public expr_ast {
    std::string name;
    std::unique_ptr<node_expr_ast> node;

  public:
    var_expr_ast(std::string name, std::unique_ptr<node_expr_ast> node)
        : name{name}, node{std::move(node)}
    {
    }

    Value* code_gen() override;
};

class node_io {
    std::string name;
    std::string field;

  public:
    node_io(std::string name, std::string field) : name{std::move(name)}, field{std::move(field)} {}
    const std::string& get_name() { return name; }
    const std::string& get_field() { return field; }
};

static int cur_tok;
static int get_next_tok() { return cur_tok = get_tok(); }

static std::map<std::string, std::unique_ptr<node_expr_ast>> nodes;
static std::vector<std::pair<node_io, node_io>> edges; // directed edges

// LogError* - These are little helper functions for error handling.
std::unique_ptr<expr_ast> LogError(const char* Str)
{
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

// static std::unique_ptr<expr_ast> parse_expr();

// number : ['+'|'-'] ('0' ... '9')+ ['.' ('0' ... '9')+]
static std::unique_ptr<expr_ast> parse_number_expr()
{
    auto res = std::make_unique<number_expr_ast>(num_val);
    get_next_tok();
    return std::move(res);
}

// string : '"' {/* any UTF-8 character */} '"'
static std::unique_ptr<expr_ast> parse_string_expr() {
    auto res = std::make_unique<string_expr_ast>(str_val);
    get_next_tok();
    return std::move(res);
}

// argument : number | string | list | fun_call ; 
static std::unique_ptr<expr_ast> parse_argument_expr();

// fun_call : id '(' {argument} ')'
static std::unique_ptr<expr_ast> parse_fun_call() {
    std::string name = id_name;

    get_next_tok(); // eat identifier
    if (cur_tok != '(') {
        return LogError("Expected '(' before function arguments");
    }
    get_next_tok(); // eat (

    std::vector<std::unique_ptr<expr_ast>> args;
    if (cur_tok != ')') {
        while (true) {
            if (auto arg = parse_argument_expr())
                args.push_back(std::move(arg));
            else
                return nullptr;

            if (cur_tok == ')')
                break;

            if (cur_tok != ',')
                return LogError("Expected ')' or ',' in argument list");
            get_next_tok();
        }
    }

    // Eat the ')'.
    get_next_tok();

    return std::make_unique<call_expr_ast>(name, std::move(args));
}

// list : '[' {argument} ']'
static std::unique_ptr<expr_ast> parse_list_expr() {

    get_next_tok(); // eat [

    std::vector<std::unique_ptr<expr_ast>> contents;
    if (cur_tok != ']') {
        while (true) {
            if (auto arg = parse_argument_expr())
                contents.push_back(std::move(arg));
            else
                return nullptr;

            if (cur_tok == ']')
                break;

            if (cur_tok != ',')
                return LogError("Expected ']' or ',' in list");
            get_next_tok();
        }
    }

    // Eat the ']'.
    get_next_tok();

    return std::make_unique<list_expr_ast>(std::move(contents));
}

// static std::unique_ptr<expr_ast> parse_paren_expr()
// {
//     get_next_tok(); // eat (.
//     auto V = parse_expr();
//     if (!V)
//         return nullptr;
//
//     if (cur_tok != ')')
//         return LogError("expected ')'");
//     get_next_tok();
//     return V;
// }
// static std::unique_ptr<expr_ast> parse_argument() {}
//
// static std::unique_ptr<expr_ast> parse_node_expr()
// {
//     std::string id_str = id_name;
//
//     if (cur_tok != tok_scope_res_op) {
//         return LogError("Expected \"::\"");
//     }
//     get_next_tok(); // eat "::"
//     if (cur_tok != Token::tok_id) {
//         return LogError("Expected node name");
//     }
//     std::string node_name = id_str; // check validity
//     if (cur_tok != '{') {
//         return LogError("Expected { in node declaration");
//     }
//     get_next_tok(); // eat {
//     std::map<std::string, std::unique_ptr<expr_ast>> fields;
//     while (cur_tok != '}') {
//         if (cur_tok != tok_id) {
//             return LogError("Expected field name in node declaration");
//         }
//         std::string arg_name = id_name;
//         fields[arg_name] = std::move(parse_argument());
//         if (cur_tok == ',') {
//             get_next_tok();
//         }
//     }
//     auto res = std::make_unique<node_expr_ast>(Category::Node, NodeName::Mix, fields);
//     get_next_tok(); // eat the '}'
//     return std::move(res);
// }
//
// static std::unique_ptr<expr_ast> parse_id_expr()
// {
//     std::string id_str = id_name;
//
//     get_next_tok(); // eat identifier.
//
//     if (cur_tok == '=') { // node variable declaration
//         get_next_tok();   // eat '='
//         auto node_defn = parse_node_expr();
//         if (!node_defn) {
//             return nullptr;
//         }
//         std::unique_ptr<node_expr_ast> node_ptr =
//             std::unique_ptr<node_expr_ast>(static_cast<node_expr_ast*>(node_defn.release()));
//         // auto var_defn = std::make_unique<var_expr_ast>(id_str, std::move(node_ptr));
//         nodes[id_str] = std::move(node_ptr);
//         return nullptr;
//     }
//
//     if (cur_tok == tok_id) { // edge
//         std::string first_field = id_name;
//
//         get_next_tok(); // eat field name
//         if (cur_tok != tok_arrow) {
//             return LogError("Expected arrow \"->\" in edge declaration");
//         }
//         get_next_tok(); // eat arrow
//         if (cur_tok != tok_id) {
//             return LogError("Expected node category in edge declaration");
//         }
//         std::string second_cat = id_name;
//         get_next_tok(); // eat second node var
//         if (cur_tok != tok_id) {
//             return LogError("Expected field name in edge declaration");
//         }
//         std::string second_field = id_name;
//         get_next_tok(); // eat field name
//
//         edges.push_back({node_io(id_str, first_field), node_io(second_cat, second_field)});
//         return nullptr;
//     }
//
//
// }
//
} // namespace

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
