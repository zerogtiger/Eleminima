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
            // std::cerr << "str\n";
            if (last_char == '\\') {
                str_val += last_char;
                last_char = getchar();
            }
            str_val += last_char;
            last_char = getchar();
        }
        last_char = getchar(); // eat "
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

Value* number_expr_ast::code_gen() { return nullptr; }

class string_expr_ast : public expr_ast {

    std::string str;

  public:
    string_expr_ast(std::string str) : str{str} {}

    Value* code_gen() override;
};

Value* string_expr_ast::code_gen() { return nullptr; }

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

Value* call_expr_ast::code_gen() { return nullptr; }

class list_expr_ast : public expr_ast {
    std::vector<std::unique_ptr<expr_ast>> content;

  public:
    list_expr_ast(std::vector<std::unique_ptr<expr_ast>> content) : content(std::move(content)) {}

    Value* code_gen() override;
};

Value* list_expr_ast::code_gen() { return nullptr; }

class field_assgn_ast : public expr_ast {

    std::string field_name;
    std::unique_ptr<expr_ast> argument;

  public:
    field_assgn_ast(std::string field_name, std::unique_ptr<expr_ast> argument)
        : field_name{field_name}, argument{std::move(argument)}
    {
    }

    Value* code_gen() override;
};

Value* field_assgn_ast::code_gen() { return nullptr; }

class node_io {
    std::string name;
    std::string field;

  public:
    node_io(std::string name, std::string field) : name{std::move(name)}, field{std::move(field)} {}
    const std::string& get_name() { return name; }
    const std::string& get_field() { return field; }
};

class edge_expr_ast : public expr_ast {

    node_io first, second;

  public:
    edge_expr_ast(node_io first, node_io second) : first{first}, second{second} {}

    Value* code_gen() override;
};

Value* edge_expr_ast::code_gen() { return nullptr; }

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

Value* node_expr_ast::code_gen() { return nullptr; }
} // namespace

static int cur_tok;
static int get_next_tok() { return cur_tok = get_tok(); }

static std::map<std::string, std::unique_ptr<node_expr_ast>> nodes;
static std::vector<std::pair<node_io, node_io>> edges; // directed edges

// LogError* - These are little helper functions for error handling.
std::unique_ptr<expr_ast> LogError(std::string str)
{
    fprintf(stderr, "Error: %s\n", str.c_str());
    return nullptr;
}

// static std::unique_ptr<expr_ast> parse_expr();

// number : ['+'|'-'] ('0' ... '9')+ ['.' ('0' ... '9')+]
static std::unique_ptr<expr_ast> parse_number_expr()
{
    auto res = std::make_unique<number_expr_ast>(num_val);
    std::cerr << "Parsed number: " << num_val << "\n";
    get_next_tok();
    return std::move(res);
}

// string : '"' {/* any UTF-8 character */} '"'
static std::unique_ptr<expr_ast> parse_string_expr()
{
    auto res = std::make_unique<string_expr_ast>(str_val);
    std::cerr << "Parsed string: " << str_val << "\n";
    get_next_tok();
    return std::move(res);
}

// argument : number | string | list | fun_call ;
static std::unique_ptr<expr_ast> parse_argument_expr();

// fun_call : id '(' {argument} ')'
static std::unique_ptr<expr_ast> parse_fun_call()
{
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

    std::cerr << "Parsed function call with: " << name << " and " << args.size() << " arguments\n";
    // Eat the ')'.
    get_next_tok();

    return std::make_unique<call_expr_ast>(name, std::move(args));
}

// list : '[' {argument} ']'
static std::unique_ptr<expr_ast> parse_list_expr()
{
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

    std::cerr << "Parsed list with " << contents.size() << " arguments\n";
    // Eat the ']'.
    get_next_tok();

    return std::make_unique<list_expr_ast>(std::move(contents));
}

// argument : number | string | list | fun_call
static std::unique_ptr<expr_ast> parse_argument_expr()
{
    if (cur_tok == tok_id) {
        return parse_fun_call();
    }
    if (cur_tok == tok_string) {
        return parse_string_expr();
    }
    if (cur_tok == '[') {
        return parse_list_expr();
    }
    if (cur_tok == tok_number) {
        return parse_number_expr();
    }
    return LogError("Unrecognized token for argument: " + std::string(1, (char) cur_tok));
}

// type : category '::' node_name
// static std::unique_ptr<expr_ast> parse_type_expr()
// {
//     std::string category = id_name;
//     get_next_tok(); // eat id name
//     if (cur_tok != tok_scope_res_op) {
//         return LogError("Expected '::' for type declareation");
//     }
//     get_next_tok(); // eat ::
//     std::string node_name = id_name;
// }

// definition : '=' type '{' { field_assignment } '}' ';' ;
static std::unique_ptr<expr_ast> parse_defn_expr()
{
    get_next_tok(); // eat '='

    if (cur_tok != tok_id) {
        return LogError("Expected category in node definition");
    }
    std::string id_str = id_name;
    get_next_tok(); // eat id

    if (cur_tok != tok_scope_res_op) {
        return LogError("Expected \"::\"");
    }
    get_next_tok();                 // eat "::"
    if (cur_tok != Token::tok_id) { // check validity
        return LogError("Expected node name");
    }
    std::string node_name = id_str;
    get_next_tok(); // eat node name

    if (cur_tok != '{') {
        return LogError("Expected { in node declaration");
    }
    get_next_tok(); // eat {

    std::map<std::string, std::unique_ptr<expr_ast>> fields;
    while (cur_tok != '}') {
        if (cur_tok != tok_id) {
            return LogError("Expected field name in node declaration");
        }
        std::string field_name = id_name;
        get_next_tok(); // eat id name

        if (cur_tok != ':') {
            return LogError("Expected ':' for field assignment");
        }
        get_next_tok(); // eat :

        auto arg = parse_argument_expr();
        if (!arg) {
            return nullptr;
        }
        std::cerr << "Parsed field assignment with name " << field_name << "\n";

        if (cur_tok == ',') {
            get_next_tok();
        }
        else {
            break;
        }
        fields[field_name] = std::move(std::move(arg));

        // auto res = std::make_unique<field_assgn_ast>(field_name, std::move(arg));
        // std::string arg_name = id_name;
        //
        // if (cur_tok == ',') {
        //     get_next_tok();
        // }
    }
    std::cerr <<  "Parsed node definition with " << fields.size() << " arguments\n";
    auto res = std::make_unique<node_expr_ast>(Category::Node, NodeName::Mix, std::move(fields));
    get_next_tok(); // eat the '}'
    return std::move(res);
}

// edge : out_field '->' id (' ')+ in_field ';' ;
static std::unique_ptr<expr_ast> parse_edge_expr(std::string first_cat)
{
    std::string first_field = id_name;
    get_next_tok(); // eat field name

    if (cur_tok != tok_arrow) {
        return LogError("Expected arrow \"->\" in edge declaration");
    }
    get_next_tok(); // eat arrow

    if (cur_tok != tok_id) {
        return LogError("Expected node category in edge declaration");
    }
    std::string second_cat = id_name;
    get_next_tok(); // eat second id

    if (cur_tok != tok_id) {
        return LogError("Expected field name in edge declaration");
    }
    std::string second_field = id_name;

    // perform necessary checks
    edges.push_back({node_io(first_cat, first_field), node_io(second_cat, second_field)});
    auto res = std::make_unique<edge_expr_ast>(node_io(first_cat, first_field),
                                               node_io(second_cat, second_field));
    std::cerr << "Parsed edge from " << first_cat << " " << first_field << " -> " << second_cat
              << " " << second_field << "\n";
    get_next_tok(); // eat field name
    return std::move(res);
}

// statement : id (' ')+ definition | id (' ')+ edge ;
static bool parse_statement_expr()
{
    if (cur_tok != tok_id) {
        LogError("Expected identifier in statement declaration");
        return false;
    }
    auto first_id = id_name;
    get_next_tok();
    if (cur_tok == '=') {
        auto node_defn = parse_defn_expr();
        if (!node_defn) {
            return false;
        }
        std::unique_ptr<node_expr_ast> node_ptr =
            std::unique_ptr<node_expr_ast>(static_cast<node_expr_ast*>(node_defn.release()));
        ::nodes[first_id] = std::move(node_ptr);
        return true;
    }
    else if (cur_tok == tok_id) {
        auto res = parse_edge_expr(first_id);
        if (!res) {
            return false;
        }
        return true;
    }
    LogError("Unrecognized token for statement");
    return false;
}

int main()
{
    get_next_tok();
    while (cur_tok != tok_eof) {
        if (!parse_statement_expr()) {
            break;
        }
    }
    return 0;
}

// int tmp;
// while ((tmp = get_tok()) != tok_eof) {
//     switch (tmp) {
//     case -1:
//         std::cerr << "EOF";
//         break;
//     case -2:
//         std::cerr << "Identifier: " << id_name;
//         break;
//     case -3:
//         std::cerr << "Number: " << num_val;
//         break;
//     case -4:
//         std::cerr << "String: " << str_val;
//         break;
//     case -5:
//         std::cerr << "Arrow";
//         break;
//     case -6:
//         std::cerr << "Scope res op";
//         break;
//     default:
//         std::cerr << (char) tmp;
//     }
//     std::cerr << "\n";
// }
// ---

// class var_expr_ast : public expr_ast {
//     std::string name;
//     std::unique_ptr<node_expr_ast> node;
//
//   public:
//     var_expr_ast(std::string name, std::unique_ptr<node_expr_ast> node)
//         : name{name}, node{std::move(node)}
//     {
//     }
//
//     Value* code_gen() override;
// };
//
// Value* var_expr_ast::code_gen() {
//     return nullptr;
// }

// field_assignment : id ':' argument [',']
// static std::unique_ptr<expr_ast> parse_field_assgn_expr()
// {
//     std::string field_name = id_name;
//     get_next_tok(); // eat id name
//     if (cur_tok != ':') {
//         return LogError("Expected ':' for field assignment");
//     }
//     get_next_tok(); // eat :
//     auto arg = parse_argument_expr();
//     if (!arg) {
//         return nullptr;
//     }
//     auto res = std::make_unique<field_assgn_ast>(field_name, std::move(arg));
//     std::cerr << "Parsed argument with name " << field_name << "\n";
//     get_next_tok();
//     return res;
// }

// // out_field : [type '::'] id
// static std::unique_ptr<expr_ast> parse_field_expr()
// {
//     std::string type = id_name;
//     get_next_tok(); // eat id name
// }
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

// category : 'Node'

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
