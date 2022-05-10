#ifndef MTR_AST_H
#define MTR_AST_H

#include "typeList.h"
#include "scanner/token.h"
#include "symbol.h"

enum mtr_expr_type {
    MTR_EXPR_BINARY,
    MTR_EXPR_PRIMARY,
    MTR_EXPR_LITERAL,
    MTR_EXPR_ARRAY_LITERAL,
    MTR_EXPR_MAP_LITERAL,
    MTR_EXPR_GROUPING,
    MTR_EXPR_UNARY,
    MTR_EXPR_CALL,
    MTR_EXPR_CAST,
    MTR_EXPR_SUBSCRIPT,
    MTR_EXPR_ACCESS
};

struct mtr_expr {
    enum mtr_expr_type type;
};

struct mtr_cast {
    struct mtr_expr expr_;
    struct mtr_expr* right;
    struct mtr_type to;
};

struct mtr_unary {
    struct mtr_expr expr_;
    struct mtr_expr* right;
    struct mtr_symbol operator;
};

struct mtr_primary {
    struct mtr_expr expr_;
    struct mtr_symbol symbol;
};

struct mtr_literal {
    struct mtr_expr expr_;
    struct mtr_token literal;
};

struct mtr_array_literal {
    struct mtr_expr expr_;
    struct mtr_expr** expressions;
    u8 count;
};

struct mtr_map_entry {
    struct mtr_expr* key;
    struct mtr_expr* value;
};

struct mtr_map_literal {
    struct mtr_expr expr_;
    struct mtr_map_entry* entries;
    u8 count;
};

struct mtr_grouping {
    struct mtr_expr expr_;
    struct mtr_expr* expression;
};

struct mtr_binary {
    struct mtr_expr expr_;
    struct mtr_expr* right;
    struct mtr_expr* left;
    struct mtr_symbol operator;
};

struct mtr_call {
    struct mtr_expr expr_;
    struct mtr_expr* callable;
    struct mtr_expr** argv;
    u8 argc;
};

struct mtr_access {
    struct mtr_expr expr_;
    struct mtr_expr* object;
    struct mtr_expr* element;
};

enum mtr_stmt_type {
    MTR_STMT_ASSIGNMENT,
    MTR_STMT_STRUCT,
    MTR_STMT_UNION,
    MTR_STMT_FN,
    MTR_STMT_NATIVE_FN,
    MTR_STMT_CLOSURE,
    MTR_STMT_RETURN,
    MTR_STMT_VAR,
    MTR_STMT_IF,
    MTR_STMT_WHILE,
    MTR_STMT_BLOCK,
    MTR_STMT_CALL
};

struct mtr_stmt {
    enum mtr_stmt_type type;
};

struct mtr_block {
    struct mtr_stmt stmt;
    struct mtr_stmt** statements;
    size_t size;
    size_t capacity;
    u16 var_count;
};

struct mtr_if {
    struct mtr_stmt stmt;
    struct mtr_stmt* then;
    struct mtr_stmt* otherwise;
    struct mtr_expr* condition;
};

struct mtr_while {
    struct mtr_stmt stmt;
    struct mtr_stmt* body;
    struct mtr_expr* condition;
};

struct mtr_variable {
    struct mtr_stmt stmt;
    struct mtr_symbol symbol;
    struct mtr_expr* value;
};

// I dont know whether to use a token or a var to store args.
// Using a var stores a pointer to an expression for default arguments
// but it means I store types twice.
// I store args type in the function type so that it is available when type-checking a call
struct mtr_function_decl {
    struct mtr_stmt stmt;
    struct mtr_stmt* body;
    struct mtr_symbol symbol;
    struct mtr_variable* argv;
    u8 argc;
};

struct mtr_closed {
    size_t* captured;
    u8 count;
};

struct mtr_closure_decl {
    struct mtr_stmt stmt;
    struct mtr_function_decl* function;
    struct mtr_symbol* upvalues;
    u16 capacity;
    u16 count;
};

struct mtr_struct_decl {
    struct mtr_stmt stmt;
    struct mtr_symbol symbol;
    struct mtr_variable** members;
    u8 argc;
};

// union types are stored in the symbol type
struct mtr_union_decl {
    struct mtr_stmt stmt;
    struct mtr_symbol symbol;
};

struct mtr_return {
    struct mtr_stmt stmt;
    struct mtr_function_decl* from;
    struct mtr_expr* expr;
};

struct mtr_assignment {
    struct mtr_stmt stmt;
    struct mtr_expr* right;
    struct mtr_expr* expression;
};

struct mtr_call_stmt {
    struct mtr_stmt stmt;
    struct mtr_expr* call;
};

struct mtr_ast {
    struct mtr_stmt* head;
    struct mtr_type_list type_list;
    const char* source;
};

void mtr_free_stmt(struct mtr_stmt* s);
void mtr_free_expr(struct mtr_expr* node);

void mtr_delete_ast(struct mtr_ast* ast);

#endif
