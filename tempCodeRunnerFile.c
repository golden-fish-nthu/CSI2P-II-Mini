void semantic_check(AST *now) {
    if (now == NULL)
        return;

    // 左操作數必須是標識符或帶有一個或多個括號的標識符。
    if (now->kind == ASSIGN) {
        AST *tmp = now->lhs;
        while (tmp->kind == LPAR)
            tmp = tmp->mid;
        if (tmp->kind != IDENTIFIER)
            err("Lvalue is required as left operand of assignment.");
    }

    // INC/DEC 的操作數必須是標識符或帶有一個或多個括號的標識符。
    if (now->kind == PREINC || now->kind == PREDEC || now->kind == POSTINC || now->kind == POSTDEC) {
        AST *tmp = now->mid;
        while (tmp->kind == LPAR)
            tmp = tmp->mid;
        if (tmp->kind != IDENTIFIER)
            err("Operand of increment/decrement must be an identifier.");
    }

    // 遞迴檢查每個節點的語義。
    semantic_check(now->lhs);
    semantic_check(now->mid);
    semantic_check(now->rhs);

    // ADD/SUB/MUL/DIV/REM 的操作數必須是常量或標識符或帶有一個或多個括號的常量或標識符。
    if (now->kind == ADD || now->kind == SUB || now->kind == MUL || now->kind == DIV || now->kind == REM) {
        AST *tmp = now->lhs;
        while (tmp->kind == LPAR)
            tmp = tmp->mid;
        if (tmp->kind != IDENTIFIER && tmp->kind != CONSTANT)
            err("Operands of binary operators must be identifiers or constants.");

        tmp = now->rhs;
        while (tmp->kind == LPAR)
            tmp = tmp->mid;
        if (tmp->kind != IDENTIFIER && tmp->kind != CONSTANT)
            err("Operands of binary operators must be identifiers or constants.");
    }
}