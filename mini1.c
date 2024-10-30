#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_REGISTERS 256
#define MAX_LENGTH 200
#define MAX_INSTRUCTIONS 100
typedef enum {
    ASSIGN,
    ADD,
    SUB,
    MUL,
    DIV,
    REM,
    PREINC,
    PREDEC,
    POSTINC,
    POSTDEC,
    IDENTIFIER,
    CONSTANT,
    LPAR,
    RPAR,
    PLUS,
    MINUS,
    END
} Kind;
typedef struct {
    char instruction[MAX_LENGTH];
} Instruction;
typedef enum {
    STMT,
    EXPR,
    ASSIGN_EXPR,
    ADD_EXPR,
    MUL_EXPR,
    UNARY_EXPR,
    POSTFIX_EXPR,
    PRI_EXPR
} GrammarState;
typedef struct TokenUnit {
    Kind kind;
    int val;  // 記錄整數值或變量名稱
    struct TokenUnit* next;
} Token;
typedef struct ASTUnit {
    Kind kind;
    int val;  // 記錄整數值或變量名稱
    struct ASTUnit *lhs, *mid, *rhs;
} AST;

#define err(x)                                                \
    {                                                         \
        puts("Compile Error!");                               \
        if (DEBUG) {                                          \
            fprintf(stderr, "Error at line: %d\n", __LINE__); \
            fprintf(stderr, "Error message: %s\n", x);        \
        }                                                     \
        exit(0);                                              \
    }

#define DEBUG 0

Token* lexer(const char* in);
Token* new_token(Kind kind, int val);
size_t token_list_to_arr(Token** head);
AST* parser(Token* arr, size_t len);
AST* parse(Token* arr, int l, int r, GrammarState S);
AST* new_AST(Kind kind, int val);
int findNextSection(Token* arr, int start, int end, int (*cond)(Kind));
int condASSIGN(Kind kind);
int condADD(Kind kind);
int condMUL(Kind kind);
int condRPAR(Kind kind);
void semantic_check(AST* now);
void freeAST(AST* now);
void token_print(Token* in, size_t len);
void AST_print(AST* head);
int get_register_for_variable(char var);
int is_constant(AST* root);
int have_identifier(AST* root);
int codegen2(AST* root);

char input[MAX_LENGTH];
int reg[NUM_REGISTERS] = {0};

int main() {
    while (fgets(input, MAX_LENGTH, stdin) != NULL) {
        Token* content = lexer(input);
        size_t len = token_list_to_arr(&content);
        if (len == 0)
            continue;
        AST* ast_root = parser(content, len);
        semantic_check(ast_root);
        for (int i = 0; i < NUM_REGISTERS; i++)  // Initialize
            reg[i] = 0;
        codegen2(ast_root);
        free(content);
        freeAST(ast_root);
    }
    return 0;
}

Token* lexer(const char* in) {
    Token* head = NULL;
    Token** now = &head;
    for (int i = 0; in[i]; i++) {
        if (isspace(in[i]))  // 忽略空白字符
            continue;
        else if (isdigit(in[i])) {
            (*now) = new_token(CONSTANT, atoi(in + i));
            while (in[i + 1] && isdigit(in[i + 1]))
                i++;
        } else if ('x' <= in[i] && in[i] <= 'z')  // 變量
            (*now) = new_token(IDENTIFIER, in[i]);
        else
            switch (in[i]) {
                case '=':
                    (*now) = new_token(ASSIGN, 0);
                    break;
                case '+':
                    if (in[i + 1] && in[i + 1] == '+') {
                        i++;  // 在lexer範圍內，所有"++"都將被標記為PREINC。
                        (*now) = new_token(PREINC, 0);
                    }  // 在lexer範圍內，所有單個"+"都將被標記為PLUS。
                    else
                        (*now) = new_token(PLUS, 0);
                    break;
                case '-':
                    if (in[i + 1] && in[i + 1] == '-') {
                        i++;  // 在lexer範圍內，所有"--"都將被標記為PREDEC。

                        (*now) = new_token(PREDEC, 0);
                    }  // 在lexer範圍內，所有單個"-"都將被標記為MINUS。
                    else
                        (*now) = new_token(MINUS, 0);
                    break;
                case '*':
                    (*now) = new_token(MUL, 0);
                    break;
                case '/':
                    (*now) = new_token(DIV, 0);
                    break;
                case '%':
                    (*now) = new_token(REM, 0);
                    break;
                case '(':
                    (*now) = new_token(LPAR, 0);
                    break;
                case ')':
                    (*now) = new_token(RPAR, 0);
                    break;
                case ';':
                    (*now) = new_token(END, 0);
                    break;
                default:
                    err("Unexpected character.");
            }
        now = &((*now)->next);
    }
    return head;
}

Token* new_token(Kind kind, int val) {
    Token* res = (Token*)malloc(sizeof(Token));
    res->kind = kind;
    res->val = val;
    res->next = NULL;
    return res;
}

size_t token_list_to_arr(Token** head) {
    size_t res;
    Token *now = (*head), *del;
    for (res = 0; now != NULL; res++)
        now = now->next;
    now = (*head);
    if (res != 0)
        (*head) = (Token*)malloc(sizeof(Token) * res);
    for (int i = 0; i < res; i++) {
        (*head)[i] = (*now);
        del = now;
        now = now->next;
        free(del);
    }
    return res;
}

AST* parser(Token* arr, size_t len) {
    for (int i = 1; i < len; i++) {  // 正確識別"ADD"和"SUB"
        if (arr[i].kind == PLUS || arr[i].kind == MINUS) {
            switch (arr[i - 1].kind) {
                case PREINC:
                case PREDEC:
                case IDENTIFIER:
                case CONSTANT:
                case RPAR:
                    arr[i].kind = arr[i].kind - PLUS + ADD;
                default:
                    break;
            }
        }
    }
    return parse(arr, 0, len - 1, STMT);
}

AST* parse(Token* arr, int l, int r, GrammarState S) {
    AST* now = NULL;
    if (l > r)
        err("Unexpected parsing range.");
    int nxt;
    switch (S) {
        case STMT:
            if (l == r && arr[l].kind == END)
                return NULL;
            else if (arr[r].kind == END)
                return parse(arr, l, r - 1, EXPR);
            else
                err("Expected \';\' at the end of line.");
        case EXPR:
            return parse(arr, l, r, ASSIGN_EXPR);
        case ASSIGN_EXPR:
            if ((nxt = findNextSection(arr, l, r, condASSIGN)) != -1) {
                now = new_AST(arr[nxt].kind, 0);
                now->lhs = parse(arr, l, nxt - 1, UNARY_EXPR);
                now->rhs = parse(arr, nxt + 1, r, ASSIGN_EXPR);
                return now;
            }
            return parse(arr, l, r, ADD_EXPR);
        case ADD_EXPR:  // 加法符
            if ((nxt = findNextSection(arr, r, l, condADD)) != -1) {
                now = new_AST(arr[nxt].kind, 0);
                now->lhs = parse(arr, l, nxt - 1, ADD_EXPR);
                now->rhs = parse(arr, nxt + 1, r, MUL_EXPR);
                return now;
            }
            return parse(arr, l, r, MUL_EXPR);
        case MUL_EXPR:  // 乘法符 TODO
            if ((nxt = findNextSection(arr, r, l, condMUL)) != -1) {
                now = new_AST(arr[nxt].kind, 0);
                now->lhs = parse(arr, l, nxt - 1, MUL_EXPR);
                now->rhs = parse(arr, nxt + 1, r, UNARY_EXPR);
                return now;
            }
            return parse(arr, l, r, UNARY_EXPR);
        case UNARY_EXPR:
            if (arr[l].kind == SUB || arr[l].kind == MINUS || arr[l].kind == PREINC || arr[l].kind == PREDEC ||
                arr[l].kind == PLUS) {
                now = new_AST(arr[l].kind, 0);
                now->mid = parse(arr, l + 1, r, UNARY_EXPR);
                return now;
            }
            return parse(arr, l, r, POSTFIX_EXPR);
        case POSTFIX_EXPR:
            if (arr[r].kind == PREINC || arr[r].kind == PREDEC) {  // 將"PREINC"、"PREDEC"轉換為"POSTINC"、"POSTDEC"
                now = new_AST(arr[r].kind - PREINC + POSTINC, 0);
                now->mid = parse(arr, l, r - 1, POSTFIX_EXPR);
                return now;
            }
            return parse(arr, l, r, PRI_EXPR);
        case PRI_EXPR:
            if (findNextSection(arr, l, r, condRPAR) == r) {
                now = new_AST(LPAR, 0);
                now->mid = parse(arr, l + 1, r - 1, EXPR);
                return now;
            }
            if (l == r) {
                if (arr[l].kind == IDENTIFIER || arr[l].kind == CONSTANT)
                    return new_AST(arr[l].kind, arr[l].val);
                err("Unexpected token during parsing.");
            }
            printf("Error at line: %d, l: %d, r: %d\n", __LINE__, l, r);
            err("No token left for parsing.");
        default:
            err("Unexpected grammar state.");
    }
}

AST* new_AST(Kind kind, int val) {
    AST* res = (AST*)malloc(sizeof(AST));
    res->kind = kind;
    res->val = val;
    res->lhs = res->mid = res->rhs = NULL;
    return res;
}

int findNextSection(Token* arr, int start, int end, int (*cond)(Kind)) {
    int par = 0;
    int d = (start < end) ? 1 : -1;
    for (int i = start; (start < end) ? (i <= end) : (i >= end); i += d) {
        if (arr[i].kind == LPAR)
            par++;
        if (arr[i].kind == RPAR)
            par--;
        if (par == 0 && cond(arr[i].kind) == 1)
            return i;
    }
    return -1;
}

int condASSIGN(Kind kind) {
    return kind == ASSIGN;
}

int condADD(Kind kind) {
    return kind == ADD || kind == SUB;
}

int condMUL(Kind kind) {
    return kind == MUL || kind == DIV || kind == REM;
}

int condRPAR(Kind kind) {
    return kind == RPAR;
}

void semantic_check(AST* now) {
    if (now == NULL)
        return;
    if (now->kind == ASSIGN) {
        AST* tmp = now->lhs;
        while (tmp->kind == LPAR)
            tmp = tmp->mid;
        if (tmp->kind != IDENTIFIER)
            err("Lvalue is required as left operand of assignment.");
    }
    if (now->kind == PREINC || now->kind == PREDEC || now->kind == POSTINC || now->kind == POSTDEC) {
        AST* tmp = now->mid;
        while (tmp->kind == LPAR)
            tmp = tmp->mid;
        if (tmp->kind != IDENTIFIER)
            err("Operand of INC/DEC must be an identifier or identifier with parentheses.");
    }
    semantic_check(now->lhs);
    semantic_check(now->mid);
    semantic_check(now->rhs);
}

int get_register_for_variable(char var) {
    switch (var) {
        case 'x':
            return 0;  // x 存儲在 memory[0]
        case 'y':
            return 4;  // y 存儲在 memory[4]
        case 'z':
            return 8;  // z 存儲在 memory[8]
        default:
            return -1;  // 錯誤
    }
}

int is_constant(AST* root) {
    while (root->kind == LPAR) {
        if (root->mid->kind != LPAR && root->mid->kind != CONSTANT)
            return 0;
        if (root->mid->kind == LPAR)
            root = root->mid;
        else if (root->mid->kind == CONSTANT)
            return 1;
        else
            return 0;
    }
}

int have_identifier(AST* root) {
    while (root->kind == LPAR) {
        if (root->mid->kind != LPAR && root->mid->kind != IDENTIFIER)
            return 0;
        if (root->mid->kind == LPAR)
            root = root->mid;
        else if (root->mid->kind == IDENTIFIER)
            return root->mid->val;
        else
            return 0;
    }
    return 0;
}

int codegen2(AST* root) {
    if (root == NULL)
        return -1;
    int lv, rv, r, is_lc, is_rc, vr;
    switch (root->kind) {
        case ASSIGN:
            vr = have_identifier(root->lhs);
            if (vr == 0)
                vr = root->lhs->val;
            rv = codegen2(root->rhs);
            is_rc = 0;
            if (root->rhs->kind == CONSTANT)
                is_rc = 1;
            if (root->rhs->kind == LPAR && is_constant(root->rhs))
                is_rc = 1;
            if (is_rc == 1) {
                for (int i = 0; i < NUM_REGISTERS; i++) {
                    if (reg[i] == 0) {
                        reg[i] = 1;
                        r = i;
                        break;
                    }
                }
                printf("add r%d %d %d\n", r, 0, rv);
                printf("store [%d] r%d\n", get_register_for_variable(vr), r);
                reg[rv] = 0;
                return r;
            } else
                printf("store [%d] r%d\n", get_register_for_variable(vr), rv);
            break;
        case ADD:
            lv = codegen2(root->lhs);
            rv = codegen2(root->rhs);
            is_lc = (root->lhs->kind == CONSTANT || (root->lhs->kind == LPAR && is_constant(root->lhs))) ? 1 : 0;
            is_rc = (root->rhs->kind == CONSTANT || (root->rhs->kind == LPAR && is_constant(root->rhs))) ? 1 : 0;
            if ((is_lc == 0) & (is_rc == 0)) {
                printf("add r%d r%d r%d\n", lv, lv, rv);
                reg[rv] = 0;
                return lv;
            } else if ((is_lc == 0) & (is_rc == 1)) {
                printf("add r%d r%d %d\n", lv, lv, rv);
                return lv;
            } else if ((is_lc == 1) & (is_rc == 0)) {
                printf("add r%d %d r%d\n", rv, lv, rv);
                return rv;
            } else {
                for (int i = 0; i < NUM_REGISTERS; i++) {
                    if (reg[i] == 0) {
                        reg[i] = 1;
                        r = i;
                        break;
                    }
                }
                printf("add r%d %d %d\n", r, lv, rv);
                return r;
            }
            break;
        case SUB:
            lv = codegen2(root->lhs);
            rv = codegen2(root->rhs);
            is_lc = (root->lhs->kind == CONSTANT || (root->lhs->kind == LPAR && is_constant(root->lhs))) ? 1 : 0;
            is_rc = (root->rhs->kind == CONSTANT || (root->rhs->kind == LPAR && is_constant(root->rhs))) ? 1 : 0;
            if ((is_lc == 0) & (is_rc == 0)) {
                printf("sub r%d r%d r%d\n", lv, lv, rv);
                if (lv != rv)
                    reg[rv] = 0;
                return lv;
            } else if ((is_lc == 0) & (is_rc == 1)) {
                printf("sub r%d r%d %d\n", lv, lv, rv);
                return lv;
            } else if ((is_lc == 1) & (is_rc == 0)) {
                printf("sub r%d %d r%d\n", rv, lv, rv);
                return rv;
            } else {
                for (int i = 0; i < NUM_REGISTERS; i++) {
                    if (reg[i] == 0) {
                        reg[i] = 1;
                        r = i;
                        break;
                    }
                }
                printf("sub r%d %d %d\n", r, lv, rv);
                return r;
            }
            break;
        case MUL:
            lv = codegen2(root->lhs);
            rv = codegen2(root->rhs);
            is_lc = (root->lhs->kind == CONSTANT || (root->lhs->kind == LPAR && is_constant(root->lhs))) ? 1 : 0;
            is_rc = (root->rhs->kind == CONSTANT || (root->rhs->kind == LPAR && is_constant(root->rhs))) ? 1 : 0;
            if ((is_lc == 0) & (is_rc == 0)) {
                printf("mul r%d r%d r%d\n", lv, lv, rv);
                reg[rv] = 0;
                return lv;
            } else if ((is_lc == 0) & (is_rc == 1)) {
                printf("mul r%d r%d %d\n", lv, lv, rv);
                return lv;
            } else if ((is_lc == 1) & (is_rc == 0)) {
                printf("mul r%d %d r%d\n", rv, lv, rv);
                return rv;
            } else {
                for (int i = 0; i < NUM_REGISTERS; i++) {
                    if (reg[i] == 0) {
                        reg[i] = 1;
                        r = i;
                        break;
                    }
                }
                printf("mul r%d %d %d\n", r, lv, rv);
                return r;
            }
            break;
        case DIV:
            lv = codegen2(root->lhs);
            rv = codegen2(root->rhs);
            is_lc = (root->lhs->kind == CONSTANT || (root->lhs->kind == LPAR && is_constant(root->lhs))) ? 1 : 0;
            is_rc = (root->rhs->kind == CONSTANT || (root->rhs->kind == LPAR && is_constant(root->rhs))) ? 1 : 0;
            if ((is_lc == 0) & (is_rc == 0)) {
                if (lv != rv) {  // 嘿嘿嘿神秘撇布
                    printf("div r%d r%d r%d\n", lv, lv, rv);
                    reg[rv] = 0;
                } else
                    printf("add r%d 0 1\n", lv);
                return lv;
            } else if ((is_lc == 0) & (is_rc == 1)) {
                printf("div r%d r%d %d\n", lv, lv, rv);
                return lv;
            } else if ((is_lc == 1) & (is_rc == 0)) {
                printf("div r%d %d r%d\n", rv, lv, rv);
                return rv;
            } else {
                for (int i = 0; i < NUM_REGISTERS; i++) {
                    if (reg[i] == 0) {
                        reg[i] = 1;
                        r = i;
                        break;
                    }
                }
                printf("div r%d %d %d\n", r, lv, rv);
                return r;
            }
            break;
        case REM:
            lv = codegen2(root->lhs);
            rv = codegen2(root->rhs);
            is_lc = (root->lhs->kind == CONSTANT || (root->lhs->kind == LPAR && is_constant(root->lhs))) ? 1 : 0;
            is_rc = (root->rhs->kind == CONSTANT || (root->rhs->kind == LPAR && is_constant(root->rhs))) ? 1 : 0;
            if ((is_lc == 0) & (is_rc == 0)) {
                if (lv != rv) {  // 嘿嘿嘿神秘撇布
                    printf("rem r%d r%d r%d\n", lv, lv, rv);
                    reg[rv] = 0;
                } else
                    printf("add r%d 0 0\n", lv);
                return lv;
            } else if ((is_lc == 0) & (is_rc == 1)) {
                printf("rem r%d r%d %d\n", lv, lv, rv);
                return lv;
            } else if ((is_lc == 1) & (is_rc == 0)) {
                printf("rem r%d %d r%d\n", rv, lv, rv);
                return rv;
            } else {
                for (int i = 0; i < NUM_REGISTERS; i++) {
                    if (reg[i] == 0) {
                        reg[i] = 1;
                        r = i;
                        break;
                    }
                }
                printf("rem r%d %d %d\n", r, lv, rv);
                return r;
            }
            break;
        case PREINC:
            rv = codegen2(root->mid);
            vr = have_identifier(root->mid);
            if (vr == 0)
                vr = root->mid->val;
            printf("add r%d r%d 1\n", rv, rv);
            printf("store [%d] r%d\n", get_register_for_variable(vr), rv);
            return rv;
            break;
        case PREDEC:
            rv = codegen2(root->mid);
            vr = have_identifier(root->mid);
            if (vr == 0)
                vr = root->mid->val;
            printf("sub r%d r%d 1\n", rv, rv);
            printf("store [%d] r%d\n", get_register_for_variable(vr), rv);
            return rv;
            break;
        case POSTINC:
            lv = codegen2(root->mid);
            vr = have_identifier(root->mid);
            if (vr == 0)
                vr = root->mid->val;
            for (int i = 0; i < NUM_REGISTERS; i++) {
                if (reg[i] == 0) {
                    reg[i] = 1;
                    r = i;
                    break;
                }
            }
            printf("add r%d r%d 1\n", r, lv);
            printf("store [%d] r%d\n", get_register_for_variable(vr), r);
            reg[r] = 0;
            return lv;
            break;
        case POSTDEC:
            lv = codegen2(root->mid);
            vr = have_identifier(root->mid);
            if (vr == 0)
                vr = root->mid->val;
            for (int i = 0; i < NUM_REGISTERS; i++) {
                if (reg[i] == 0) {
                    reg[i] = 1;
                    r = i;
                    break;
                }
            }
            printf("sub r%d r%d 1\n", r, lv);
            printf("store [%d] r%d\n", get_register_for_variable(vr), r);
            reg[r] = 0;
            return lv;
            break;
        case IDENTIFIER:
            for (int i = 0; i < NUM_REGISTERS; i++) {
                if (reg[i] == 0) {
                    reg[i] = 1;
                    r = i;
                    break;
                }
            }
            printf("load r%d [%d]\n", r, get_register_for_variable(root->val));
            return r;
            break;
        case CONSTANT:
            return root->val;
            break;
        case PLUS:
            lv = codegen2(root->mid);
            return lv;
            break;
        case MINUS:
            lv = codegen2(root->mid);

            is_lc = 0;
            if (root->mid == NULL) {
                err("Unexpected NULL in unary expression.");
            }
            is_lc = (root->mid->kind == CONSTANT || (root->mid->kind == LPAR && is_constant(root->mid))) ? 1 : 0;

            if ((is_lc == 1)) {
                for (int i = 0; i < NUM_REGISTERS; i++) {
                    if (reg[i] == 0) {
                        reg[i] = 1;
                        r = i;
                        break;
                    }
                }
                printf("sub r%d 0 %d\n", r, lv);
                return r;
            } else {
                printf("sub r%d 0 r%d\n", lv, lv);
                return lv;
            }
            break;
        case LPAR:
            return codegen2(root->mid);
            break;
        case RPAR:
            return codegen2(root->mid);
            break;
        default:
            break;
    }
    return 0;
}

void freeAST(AST* now) {
    if (now == NULL)
        return;
    freeAST(now->lhs);
    freeAST(now->mid);
    freeAST(now->rhs);
    free(now);
}

void token_print(Token* in, size_t len) {
    const static char KindName[][20] = {"Assign", "Add", "Sub", "Mul", "Div", "Rem",
                                        "Inc", "Dec", "Inc", "Dec", "Identifier", "Constant",
                                        "LPar", "RPar", "Plus", "Minus", "End"};
    const static char KindSymbol[][20] = {"'='", "'+'", "'-'", "'*'", "'/'", "'%'", "\"++\"", "\"--\"",
                                          "\"++\"", "\"--\"", "", "", "'('", "')'", "'+'", "'-'"};
    const static char format_str[] = "<Index = %3d>: %-10s, %-6s = %s\n";
    const static char format_int[] = "<Index = %3d>: %-10s, %-6s = %d\n";
    for (int i = 0; i < len; i++) {
        switch (in[i].kind) {
            case LPAR:
            case RPAR:
            case PREINC:
            case PREDEC:
            case ADD:
            case SUB:
            case MUL:
            case DIV:
            case REM:
            case ASSIGN:
            case PLUS:
            case MINUS:
                fprintf(stderr, format_str, i, KindName[in[i].kind], "symbol", KindSymbol[in[i].kind]);
                break;
            case CONSTANT:
                fprintf(stderr, format_int, i, KindName[in[i].kind], "value", in[i].val);
                break;
            case IDENTIFIER:
                fprintf(stderr, format_str, i, KindName[in[i].kind], "name", (char*)(&(in[i].val)));
                break;
            case END:
                fprintf(stderr, "<Index = %3d>: %-10s\n", i, KindName[in[i].kind]);
                break;
            default:
                fputs("=== unknown token ===", stderr);
        }
    }
}

void AST_print(AST* head) {
    static char indent_str[MAX_LENGTH] = "  ";
    static int indent = 2;
    const static char KindName[][20] = {"Assign", "Add", "Sub", "Mul", "Div", "Rem",
                                        "PreInc", "PreDec", "PostInc", "PostDec", "Identifier", "Constant",
                                        "Parentheses", "Parentheses", "Plus", "Minus"};
    const static char format[] = "%s\n";
    const static char format_str[] = "%s, <%s = %s>\n";
    const static char format_val[] = "%s, <%s = %d>\n";
    if (head == NULL)
        return;
    char* indent_now = indent_str + indent;
    indent_str[indent - 1] = '-';
    fprintf(stderr, "%s", indent_str);
    indent_str[indent - 1] = ' ';
    if (indent_str[indent - 2] == '`')
        indent_str[indent - 2] = ' ';
    switch (head->kind) {
        case ASSIGN:
        case ADD:
        case SUB:
        case MUL:
        case DIV:
        case REM:
        case PREINC:
        case PREDEC:
        case POSTINC:
        case POSTDEC:
        case LPAR:
        case RPAR:
        case PLUS:
        case MINUS:
            fprintf(stderr, format, KindName[head->kind]);
            break;
        case IDENTIFIER:
            fprintf(stderr, format_str, KindName[head->kind], "name", (char*)&(head->val));
            break;
        case CONSTANT:
            fprintf(stderr, format_val, KindName[head->kind], "value", head->val);
            break;
        default:
            fputs("=== unknown AST type ===", stderr);
    }
    indent += 2;
    strcpy(indent_now, "| ");
    AST_print(head->lhs);
    strcpy(indent_now, "` ");
    AST_print(head->mid);
    AST_print(head->rhs);
    indent -= 2;
    (*indent_now) = '\0';
}