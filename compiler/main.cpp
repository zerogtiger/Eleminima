#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stack>
#include <string>

using namespace llvm;

enum Category {
    io,
    spacial,
    stats,
    color,
};

enum NodeName {
    crop,
    scale,
    histogram,
    mix,
    color_ramp,
    grayscale,
    image,
    output,
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

    double get_val() { return val; }

    Value* code_gen() override;
};

class string_expr_ast : public expr_ast {

    std::string str;

  public:
    string_expr_ast(std::string str) : str{str} {}

    Value* code_gen() override;
    std::string& get_str() { return str; }
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

class node_io {
    std::string name;
    std::string field;

  public:
    node_io() = default;
    node_io(std::string name, std::string field) : name{std::move(name)}, field{std::move(field)} {}
    const std::string& get_name() { return name; }
    const std::string& get_field() { return field; }

    bool operator==(const node_io& other) { return name == other.name && field == other.field; }
};

class edge_expr_ast : public expr_ast {

    node_io first, second;

  public:
    edge_expr_ast(node_io first, node_io second) : first{first}, second{second} {}

    Value* code_gen() override;
};

class node_expr_ast {
    Category cat;
    NodeName name;
    std::map<std::string, std::unique_ptr<expr_ast>> fields;
    std::map<std::string, node_io> in_edges, out_edges;

  public:
    node_expr_ast(Category cat, NodeName name,
                  std::map<std::string, std::unique_ptr<expr_ast>>&& fields)
        : cat{cat}, name{name}, fields{std::move(fields)}
    {
    }

    std::map<std::string, Value*> code_gen();
    Category get_cat() { return cat; }
    NodeName get_name() { return name; }

    void add_in_edge(std::string field_name, std::string dest_name, std::string dest_field)
    {
        in_edges[field_name] = ::node_io(dest_name, dest_field);
    }

    void add_out_edge(std::string field_name, std::string src_name, std::string src_field)
    {
        out_edges[field_name] = ::node_io(src_name, src_field);
    }

    std::map<std::string, node_io>& get_in_edges() { return in_edges; }

    std::map<std::string, node_io>& get_out_edges() { return out_edges; }
};

} // namespace

static int cur_tok;
static int get_next_tok() { return cur_tok = get_tok(); }

static std::map<std::string, std::unique_ptr<node_expr_ast>> nodes;

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

static bool is_valid_category(std::string cat)
{
    return cat == "color" || cat == "spacial" || cat == "stats" || cat == "io";
}

static bool is_valid_node_name_given_cat(std::string cat, std::string node_name)
{
    if (cat == "spacial") {
        return node_name == "crop" || node_name == "scale";
    }
    if (cat == "stats") {
        return node_name == "histogram";
    }
    if (cat == "color") {
        return node_name == "color_ramp" || node_name == "mix" || node_name == "grayscale";
    }
    if (cat == "io") {
        return node_name == "image" || node_name == "output";
    }
    return false;
}

static Category get_category(std::string cat)
{
    if (cat == "spacial") {
        return Category::spacial;
    }
    if (cat == "stats") {
        return Category::stats;
    }
    if (cat == "color") {
        return Category::color;
    }
    return Category::io;
}

static NodeName get_node_name(std::string nn)
{
    if (nn == "mix") {
        return NodeName::mix;
    }
    else if (nn == "grayscale") {
        return NodeName::grayscale;
    }
    else if (nn == "color_ramp") {
        return NodeName::color_ramp;
    }
    else if (nn == "image") {
        return NodeName::image;
    }
    else if (nn == "crop") {
        return NodeName::crop;
    }
    else if (nn == "scale") {
        return NodeName::scale;
    }
    else if (nn == "histogram") {
        return NodeName::histogram;
    }
    return NodeName::output;
}

// definition : '=' type '{' { field_assignment } '}' ';' ;
static std::unique_ptr<node_expr_ast> parse_defn_expr()
{
    get_next_tok(); // eat '='

    if (cur_tok != tok_id) {
        LogError("Expected category in node definition");
        return nullptr;
    }

    std::string cat = id_name;
    get_next_tok(); // eat id
    if (!is_valid_category(cat)) {
        LogError("Expected valid category name, got \"" + cat + "\"");
        return nullptr;
    }

    if (cur_tok != tok_scope_res_op) {
        LogError("Expected \"::\"");
        return nullptr;
    }
    get_next_tok(); // eat "::"
    if (cur_tok != Token::tok_id) {
        LogError("Expected node name");
        return nullptr;
    }
    std::string node_name = id_name;
    get_next_tok(); // eat node name
    if (!is_valid_node_name_given_cat(cat, node_name)) {
        LogError("Expected valid node name under \"" + cat + "\", got \"" + node_name + "\"");
        return nullptr;
    }

    if (cur_tok != '{') {
        LogError("Expected { in node declaration");
        return nullptr;
    }
    get_next_tok(); // eat {

    std::map<std::string, std::unique_ptr<expr_ast>> fields;
    while (cur_tok != '}') {
        if (cur_tok != tok_id) {
            LogError("Expected field name in node declaration");
            return nullptr;
        }
        std::string field_name = id_name;
        get_next_tok(); // eat id name

        if (cur_tok != ':') {
            LogError("Expected ':' for field assignment");
            return nullptr;
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
        fields[field_name] = std::move(arg);

        // fields.insert({field_name, std::move(arg)});

        // auto res = std::make_unique<field_assgn_ast>(field_name, std::move(arg));
        // std::string arg_name = id_name;
        //
        // if (cur_tok == ',') {
        //     get_next_tok();
        // }
    }
    std::cerr << "Parsed node definition with " << fields.size() << " arguments\n";
    auto res = std::make_unique<node_expr_ast>(get_category(cat), get_node_name(node_name),
                                               std::move(fields));
    get_next_tok(); // eat the '}'
    return std::move(res);
}

// Lots of room for efficiency improvementns
static bool contains_cycle(node_io in, node_io out)
{
    // bfs
    std::queue<std::string> q;
    std::set<std::string> visited;
    q.push(out.get_name());
    while (!q.empty()) {
        auto curr = q.front();
        q.pop();
        if (visited.count(curr)) {
            continue;
        }
        visited.insert(curr);
        if (curr == in.get_name()) {
            q.push(out.get_name());
        }
        for (auto& e : ::nodes[curr]->get_out_edges()) {
            q.push(e.second.get_name());
        }
    }

    return visited.count(in.get_name());
}

void add_edge(node_io src, node_io dest)
{

    auto dest_node = ::nodes[dest.get_name()].get();
    if (::nodes[dest.get_name()]->get_in_edges().count(dest.get_field())) {

        LogError("Pre-existing edge to " + dest.get_name() + " " + dest.get_field() +
                 " found, replaced with new edge");

        node_io old_src = dest_node->get_in_edges()[dest.get_field()];
        ::nodes[old_src.get_name()]->get_out_edges().erase(old_src.get_field());
    }
    dest_node->get_in_edges()[dest.get_field()] = src;
    ::nodes[src.get_name()]->get_out_edges()[src.get_field()] = dest;
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
        return LogError("Expected variable name in edge declaration");
    }

    std::string second_cat = id_name;
    get_next_tok(); // eat second id
    if (!::nodes.count(second_cat)) {
        LogError("Expected predefined variable name for edge declaration. Got \"" + second_cat +
                 "\" instead.");
    }

    if (cur_tok != tok_id) {
        return LogError("Expected field name in edge declaration");
    }
    std::string second_field = id_name;

    std::cerr << "Parsed edge from " << first_cat << " " << first_field << " -> " << second_cat
              << " " << second_field << "\n";
    auto res = std::make_unique<edge_expr_ast>(node_io(first_cat, first_field),
                                               node_io(second_cat, second_field));
    if (contains_cycle(node_io(first_cat, first_field), node_io(second_cat, second_field))) {
        LogError("Edge from " + first_cat + " " + first_field + " -> " + second_cat + " " +
                 second_field + " creates a cycle. Ignored.");
        get_next_tok(); // eat field name
        return std::move(res);
    }
    add_edge(node_io(first_cat, first_field), node_io(second_cat, second_field));
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
        if (::nodes.count(first_id)) {
            LogError("Expected distinct variable name for node declaration. Got \"" + first_id +
                     "\" instead.");
            return false;
        }
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
        if (!::nodes.count(first_id)) {
            LogError("Expected predefined variable name for edge declaration. Got \"" + first_id +
                     "\" instead.");
            return false;
        }
        auto res = parse_edge_expr(first_id);
        if (!res) {
            return false;
        }
        return true;
    }
    LogError("Unrecognized token for statement");
    return false;
}

// =====================
// Code Generation (IR)
// =====================

static std::unique_ptr<LLVMContext> context;
static std::unique_ptr<Module> module;
static std::unique_ptr<IRBuilder<>> ir_builder;
static std::map<std::string, std::map<std::string, Value*>> named_values;
static llvm::StructType *image_wrapper_type, *color_wrapper_type, *point_color_pair_type;

Value* log_error_v(std::string str)
{
    LogError(str);
    return nullptr;
}

void ir_type_def()
{
    context = std::make_unique<LLVMContext>();
    module = std::make_unique<Module>("Eleminima module", *context);
    ir_builder = std::make_unique<IRBuilder<>>(*context);

    // typedefs
    image_wrapper_type = llvm::StructType::create(*context, "ImageWrapper");
    image_wrapper_type->setBody(
        llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)); // void* instance

    color_wrapper_type = llvm::StructType::create(*context, "ColorWrapper");
    color_wrapper_type->setBody(
        llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)); // void* instance

    point_color_pair_type = llvm::StructType::create(*context, "PointColorPair");
    std::vector<llvm::Type*> point_color_struct_types = {
        llvm::Type::getDoubleTy(*context), llvm::PointerType::get(color_wrapper_type, 0)};
    point_color_pair_type->setBody(point_color_struct_types);

    // image_type = llvm::StructType::create(*context, "Image");
    // image_type->setBody({
    //     llvm::Type::getInt32Ty(*context),                          // width
    //     llvm::Type::getInt32Ty(*context),                          // height
    //     llvm::Type::getInt32Ty(*context),                          // channels
    //     llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0) // data (uint8_t*)
    // });

    // llvm::FunctionType* func_type = llvm::FunctionType::get(image_type, false);
    // llvm::Function* dummy_func = llvm::Function::Create(
    //     func_type, llvm::GlobalValue::ExternalLinkage, "dummy_func", module.get());
}

Value* extern_funtion_gen()
{
    FunctionType* func_type;
    Function* func;
    llvm::Argument* arg_iter;

    // ColorWrapper* color_create_default();
    func_type = FunctionType::get(llvm::PointerType::get(color_wrapper_type, 0), {}, false);
    func = Function::Create(func_type, Function::ExternalLinkage, "color_create_default",
                            module.get());

    // ColorWrapper* color_create_rgb(double r, double g, double b)
    func_type =
        FunctionType::get(llvm::PointerType::get(color_wrapper_type, 0),
                          {llvm::Type::getDoubleTy(*context), llvm::Type::getDoubleTy(*context),
                           llvm::Type::getDoubleTy(*context)},
                          false);
    func = Function::Create(func_type, Function::ExternalLinkage, "color_create_rgb", module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("r");
    (arg_iter++)->setName("g");
    (arg_iter++)->setName("b");

    // ColorWrapper* color_create_rgba(double r, double g, double b, double a)
    func_type =
        FunctionType::get(llvm::PointerType::get(color_wrapper_type, 0),
                          {llvm::Type::getDoubleTy(*context), llvm::Type::getDoubleTy(*context),
                           llvm::Type::getDoubleTy(*context), llvm::Type::getDoubleTy(*context)},
                          false);
    func =
        Function::Create(func_type, Function::ExternalLinkage, "color_create_rgba", module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("r");
    (arg_iter++)->setName("g");
    (arg_iter++)->setName("b");
    (arg_iter++)->setName("a");

    // void color_destroy(ColorWrapper* color)
    func_type = FunctionType::get(llvm::Type::getVoidTy(*context),
                                  llvm::PointerType::get(color_wrapper_type, 0), false);
    func = Function::Create(func_type, Function::ExternalLinkage, "color_destroy", module.get());
    func->arg_begin()->setName("color");

    // ImageWrapper* image_create_w_h_channels(int w, int h, int channels)
    func_type =
        FunctionType::get(llvm::PointerType::get(image_wrapper_type, 0),
                          {llvm::Type::getInt32Ty(*context), llvm::Type::getInt32Ty(*context),
                           llvm::Type::getInt32Ty(*context)},
                          false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_create_w_h_channels",
                            module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("w");
    (arg_iter++)->setName("h");
    (arg_iter++)->setName("channels");

    // ImageWrapper* image_create_w_h_channels_fill(int w, int h, int channels, ColorWrapper* fill)
    func_type = FunctionType::get(
        llvm::PointerType::get(image_wrapper_type, 0),
        {llvm::Type::getInt32Ty(*context), llvm::Type::getInt32Ty(*context),
         llvm::Type::getInt32Ty(*context), llvm::PointerType::get(color_wrapper_type, 0)},
        false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_create_w_h_channels_fill",
                            module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("w");
    (arg_iter++)->setName("h");
    (arg_iter++)->setName("channels");
    (arg_iter++)->setName("fill");

    // ImageWrapper* image_create_filename(const char* filename)
    func_type =
        FunctionType::get(llvm::PointerType::get(image_wrapper_type, 0),
                          {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)}, false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_create_from_file",
                            module.get());
    func->arg_begin()->setName("filename");

    // bool image_write(ImageWrapper* img, const char* filename)
    func_type = FunctionType::get(llvm::Type::getInt1Ty(*context),
                                  {
                                      llvm::PointerType::get(image_wrapper_type, 0),
                                      llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
                                  },
                                  false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_write", module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("img");
    (arg_iter++)->setName("filename");

    // void image_destroy(ImageWrapper* img)
    func_type = FunctionType::get(llvm::Type::getVoidTy(*context),
                                  llvm::PointerType::get(image_wrapper_type, 0), false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_destroy", module.get());
    func->arg_begin()->setName("img");

    // void image_grayscale_avg(ImageWrapper* img)
    func_type = FunctionType::get(llvm::Type::getVoidTy(*context),
                                  llvm::PointerType::get(image_wrapper_type, 0), false);
    func =
        Function::Create(func_type, Function::ExternalLinkage, "image_grayscale_avg", module.get());
    func->arg_begin()->setName("img");

    // void image_grayscale_lum(ImageWrapper* img)
    func_type = FunctionType::get(llvm::Type::getVoidTy(*context),
                                  llvm::PointerType::get(image_wrapper_type, 0), false);
    func =
        Function::Create(func_type, Function::ExternalLinkage, "image_grayscale_lum", module.get());
    func->arg_begin()->setName("img");

    // void image_crop(ImageWrapper* img, uint16_t cx, uint16_t cy, uint16_t cw, uint16_t ch)
    func_type =
        FunctionType::get(llvm::Type::getVoidTy(*context),
                          {llvm::PointerType::get(image_wrapper_type, 0),
                           llvm::Type::getDoubleTy(*context), llvm::Type::getDoubleTy(*context),
                           llvm::Type::getDoubleTy(*context), llvm::Type::getDoubleTy(*context)},
                          false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_crop", module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("img");
    (arg_iter++)->setName("cx");
    (arg_iter++)->setName("cy");
    (arg_iter++)->setName("cw");
    (arg_iter++)->setName("ch");

    // void image_f_scale(ImageWrapper* img, uint32_t new_w, uint32_t new_h, bool linked,
    //      TwoDimInterpC method)
    func_type =
        FunctionType::get(llvm::Type::getVoidTy(*context),
                          {llvm::PointerType::get(image_wrapper_type, 0),
                           llvm::Type::getInt32Ty(*context), llvm::Type::getInt32Ty(*context),
                           llvm::Type::getInt1Ty(*context), llvm::Type::getInt32Ty(*context)},
                          false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_f_scale", module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("img");
    (arg_iter++)->setName("new_w");
    (arg_iter++)->setName("new_h");
    (arg_iter++)->setName("linked");
    (arg_iter++)->setName("method");

    // ImageWrapper* image_histogram(ImageWrapper* img, bool inc_lum)
    func_type = FunctionType::get(
        llvm::PointerType::get(image_wrapper_type, 0),
        {llvm::PointerType::get(image_wrapper_type, 0), llvm::Type::getInt1Ty(*context)}, false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_histogram", module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("img");
    (arg_iter++)->setName("inc_lum");

    // void image_color_ramp(ImageWrapper* img, PointColorPair* points, size_t points_count,
    //      OneDimInterpC method)
    func_type = FunctionType::get(llvm::Type::getVoidTy(*context),
                                  {
                                      llvm::PointerType::get(image_wrapper_type, 0),
                                      llvm::PointerType::get(point_color_pair_type, 0),
                                      llvm::Type::getInt32Ty(*context),
                                      llvm::Type::getInt32Ty(*context),
                                  },
                                  false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_color_ramp", module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("img");
    (arg_iter++)->setName("points");
    (arg_iter++)->setName("points_count");
    (arg_iter++)->setName("method");

    // ImageWrapper* image_preview_color_ramp(ImageWrapper* img, PointColorPair* points,
    //      size_t points_count, OneDimInterpC method)
    func_type = FunctionType::get(llvm::PointerType::get(image_wrapper_type, 0),
                                  {
                                      llvm::PointerType::get(image_wrapper_type, 0),
                                      llvm::PointerType::get(point_color_pair_type, 0),
                                      llvm::Type::getInt32Ty(*context),
                                      llvm::Type::getInt32Ty(*context),
                                  },
                                  false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_color_ramp", module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("img");
    (arg_iter++)->setName("points");
    (arg_iter++)->setName("points_count");
    (arg_iter++)->setName("method");

    // void image_alpha_overlay_img_img(ImageWrapper* img, ImageWrapper* fac, int fac_x, int fac_y,
    //      ImageWrapper* other, int other_x, int other_y)
    func_type = FunctionType::get(llvm::Type::getVoidTy(*context),
                                  {
                                      llvm::PointerType::get(image_wrapper_type, 0),
                                      llvm::PointerType::get(image_wrapper_type, 0),
                                      llvm::Type::getInt32Ty(*context),
                                      llvm::Type::getInt32Ty(*context),
                                      llvm::PointerType::get(image_wrapper_type, 0),
                                      llvm::Type::getInt32Ty(*context),
                                      llvm::Type::getInt32Ty(*context),
                                  },
                                  false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_alpha_overlay_img_img",
                            module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("img");
    (arg_iter++)->setName("fac");
    (arg_iter++)->setName("fac_x");
    (arg_iter++)->setName("fac_y");
    (arg_iter++)->setName("other");
    (arg_iter++)->setName("other_x");
    (arg_iter++)->setName("other_y");

    // void image_alpha_overlay_color_img(ImageWrapper* img, ColorWrapper* color, ImageWrapper*
    // other,
    //      int other_x, int other_y)
    func_type = FunctionType::get(llvm::Type::getVoidTy(*context),
                                  {
                                      llvm::PointerType::get(image_wrapper_type, 0),
                                      llvm::PointerType::get(color_wrapper_type, 0),
                                      llvm::PointerType::get(image_wrapper_type, 0),
                                      llvm::Type::getInt32Ty(*context),
                                      llvm::Type::getInt32Ty(*context),
                                  },
                                  false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_alpha_overlay_color_img",
                            module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("img");
    (arg_iter++)->setName("color");
    (arg_iter++)->setName("other");
    (arg_iter++)->setName("other_x");
    (arg_iter++)->setName("other_y");

    // void image_alpha_overlay_color_color(ImageWrapper* img, ColorWrapper* color, ColorWrapper*
    // other)
    func_type = FunctionType::get(llvm::Type::getVoidTy(*context),
                                  {
                                      llvm::PointerType::get(image_wrapper_type, 0),
                                      llvm::PointerType::get(color_wrapper_type, 0),
                                      llvm::PointerType::get(color_wrapper_type, 0),
                                  },
                                  false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_alpha_overlay_color_color",
                            module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("img");
    (arg_iter++)->setName("color");
    (arg_iter++)->setName("other");

    // void image_alpha_overlay_img_color(ImageWrapper* img, ImageWrapper* fac, int fac_x, int
    // fac_y,
    //      ColorWrapper* other)
    func_type = FunctionType::get(llvm::Type::getVoidTy(*context),
                                  {
                                      llvm::PointerType::get(image_wrapper_type, 0),
                                      llvm::PointerType::get(image_wrapper_type, 0),
                                      llvm::Type::getInt32Ty(*context),
                                      llvm::Type::getInt32Ty(*context),
                                      llvm::PointerType::get(color_wrapper_type, 0),
                                  },
                                  false);
    func = Function::Create(func_type, Function::ExternalLinkage, "image_alpha_overlay_img_color",
                            module.get());
    arg_iter = func->arg_begin();
    (arg_iter++)->setName("img");
    (arg_iter++)->setName("fac");
    (arg_iter++)->setName("fac_x");
    (arg_iter++)->setName("fac_y");
    (arg_iter++)->setName("other");

    // // Look up the name in the global module table.
    // Function* callee_f = module->getFunction("image_create_from_file");
    //
    // if (!callee_f)
    //     return log_error_v("Unknown function referenced");
    //
    // std::vector<Value*> args_v;
    //
    // func_type = llvm::FunctionType::get(llvm::Type::getVoidTy(*context), {}, false);
    // func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "main",
    // module.get()); llvm::BasicBlock* bb = llvm::BasicBlock::Create(*context, "entry", func);
    // ir_builder->SetInsertPoint(bb);
    //
    // llvm::Value* str_ptr = ir_builder->CreateGlobalStringPtr(
    //     "/Users/tigerding/Projects/eleminima/runtime/demo/original/demo.jpeg", "str");
    // args_v.push_back(str_ptr);
    //
    // if (callee_f->arg_size() != args_v.size())
    //     return log_error_v("Incorrect # arguments passed");
    //
    // auto calltmp = ir_builder->CreateCall(callee_f, args_v, "calltmp");
    //
    // args_v.clear();
    // // Look up the name in the global module table.
    // callee_f = module->getFunction("image_grayscale_avg");
    // args_v.push_back(calltmp);
    //
    // if (!callee_f)
    //     return log_error_v("Unknown function referenced");
    //
    // if (callee_f->arg_size() != args_v.size())
    //     return log_error_v("Incorrect # arguments passed");
    //
    // ir_builder->CreateCall(callee_f, args_v);
    //
    // args_v.clear();
    // // Look up the name in the global module table.
    // callee_f = module->getFunction("image_write");
    // args_v.push_back(calltmp);
    // str_ptr =
    //     ir_builder->CreateGlobalStringPtr("/Users/tigerding/Projects/eleminima/demo.jpeg",
    //     "str");
    // args_v.push_back(str_ptr);
    //
    // if (!callee_f)
    //     return log_error_v("Unknown function referenced");
    //
    // if (callee_f->arg_size() != args_v.size())
    //     return log_error_v("Incorrect # arguments passed");
    //
    // calltmp = ir_builder->CreateCall(callee_f, args_v);
    // ir_builder->CreateRetVoid();

    return nullptr;
}

Value* number_expr_ast::code_gen() { return ConstantFP::get(*context, APFloat(val)); }

Value* string_expr_ast::code_gen() { return ir_builder->CreateGlobalStringPtr(str, "str"); }

Value* call_expr_ast::code_gen()
{
    // Look up the name in the global module table.
    Function* callee_f = module->getFunction(callee);
    if (!callee_f)
        return log_error_v("Unknown function referenced");

    // If argument mismatch error.
    if (callee_f->arg_size() != args.size())
        return log_error_v("Incorrect # arguments passed");

    std::vector<Value*> args_v;
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
        args_v.push_back(args[i]->code_gen());
        if (!args_v.back())
            return nullptr;
    }
    return ir_builder->CreateCall(callee_f, args_v, "calltmp");
}

Value* list_expr_ast::code_gen() { return nullptr; }

Value* field_assgn_ast::code_gen() { return nullptr; }

Value* edge_expr_ast::code_gen() { return nullptr; }

std::map<std::string, Value*> node_expr_ast::code_gen()
{
    if (cat == io && name == image) {

        // check src before accessing
        string_expr_ast* src = static_cast<string_expr_ast*>(fields["src"].get());

        std::vector<Value*> args_v;

        args_v.push_back(src->code_gen());

        Function* callee_f = module->getFunction("image_create_from_file");

        if (!callee_f) {
            log_error_v("Unknown function referenced");
            return {};
        }

        if (callee_f->arg_size() != args_v.size()) {
            log_error_v("Incorrect # arguments passed");
            return {};
        }

        return {{"image", ir_builder->CreateCall(callee_f, args_v)}};
    }
    else if (cat == io && name == output) {

        // check src before accessing
        string_expr_ast* dest = static_cast<string_expr_ast*>(fields["dest"].get());

        std::vector<Value*> args_v;

        args_v.push_back(named_values[in_edges["image"].get_name()][in_edges["image"].get_field()]);
        args_v.push_back(dest->code_gen());

        Function* callee_f = module->getFunction("image_write");

        if (!callee_f) {
            log_error_v("Unknown function referenced");
            return {};
        }

        if (callee_f->arg_size() != args_v.size()) {
            log_error_v("Incorrect # arguments passed");
            return {};
        }

        ir_builder->CreateCall(callee_f, args_v);
        return {};
    }
    else if (cat == color && name == grayscale) {

        std::vector<Value*> args_v;
        Function* callee_f;
        if (fields.count("method") &&
            static_cast<string_expr_ast*>(fields["method"].get())->get_str() == "avg") {
            callee_f = module->getFunction("image_grayscale_avg");
        }
        else {
            callee_f = module->getFunction("image_grayscale_lum");
        }
        args_v.push_back(named_values[in_edges["image"].get_name()][in_edges["image"].get_field()]);

        if (!callee_f) {
            log_error_v("Unknown function referenced");
            return {};
        }

        if (callee_f->arg_size() != args_v.size()) {
            log_error_v("Incorrect # arguments passed");
            return {};
        }

        ir_builder->CreateCall(callee_f, args_v);
        return {
            {"image", named_values[in_edges["image"].get_name()][in_edges["image"].get_field()]}};
    }
    else if (cat == stats && name == histogram) {

        std::vector<Value*> args_v;
        Function* callee_f = module->getFunction("image_histogram");

        args_v.push_back(named_values[in_edges["image"].get_name()][in_edges["image"].get_field()]);
        if (fields.count("include_lum") &&
            static_cast<number_expr_ast*>(fields["include_lum"].get())->get_val() == 0) {
            args_v.push_back(llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), 0));
        }
        else {
            args_v.push_back(llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), 1));
        }

        if (!callee_f) {
            log_error_v("Unknown function referenced");
            return {};
        }

        if (callee_f->arg_size() != args_v.size()) {
            log_error_v("Incorrect # arguments passed");
            return {};
        }

        return {{"preview", ir_builder->CreateCall(callee_f, args_v)}};
    }
    else if (cat == spacial && name == crop) {

        std::vector<Value*> args_v;
        Function* callee_f = module->getFunction("image_crop");

        args_v.push_back(named_values[in_edges["image"].get_name()][in_edges["image"].get_field()]);
        if (fields.count("start_x")) {
            args_v.push_back(fields["start_x"]->code_gen());
        }
        else {
            log_error_v("Error: expected start_x in crop node declaration.");
            return {};
        }

        if (fields.count("start_y")) {
            args_v.push_back(fields["start_y"]->code_gen());
        }
        else {
            log_error_v("Error: expected start_y in crop node declaration.");
            return {};
        }

        if (fields.count("width")) {
            args_v.push_back(fields["width"]->code_gen());
        }
        else {
            log_error_v("Error: expected width in crop node declaration.");
            return {};
        }

        if (fields.count("height")) {
            args_v.push_back(fields["height"]->code_gen());
        }
        else {
            log_error_v("Error: expected height in crop node declaration.");
            return {};
        }

        if (!callee_f) {
            log_error_v("Unknown function referenced");
            return {};
        }

        if (callee_f->arg_size() != args_v.size()) {
            log_error_v("Incorrect # arguments passed");
            return {};
        }

        return {
            {"image", named_values[in_edges["image"].get_name()][in_edges["image"].get_field()]}};
    }
    // else if (cat == spacial && name == scale) {
    //
    //     std::vector<Value*> args_v;
    //     Function* callee_f = module->getFunction("image_crop");
    //
    //     args_v.push_back(named_values[in_edges["image"].get_name()][in_edges["image"].get_field()]);
    //     if (fields.count("start_x")) {
    //         args_v.push_back(fields["start_x"]->code_gen());
    //     }
    //     else {
    //         log_error_v("Error: expected start_x in crop node declaration.");
    //         return {};
    //     }
    //
    //     if (fields.count("start_y")) {
    //         args_v.push_back(fields["start_y"]->code_gen());
    //     }
    //     else {
    //         log_error_v("Error: expected start_y in crop node declaration.");
    //         return {};
    //     }
    //
    //     if (fields.count("width")) {
    //         args_v.push_back(fields["width"]->code_gen());
    //     }
    //     else {
    //         log_error_v("Error: expected width in crop node declaration.");
    //         return {};
    //     }
    //
    //     if (fields.count("height")) {
    //         args_v.push_back(fields["height"]->code_gen());
    //     }
    //     else {
    //         log_error_v("Error: expected height in crop node declaration.");
    //         return {};
    //     }
    //
    //     if (!callee_f) {
    //         log_error_v("Unknown function referenced");
    //         return {};
    //     }
    //
    //     if (callee_f->arg_size() != args_v.size()) {
    //         log_error_v("Incorrect # arguments passed");
    //         return {};
    //     }
    //
    //     return {{"image", ir_builder->CreateCall(callee_f, args_v)}};
    // }

    return {};
}

void code_gen()
{
    FunctionType* func_type = llvm::FunctionType::get(llvm::Type::getVoidTy(*context), {}, false);
    Function* func =
        llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "main", module.get());
    llvm::BasicBlock* bb = llvm::BasicBlock::Create(*context, "entry", func);
    ir_builder->SetInsertPoint(bb);

    std::vector<std::pair<std::string, std::string>> st;
    for (auto& node : ::nodes) {
        // for each output, go through entire tree again
        if (node.second->get_name() == NodeName::output) {
            st.clear();
            st.push_back({node.first, "image"}); // current output
            // for (auto& e : node.second->get_in_edges()) {
            //     st.push_back(e.second.get_name()); // push all input nodes into stack
            // }
        }
        std::map<std::string, bool> visited;
        while (!st.empty()) {
            auto curr = st.back();
            if (visited[curr.first]) {
                st.pop_back();
                named_values[curr.first] = ::nodes[curr.first]->code_gen();
            }
            else {
                visited[curr.first] = true;
                for (auto& e : ::nodes[curr.first]->get_in_edges()) {
                    st.push_back({e.second.get_name(), e.second.get_field()});
                }
            }
        }
    }
    ir_builder->CreateRetVoid();
}

int main()
{
    get_next_tok();
    while (cur_tok != tok_eof) {
        if (!parse_statement_expr()) {
            break;
        }
    }

    ir_type_def();
    extern_funtion_gen();

    code_gen();

    module->print(errs(), nullptr);
    std::error_code ec;
    llvm::raw_fd_ostream out_file("module_output.ll", ec, sys::fs::OF_None);
    module->print(out_file, nullptr);
    out_file.close();
}
