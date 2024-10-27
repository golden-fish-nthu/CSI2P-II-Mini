#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int reg[256] = {0}; // 0 = not used, 1 = used
#define MAX_LENGTH 200
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
    int val; // 記錄整數值或變量名稱
    struct TokenUnit *next;
} Token;
typedef struct ASTUnit {
    Kind kind;
    int val; // 記錄整數值或變量名稱
    struct ASTUnit *lhs, *mid, *rhs;
} AST;
typedef struct {
    Kind kinds;
    int val;
} NodeInfo;
/// 工具接口
#define err(x)                                                                                                         \
    {                                                                                                                  \
        puts("Compile Error!");                                                                                        \
        if (DEBUG) {                                                                                                   \
            fprintf(stderr, "Error at line: %d\n", __LINE__);                                                          \
            fprintf(stderr, "Error message: %s\n", x);                                                                 \
        }                                                                                                              \
        exit(0);                                                                                                       \
    }
// 你可以設置DEBUG=1來調試。記得在提交前設置回0。
#define DEBUG 1
// 將輸入的字符數組拆分為標記鏈表。
Token *lexer(const char *in);
// 創建一個新的標記。
Token *new_token(Kind kind, int val);
// 將標記鏈表轉換為數組，返回其長度。
size_t token_list_to_arr(Token **head);
// 解析標記數組。返回構建的AST。
AST *parser(Token *arr, size_t len);
// 解析標記數組。返回構建的AST。
AST *parse(Token *arr, int l, int r, GrammarState S);
// 創建一個新的AST節點。
AST *new_AST(Kind kind, int val);
// 查找符合條件(cond)的下一個標記的位置。如果未找到則返回-1。從start到end方向搜索。
int findNextSection(Token *arr, int start, int end, int (*cond)(Kind));
// 如果kind是ASSIGN則返回1。
int condASSIGN(Kind kind);
// 如果kind是ADD或SUB則返回1。
int condADD(Kind kind);
// 如果kind是MUL、DIV或REM則返回1。
int condMUL(Kind kind);
// 如果kind是RPAR則返回1。
int condRPAR(Kind kind);
// 檢查AST是否語義正確。如果檢查失敗，該函數將自動調用err()。
void semantic_check(AST *now);
// 生成ASM代碼。
int codegen(AST *root);
// 釋放整個AST。
void freeAST(AST *now);
/// 調試接口
// 打印標記數組。
void token_print(Token *in, size_t len);
// 打印AST樹。
void AST_print(AST *head);
char input[MAX_LENGTH];
int main() {
    while (fgets(input, MAX_LENGTH, stdin) != NULL) {
        Token *content = lexer(input);
        size_t len = token_list_to_arr(&content);
        if (len == 0)
            continue;
        AST *ast_root = parser(content, len);
        token_print(content, len);
        AST_print(ast_root);
        semantic_check(ast_root);
        codegen(ast_root);
        free(content);
        freeAST(ast_root);
    }
    return 0;
}
Token *lexer(const char *in) {
    Token *head = NULL;
    Token **now = &head;
    for (int i = 0; in[i]; i++) {
        if (isspace(in[i])) // 忽略空白字符
            continue;
        else if (isdigit(in[i])) {
            (*now) = new_token(CONSTANT, atoi(in + i));
            while (in[i + 1] && isdigit(in[i + 1]))
                i++;
        } else if ('x' <= in[i] && in[i] <= 'z') // 變量
            (*now) = new_token(IDENTIFIER, in[i]);
        else
            switch (in[i]) {
            case '=':
                (*now) = new_token(ASSIGN, 0);
                break;
            case '+':
                if (in[i + 1] && in[i + 1] == '+') {
                    i++; // 在lexer範圍內，所有"++"都將被標記為PREINC。
                    (*now) = new_token(PREINC, 0);
                } // 在lexer範圍內，所有單個"+"都將被標記為PLUS。
                else
                    (*now) = new_token(PLUS, 0);
                break;
            case '-':
                if (in[i + 1] && in[i + 1] == '-') {
                    i++; // 在lexer範圍內，所有"--"都將被標記為PREDEC。

                    (*now) = new_token(PREDEC, 0);
                } // 在lexer範圍內，所有單個"-"都將被標記為MINUS。
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

Token *new_token(Kind kind, int val) {
    Token *res = (Token *)malloc(sizeof(Token));
    res->kind = kind;
    res->val = val;
    res->next = NULL;
    return res;
}

size_t token_list_to_arr(Token **head) {
    size_t res;
    Token *now = (*head), *del;
    for (res = 0; now != NULL; res++)
        now = now->next;
    now = (*head);
    if (res != 0)
        (*head) = (Token *)malloc(sizeof(Token) * res);
    for (int i = 0; i < res; i++) {
        (*head)[i] = (*now);
        del = now;
        now = now->next;
        free(del);
    }
    return res;
}

AST *parser(Token *arr, size_t len) {
    for (int i = 1; i < len; i++) { // 正確識別"ADD"和"SUB"
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

AST *parse(Token *arr, int l, int r, GrammarState S) {
    AST *now = NULL;
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
    case ADD_EXPR: // 加法符
        if ((nxt = findNextSection(arr, r, l, condADD)) != -1) {
            now = new_AST(arr[nxt].kind, 0);
            now->lhs = parse(arr, l, nxt - 1, ADD_EXPR);
            now->rhs = parse(arr, nxt + 1, r, MUL_EXPR);
            return now;
        }
        return parse(arr, l, r, MUL_EXPR);
    case MUL_EXPR: // 乘法符 TODO
        if ((nxt = findNextSection(arr, r, l, condMUL)) != -1) {
            now = new_AST(arr[nxt].kind, 0);
            now->lhs = parse(arr, l, nxt - 1, MUL_EXPR);
            now->rhs = parse(arr, nxt + 1, r, UNARY_EXPR);
            return now;
        }
        return parse(arr, l, r, UNARY_EXPR);
    case UNARY_EXPR:
        if (l > r)
            err("Unexpected end of expression.");
        if (arr[l].kind == SUB || arr[l].kind == MINUS || arr[l].kind == PREINC ||
            arr[l].kind == PREDEC) { // 一元運算符
            now = new_AST(arr[l].kind, 0);
            now->mid = parse(arr, l + 1, r, UNARY_EXPR);
            return now;
        }
        return parse(arr, l, r, POSTFIX_EXPR);
    case POSTFIX_EXPR:
        if (arr[r].kind == PREINC || arr[r].kind == PREDEC) { // 將"PREINC"、"PREDEC"轉換為"POSTINC"、"POSTDEC"
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
        // printf("Error at line: %d, l: %d, r: %d\n", __LINE__, l, r);
        err("No token left for parsing.");
    default:
        err("Unexpected grammar state.");
    }
}
AST *new_AST(Kind kind, int val) {
    AST *res = (AST *)malloc(sizeof(AST));
    res->kind = kind;
    res->val = val;
    res->lhs = res->mid = res->rhs = NULL;
    return res;
}

int findNextSection(Token *arr, int start, int end, int (*cond)(Kind)) {
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
void semantic_check(AST *now) {
    if (now == NULL)
        return;
    if (now->kind == ASSIGN) {
        AST *tmp = now->lhs;
        while (tmp->kind == LPAR)
            tmp = tmp->mid;
        if (tmp->kind != IDENTIFIER)
            err("Lvalue is required as left operand of assignment.");
    }
    if (now->kind == PREINC || now->kind == PREDEC || now->kind == POSTINC || now->kind == POSTDEC) {
        AST *tmp = now->mid;
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
        return 0; // x 存儲在 memory[0]
    case 'y':
        return 4; // y 存儲在 memory[4]
    case 'z':
        return 8; // z 存儲在 memory[8]
    default:
        return -1; // 錯誤
    }
}
NodeInfo get_node_info(AST *root) {
    NodeInfo info = {root->kind, root->val}; // 初始化結果

    while (root->kind == LPAR) {
        if (root->mid->kind != LPAR && root->mid->kind != CONSTANT && root->mid->kind != IDENTIFIER) {
            info.kinds = root->mid->kind;
            info.val = root->mid->val;
            return info; // 是其他類型
        }
        if (root->mid->kind == LPAR) {
            root = root->mid; // 繼續查找
        } else {
            info.kinds = root->mid->kind;
            info.val = root->mid->val;
            return info; // 有常數或標識符
        }
    }
    return info;
}

int codegen(AST *root) {
    if (root == NULL)
        return -1;
    int left, right, r; // 寄存器變量
    NodeInfo node_info;
    int vr;
    switch (root->kind) {
    case ASSIGN:
        node_info = get_node_info(root->lhs);
        if (node_info.kinds == IDENTIFIER)
            vr = node_info.val;
        else
            vr = root->lhs->val;
        right = codegen(root->rhs);
        node_info = get_node_info(root->rhs);
        if (node_info.kinds == CONSTANT) {
            for (int i = 0; i < 256; i++) {
                if (reg[i] == 0) {
                    reg[i] = 1;
                    r = i;
                    break;
                }
            }
            printf("add r%d %d %d\n", r, 0, right);
            printf("store [%d] r%d\n", get_register_for_variable((char)vr), r);
            reg[right] = 0;
            return r;
        } else {
            printf("store [%d] r%d\n", get_register_for_variable((char)vr), right);
        }
        break;
    case ADD:
        left = codegen(root->lhs);
        right = codegen(root->rhs);
        node_info = get_node_info(root->lhs);
        int is_lc = (node_info.kinds == CONSTANT);
        node_info = get_node_info(root->rhs);
        int is_rc = (node_info.kinds == CONSTANT);
        if (!is_lc && !is_rc) {
            printf("add r%d r%d r%d\n", left, left, right);
            reg[right] = 0;
            return left;
        } else if (!is_lc && is_rc) {
            printf("add r%d r%d %d\n", left, left, right);
            return left;
        } else if (is_lc && !is_rc) {
            printf("add r%d %d r%d\n", right, left, right);
            return right;
        } else {
            for (int i = 0; i < 256; i++) {
                if (reg[i] == 0) {
                    reg[i] = 1;
                    r = i;
                    break;
                }
            }
            printf("add r%d %d %d\n", r, left, right);
            return r;
        }
        break;
    case SUB:
        left = codegen(root->lhs);
        right = codegen(root->rhs);
        node_info = get_node_info(root->lhs);
        is_lc = (node_info.kinds == CONSTANT);
        node_info = get_node_info(root->rhs);
        is_rc = (node_info.kinds == CONSTANT);
        if (!is_lc && !is_rc) {
            printf("sub r%d r%d r%d\n", left, left, right);
            reg[right] = 0;
            return left;
        } else if (!is_lc && is_rc) {
            printf("sub r%d r%d %d\n", left, left, right);
            return left;
        } else if (is_lc && !is_rc) {
            printf("sub r%d %d r%d\n", right, left, right);
            return right;
        } else {
            for (int i = 0; i < 256; i++) {
                if (reg[i] == 0) {
                    reg[i] = 1;
                    r = i;
                    break;
                }
            }
            printf("sub r%d %d %d\n", r, left, right);
            return r;
        }
        break;
    case MUL:
        left = codegen(root->lhs);
        right = codegen(root->rhs);
        node_info = get_node_info(root->lhs);
        is_lc = (node_info.kinds == CONSTANT);
        node_info = get_node_info(root->rhs);
        is_rc = (node_info.kinds == CONSTANT);
        if (!is_lc && !is_rc) {
            printf("mul r%d r%d r%d\n", left, left, right);
            reg[right] = 0;
            return left;
        } else if (!is_lc && is_rc) {
            printf("mul r%d r%d %d\n", left, left, right);
            return left;
        } else if (is_lc && !is_rc) {
            printf("mul r%d %d r%d\n", right, left, right);
            return right;
        } else {
            for (int i = 0; i < 256; i++) {
                if (reg[i] == 0) {
                    reg[i] = 1;
                    r = i;
                    break;
                }
            }
            printf("mul r%d %d %d\n", r, left, right);
            return r;
        }
        break;
    case DIV:
        left = codegen(root->lhs);
        right = codegen(root->rhs);
        node_info = get_node_info(root->lhs);
        is_lc = (node_info.kinds == CONSTANT);
        node_info = get_node_info(root->rhs);
        is_rc = (node_info.kinds == CONSTANT);
        if (!is_lc && !is_rc) {
            printf("div r%d r%d r%d\n", left, left, right);
            reg[right] = 0;
            return left;
        } else if (!is_lc && is_rc) {
            printf("div r%d r%d %d\n", left, left, right);
            return left;
        } else if (is_lc && !is_rc) {
            printf("div r%d %d r%d\n", right, left, right);
            return right;
        } else {
            for (int i = 0; i < 256; i++) {
                if (reg[i] == 0) {
                    reg[i] = 1;
                    r = i;
                    break;
                }
            }
            printf("div r%d %d %d\n", r, left, right);
            return r;
        }
        break;
    case REM:
        left = codegen(root->lhs);
        right = codegen(root->rhs);
        node_info = get_node_info(root->lhs);
        is_lc = (node_info.kinds == CONSTANT);
        node_info = get_node_info(root->rhs);
        is_rc = (node_info.kinds == CONSTANT);
        if (!is_lc && !is_rc) {
            printf("rem r%d r%d r%d\n", left, left, right);
            reg[right] = 0;
            return left;
        } else if (!is_lc && is_rc) {
            printf("rem r%d r%d %d\n", left, left, right);
            return left;
        } else if (is_lc && !is_rc) {
            printf("rem r%d %d r%d\n", right, left, right);
            return right;
        } else {
            for (int i = 0; i < 256; i++) {
                if (reg[i] == 0) {
                    reg[i] = 1;
                    r = i;
                    break;
                }
            }
            printf("rem r%d %d %d\n", r, left, right);
            return r;
        }
        break;
    case PREINC:
        right = codegen(root->mid);
        node_info = get_node_info(root->mid);
        if (node_info.kinds == IDENTIFIER)
            vr = node_info.val;
        else
            vr = root->mid->val;
        // printf("Kind: %d, Val: %d\n", node_info.kinds, node_info.val);
        // printf("right: %d\n", right);
        printf("add r%d r%d 1\n", right, right);
        printf("store [%d] r%d\n", get_register_for_variable((char)vr), right);
        return right;
        break;
    case PREDEC:
        right = codegen(root->mid);
        node_info = get_node_info(root->mid);
        if (node_info.kinds == IDENTIFIER)
            vr = node_info.val;
        else
            vr = root->mid->val;
        printf("sub r%d r%d 1\n", right, right);
        printf("store [%d] r%d\n", get_register_for_variable((char)vr), right);
        return right;
        break;
    case POSTINC:
        right = codegen(root->mid);
        node_info = get_node_info(root->mid);
        if (node_info.kinds == IDENTIFIER)
            vr = node_info.val;
        else
            vr = root->mid->val;
        for (int i = 0; i < 256; i++) {
            if (reg[i] == 0) {
                reg[i] = 1;
                r = i;
                break;
            }
        }
        printf("add r%d r%d 1\n", r, r);
        printf("store [%d] r%d\n", get_register_for_variable((char)vr), r);
        reg[r] = 0;
        return left;
        break;
    case POSTDEC:
        right = codegen(root->mid);
        node_info = get_node_info(root->mid);
        if (node_info.kinds == IDENTIFIER)
            vr = node_info.val;
        else
            vr = root->mid->val;
        for (int i = 0; i < 256; i++) {
            if (reg[i] == 0) {
                reg[i] = 1;
                r = i;
                break;
            }
        }
        printf("sub r%d r%d 1\n", r, r);
        printf("store [%d] r%d\n", get_register_for_variable((char)vr), r);
        reg[r] = 0;
        return left;
        break;
    case IDENTIFIER:
        for (int i = 0; i < 256; i++) {
            if (reg[i] == 0) {
                reg[i] = 1;
                r = i;
                break;
            }
        }
        printf("load r%d [%d]\n", r, get_register_for_variable((char)root->val));
        return r;
        break;
    case CONSTANT:
        return root->val;
        break;
    case PLUS:
        left = codegen(root->mid);
        return left;
        break;
    case MINUS:
        left = codegen(root->mid);
        node_info = get_node_info(root->mid);
        is_lc = (node_info.kinds == CONSTANT);
        if (is_lc) {
            for (int i = 0; i < 256; i++) {
                if (reg[i] == 0) {
                    reg[i] = 1;
                    r = i;
                    break;
                }
            }
            printf("sub r%d 0 %d\n", r, left);
            return r;
        } else {
            printf("sub r%d 0 r%d\n", left, left);
            return left;
        }
        break;
    case LPAR:
        return codegen(root->mid);
        break;
    case RPAR:
        return codegen(root->mid);
        break;
    default:
        break;
    }
    return 0;
}

void freeAST(AST *now) {
    if (now == NULL)
        return;
    freeAST(now->lhs);
    freeAST(now->mid);
    freeAST(now->rhs);
    free(now);
}

void token_print(Token *in, size_t len) {
    const static char KindName[][20] = {"Assign", "Add",  "Sub",  "Mul",   "Div",        "Rem",
                                        "Inc",    "Dec",  "Inc",  "Dec",   "Identifier", "Constant",
                                        "LPar",   "RPar", "Plus", "Minus", "End"};
    const static char KindSymbol[][20] = {"'='",    "'+'",    "'-'", "'*'", "'/'", "'%'", "\"++\"", "\"--\"",
                                          "\"++\"", "\"--\"", "",    "",    "'('", "')'", "'+'",    "'-'"};
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
            fprintf(stderr, format_str, i, KindName[in[i].kind], "name", (char *)(&(in[i].val)));
            break;
        case END:
            fprintf(stderr, "<Index = %3d>: %-10s\n", i, KindName[in[i].kind]);
            break;
        default:
            fputs("=== unknown token ===", stderr);
        }
    }
}

void AST_print(AST *head) {
    static char indent_str[MAX_LENGTH] = "  ";
    static int indent = 2;
    const static char KindName[][20] = {"Assign",      "Add",         "Sub",     "Mul",     "Div",        "Rem",
                                        "PreInc",      "PreDec",      "PostInc", "PostDec", "Identifier", "Constant",
                                        "Parentheses", "Parentheses", "Plus",    "Minus"};
    const static char format[] = "%s\n";
    const static char format_str[] = "%s, <%s = %s>\n";
    const static char format_val[] = "%s, <%s = %d>\n";
    if (head == NULL)
        return;
    char *indent_now = indent_str + indent;
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
        fprintf(stderr, format_str, KindName[head->kind], "name", (char *)&(head->val));
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
