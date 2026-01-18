#include "StmtNode.h"
#include "core/helpers/Console.h"
#include "json.hpp"
#include "core/bytecode/ExprBuilder.h"
#include "core/bytecode/StmtBuilder.h"
#include "jvm/attribute-code.h"
#include "jvm/descriptor-method.h"
#include "jvm/method.h"

using json = nlohmann::json;

string StmtNode::_getClassName() const {
    return "StmtNode";
}

string StmtNode::toJson() const {
    json j;
    j["id"] = GetId();
    j["type"] = toString(type);

    if (expr != nullptr) {
        j["expr"] = json::parse(expr->toJson());
    }

    if (catchStmt != nullptr) {
        j["catch"] = json::parse(catchStmt->toJson());
    }
    if (catchId != "") {
        j["catchId"] = catchId;
    }
    if (catchType != nullptr) {
        j["catchType"] = json::parse(catchType->toJson());
    }
    if (finallyStmt != nullptr) {
        j["finally"] = json::parse(finallyStmt->toJson());
    }

    if (loopInitializer != nullptr) {
        j["init"] = json::parse(loopInitializer->toJson());
    }
    if (condition != nullptr) {
        j["cond"] = json::parse(condition->toJson());
    }
    if (loopEndAction != nullptr) {
        j["action"] = json::parse(loopEndAction->toJson());
    }

    if (foreachCollection != nullptr) {
        j["collection"] = json::parse(foreachCollection->toJson());
    }
    if (foreachKey != nullptr) {
        j["key"] = json::parse(foreachKey->toJson());
    }
    if (foreachValue != nullptr) {
        j["value"] = json::parse(foreachValue->toJson());
    }

    if (elseIfStmt != nullptr) {
        j["elif"] = json::parse(elseIfStmt->toJson());
    }
    if (elseStmt != nullptr) {
        j["else"] = json::parse(elseStmt->toJson());
    }

    if (!children.empty()) {
        json childrenArray = json::array();
        for (const auto &child: children) {
            childrenArray.push_back(json::parse(child->toJson()));
        }
        j["children"] = childrenArray;
    }

    return j.dump(2);
}

string StmtNode::toDot() const {
    string result;
    string label;

#ifdef NODE_DOT_LABEL_DEBUG
    label += "(S) ";
#endif

    label += toSymbol(type);

#ifdef NODE_DOT_LABEL_DEBUG
    label += "\\n" + toString(type);
    label += "\\nID: " + std::to_string(GetId());
#endif

    std::string::size_type pos = 0;
    while ((pos = label.find('"', pos)) != std::string::npos) {
        label.replace(pos, 1, "\\\"");
        pos += 2;
    }

    result += "  node" + std::to_string(GetId()) + " [label=\"" + label + "\", fillcolor=\"";
    result += type == ST_STMT_LIST
                  ? "#91BCC9"
                  : "#ADD8E6";
    result += "\", style=filled];\n";

    if (expr != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(expr->GetId()) + " [label=expr];\n";
        result += expr->toDot();
    }

    if (condition != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(condition->GetId()) +
                " [label=condition];\n";
        result += condition->toDot();
    }

    if (loopInitializer != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(loopInitializer->GetId()) +
                " [label=loopInitializer];\n";
        result += loopInitializer->toDot();
    }

    if (loopEndAction != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(loopEndAction->GetId()) +
                " [label=loopEndAction];\n";
        result += loopEndAction->toDot();
    }

    if (foreachCollection != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(foreachCollection->GetId()) +
                " [label=foreachCollection];\n";
        result += foreachCollection->toDot();
    }

    if (foreachKey != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(foreachKey->GetId()) +
                " [label=foreachKey];\n";
        result += foreachKey->toDot();
    }

    if (foreachValue != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(foreachValue->GetId()) +
                " [label=foreachValue];\n";
        result += foreachValue->toDot();
    }

    if (elseIfStmt != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(elseIfStmt->GetId()) +
                " [label=elseIfStmt];\n";
        result += elseIfStmt->toDot();
    }

    if (elseStmt != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(elseStmt->GetId()) +
                " [label=elseStmt];\n";
        result += elseStmt->toDot();
    }

    if (catchStmt != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(catchStmt->GetId()) +
                " [label=catchStmt];\n";
        result += catchStmt->toDot();
    }

    if (catchType != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(catchType->GetId()) +
                " [label=\"catchType, catchId: " + catchId + "\"];\n";
        result += catchType->toDot();
    }

    if (finallyStmt != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(finallyStmt->GetId()) +
                " [label=finallyStmt];\n";
        result += finallyStmt->toDot();
    }

    if (stmt != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(stmt->GetId()) + " [label=stmt];\n";
        result += stmt->toDot();
    }

    int i = 0;
    for (const auto &child: children) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(child->GetId()) +
                " [label=child" + std::to_string(i++) + "];\n";
        result += child->toDot();
    }

    return result;
}

bool StmtNode::doSemantics() {
    Log("starting semantics for " + toString(type) + "...");

    bool isOk = true;
    bool isFoundDefaultCase = false;
    StmtNode *initMatched, *doWhileBody = nullptr;
    ExprNode *exprNode = nullptr;

    if (this->stmt != nullptr) {
        isOk = isOk && this->stmt->doSemantics();
    } else {
        Log("(default) skipped stmt");
    }

    if (this->expr != nullptr) {
        isOk = isOk && this->expr->doSemantics();
    } else {
        Log("(default) skipped expr");
    }

    switch (type) {
        case ST_UNKNOWN:
            Warn("unknown type");
            return true;

        case ST_STMT_LIST:
            for (const auto &child: children) {
                isOk = isOk && child->doSemantics();
            }
            break;

        case ST_WHILE:
        case ST_DO_WHILE:
            if (this->condition != nullptr) {
                isOk = isOk && this->condition->doSemantics();
            } else {
                Log("(ST_WHILE/ST_DO_WHILE) skipped condition");
            }
            break;

        case ST_FOR:
            if (this->loopInitializer != nullptr) {
                isOk = isOk && this->loopInitializer->doSemantics();
            } else {
                Log("(ST_FOR) skipped loopInitializer");
            }

            if (this->condition != nullptr) {
                isOk = isOk && this->condition->doSemantics();
            } else {
                Log("(ST_FOR) skipped condition");
            }

            if (this->loopEndAction != nullptr) {
                isOk = isOk && this->loopEndAction->doSemantics();
            } else {
                Log("(ST_FOR) skipped loopEndAction");
            }
            break;

        case ST_FOREACH:
            if (this->foreachCollection != nullptr) {
                isOk = isOk && this->foreachCollection->doSemantics();
            } else {
                Log("(ST_FOREACH) skipped foreachCollection");
            }

            if (this->foreachKey != nullptr) {
                isOk = isOk && this->foreachKey->doSemantics();
            } else {
                Log("(ST_FOREACH) skipped foreachKey");
            }

            if (this->foreachValue != nullptr) {
                isOk = isOk && this->foreachValue->doSemantics();
            } else {
                Log("(ST_FOREACH) skipped foreachValue");
            }

            //!!!TODO checkForeachElement aka if it's an array / itterable (check what could be as an expr here)
            Warn("ST_FOREACH implementation is unfinished");
            break;

        case ST_IF:
            if (this->condition != nullptr) {
                isOk = isOk && this->condition->doSemantics();
            } else {
                Log("(ST_IF) skipped condition");
            }

            if (this->elseIfStmt != nullptr) {
                isOk = isOk && this->elseIfStmt->doSemantics();
            } else {
                Log("(ST_IF) skipped elseIfStmt");
            }

            if (this->elseStmt != nullptr) {
                isOk = isOk && this->elseStmt->doSemantics();
            } else {
                Log("(ST_IF) skipped elseStmt");
            }
            break;

        case ST_SWITCH:
            // Check if there's multiple default cases
            for (const auto &child: children) {
                if (isFoundDefaultCase && child->type == ST_CASE_DEFAULT) {
                    isOk = false;
                    Error("ST_SWITCH has multiple default cases");
                    break;
                }

                if (child->type == ST_CASE_DEFAULT) {
                    isFoundDefaultCase = true;
                }
            }

            // Change to do-while
            this->type = ST_STMT_LIST;
            this->children.clear();
            if (this->stmt == nullptr) {
                this->expr = nullptr;
                Warn("ST_SWITCH is empty");
                break;
            }

            initMatched = StmtNode::ExprStmt(
                ExprNode::Assign(
                    ExprNode::Sigil(ExprNode::Id(new string("___F___"))),
                    ExprNode::Int(0)
                )
            );


            this->children.push_back(initMatched);

            doWhileBody = StmtNode::StmtList(nullptr);

            for (const auto &caseChild: this->stmt->children) {
                switch (caseChild->type) {
                    case ST_CASE_DEFAULT:
                        exprNode = ExprNode::GreatOrEqual(
                            ExprNode::Sigil(ExprNode::Id(new string("___F___"))),
                            ExprNode::Int(1)
                        );
                        break;
                    case ST_CASE:
                        exprNode = ExprNode::OrLower(
                            ExprNode::Equal(this->expr, caseChild->expr),
                            ExprNode::Equal(
                                ExprNode::Sigil(ExprNode::Id(new string("___F___"))),
                                ExprNode::Int(1)
                            )
                        );
                        break;
                    default:
                        isOk = false;
                        Error("invalid case");
                        continue;
                }

                StmtNode::AppendToStmtList(
                    doWhileBody,
                    StmtNode::If(
                        exprNode,
                        StmtNode::AppendToStmtList(
                            StmtNode::StmtList(
                                StmtNode::ExprStmt(
                                    ExprNode::Assign(
                                        ExprNode::Sigil(ExprNode::Id(new string("___F___"))),
                                        ExprNode::Int(1)
                                    )
                                )
                            ),
                            caseChild->stmt
                        )
                    )
                );
            }

            StmtNode::AppendToStmtList(
                doWhileBody,
                StmtNode::If(
                    ExprNode::Equal(
                        ExprNode::Sigil(ExprNode::Id(new string("___F___"))),
                        ExprNode::Int(0)
                    ),
                    StmtNode::ExprStmt(
                        ExprNode::Assign(
                            ExprNode::Sigil(ExprNode::Id(new string("___F___"))),
                            ExprNode::Int(2)
                        )
                    )
                )
            );

            this->children.push_back(
                StmtNode::DoWhile(
                    ExprNode::NotEqual(
                        ExprNode::Sigil(ExprNode::Id(new string("___F___"))),
                        ExprNode::Int(2)),
                    doWhileBody
                )
            );

            this->expr = nullptr;
            this->stmt = nullptr;

            Warn("ST_SWITCH implementation is unfinished");
            break;

        case ST_TRY:
            if (this->catchStmt != nullptr) {
                isOk = isOk && this->catchStmt->doSemantics();
            } else {
                Log("(ST_TRY) skipped catchStmt");
            }

            if (this->finallyStmt != nullptr) {
                isOk = isOk && this->finallyStmt->doSemantics();
            } else {
                Log("(ST_TRY) skipped finallyStmt");
            }
            break;

        case ST_CASE:
        case ST_CASE_DEFAULT:
        case ST_ECHO:
        case ST_RETURN:
        case ST_BREAK:
        case ST_CONTINUE:
        case ST_EXPRESSION:
        case ST_ELSE_IF:
        case ST_ELSE:
            break;

        default:
            Error("unknown enum type");
            return false;
    }

    if (isOk) {
        Log("finished semantics for " + toString(type) + "");
    } else {
        Error("semantics for " + toString(type) + " failed");
    }

    return isOk;
}

Class *StmtNode::processClass(Class *root, std::vector<Class *> &list) {
    Log("starting bytecode generation for " + toString(type) + "...");

    bool isOk = true;

    try {
        Method *mainMethod = root->getOrCreateMethod(
            "main",
            DescriptorMethod(
                std::nullopt, // void
                {DescriptorField("java/lang/String", 1)} // String[] args
            )
        );

        mainMethod->addFlag(Method::ACC_PUBLIC);
        mainMethod->addFlag(Method::ACC_STATIC);

        AttributeCode *code = mainMethod->getCodeAttribute();

        addStmt(root, mainMethod, code, true);
    } catch (const std::exception &e) {
        isOk = false;
        Error(std::string("StmtNode::processClass failed: ") + e.what());
    }

    if (isOk) {
        Log("finished semantics for " + toString(type));
    } else {
        Error("semantics for " + toString(type) + " failed");
    }

    return root;
}

AttributeCode *StmtNode::addStmt(Class *root, Method *method, AttributeCode *code, bool isMain) const {
    if (root == nullptr) throw std::logic_error("StmtNode::addStmt: root is null");
    if (method == nullptr) throw std::logic_error("StmtNode::addStmt: method is null");
    if (code == nullptr) throw std::logic_error("StmtNode::addStmt: code is null");

    // ------------------------------------------------------------
    // Helpers (local to this function)
    // ------------------------------------------------------------
    auto *nullValueField = root->getOrCreateFieldrefConstant(
        "com/phpjvm/BasePhpValue",
        "NULL_VALUE",
        DescriptorField("com/phpjvm/BasePhpValue")
    );

    auto *toBool = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "toBool",
        DescriptorMethod(DescriptorField("Z"), {})
    );

    // Emit: (cond as BasePhpValue) -> toBool() -> if == 0 jump L_false
    auto emitJumpIfFalse = [&](ExprNode *cond, jvm::Label *L_false) {
        if (cond == nullptr) {
            // null condition => treat as "true" (no jump)
            return;
        }
        ExprBuilder::EmitValue(root, method, code, cond); // BasePhpValue
        *code << code->InvokeVirtual(toBool); // int (0/1)
        *code << code->If(Instruction::Compare::Equal, L_false); // if 0 -> jump
    };

    auto emitEvalAndPop = [&](ExprNode *e) {
        if (!e) return;
        ExprBuilder::EmitValue(root, method, code, e);
        *code << code->PopOne();
    };

    // ------------------------------------------------------------
    // Loop context stacks for break/continue
    // ------------------------------------------------------------
    // NOTE: kept here (static) so you don't need to change signatures.
    // Each loop pushes (breakLabel, continueLabel), and ST_BREAK/ST_CONTINUE jump to the top.
    static thread_local std::vector<jvm::Label *> s_breakStack;
    static thread_local std::vector<jvm::Label *> s_continueStack;

    auto pushLoop = [&](jvm::Label *L_break, jvm::Label *L_continue) {
        s_breakStack.push_back(L_break);
        s_continueStack.push_back(L_continue);
    };
    auto popLoop = [&]() {
        if (!s_breakStack.empty()) s_breakStack.pop_back();
        if (!s_continueStack.empty()) s_continueStack.pop_back();
    };
    auto currentBreak = [&]() -> jvm::Label * {
        return s_breakStack.empty() ? nullptr : s_breakStack.back();
    };
    auto currentContinue = [&]() -> jvm::Label * {
        return s_continueStack.empty() ? nullptr : s_continueStack.back();
    };

    // ------------------------------------------------------------
    // Dispatch
    // ------------------------------------------------------------
    switch (type) {
        case ST_STMT_LIST: {
            for (StmtNode *child: children) {
                if (child != nullptr) code = child->addStmt(root, method, code, isMain);
            }
            return code;
        }

        case ST_ECHO: {
            StmtBuilder::EmitEcho(root, method, code, expr);
            return code;
        }

        case ST_EXPRESSION: {
            ExprBuilder::EmitValue(root, method, code, expr);
            *code << code->PopOne();
            return code;
        }

        case ST_RETURN: {
            if (isMain) {
                if (expr != nullptr) {
                    ExprBuilder::EmitValue(root, method, code, expr);
                    *code << code->PopOne();
                }
                *code << code->ReturnVoid();
                return code;
            }

            if (expr != nullptr) {
                ExprBuilder::EmitValue(root, method, code, expr);
            } else {
                *code << code->GetStatic(nullValueField);
            }
            *code << code->ReturnReference();
            return code;
        }

        // ============================================================
        // IF / ELSEIF / ELSE
        // ============================================================
        case ST_IF: {
            auto *L_end = code->CodeLabel();
            auto *L_next = code->CodeLabel(); // where we go if "if" condition is false

            // if (!cond) goto L_next
            emitJumpIfFalse(condition, L_next);

            // then-body
            if (stmt) code = stmt->addStmt(root, method, code, isMain);

            // after executing then-body, skip the rest of chain
            *code << code->GoTo(L_end);

            // else/elseif entry
            *code << L_next;

            // else-if chain
            const StmtNode *elif = elseIfStmt;
            while (elif != nullptr && elif->type == ST_ELSE_IF) {
                auto *L_elifNext = code->CodeLabel();

                emitJumpIfFalse(elif->condition, L_elifNext);

                if (elif->stmt) code = elif->stmt->addStmt(root, method, code, isMain);
                *code << code->GoTo(L_end);

                *code << L_elifNext;

                // Support chaining via elif->elseIfStmt (even if your factory doesn't set it,
                // your parser might). If not set, chain ends.
                elif = elif->elseIfStmt;
            }

            // else block (your AST often wraps it as ST_ELSE with .stmt)
            if (elseStmt != nullptr) {
                if (elseStmt->type == ST_ELSE) {
                    if (elseStmt->stmt) code = elseStmt->stmt->addStmt(root, method, code, isMain);
                } else {
                    // if parser stored else body directly
                    code = elseStmt->addStmt(root, method, code, isMain);
                }
            }

            *code << L_end;
            return code;
        }

        // ============================================================
        // WHILE
        // ============================================================
        case ST_WHILE: {
            auto *L_cond = code->CodeLabel();
            auto *L_break = code->CodeLabel();

            // continue in while goes to condition check
            pushLoop(L_break, L_cond);

            *code << L_cond;
            emitJumpIfFalse(condition, L_break);

            if (stmt) code = stmt->addStmt(root, method, code, isMain);

            *code << code->GoTo(L_cond);
            *code << L_break;

            popLoop();
            return code;
        }

        // ============================================================
        // DO-WHILE
        // ============================================================
        case ST_DO_WHILE: {
            auto *L_body = code->CodeLabel();
            auto *L_check = code->CodeLabel();
            auto *L_break = code->CodeLabel();

            // continue in do-while goes to the condition check at the bottom
            pushLoop(L_break, L_check);

            *code << L_body;
            if (stmt) code = stmt->addStmt(root, method, code, isMain);

            *code << L_check;
            // if (!cond) break; else loop
            emitJumpIfFalse(condition, L_break);
            *code << code->GoTo(L_body);

            *code << L_break;

            popLoop();
            return code;
        }

        // ============================================================
        // FOR (initializer; condition; endAction) { body }
        // ============================================================
        case ST_FOR: {
            // init;
            emitEvalAndPop(loopInitializer);

            auto *L_cond = code->CodeLabel();
            auto *L_body = code->CodeLabel();
            auto *L_continue = code->CodeLabel(); // runs endAction then jumps to cond
            auto *L_break = code->CodeLabel();

            // continue in for goes to endAction (then condition)
            pushLoop(L_break, L_continue);

            // condition check
            *code << L_cond;
            if (condition != nullptr) {
                emitJumpIfFalse(condition, L_break);
            }
            // if condition is null => infinite loop (no check)

            *code << L_body;
            if (stmt) code = stmt->addStmt(root, method, code, isMain);

            // continue target: endAction; goto cond
            *code << L_continue;
            emitEvalAndPop(loopEndAction);
            *code << code->GoTo(L_cond);

            *code << L_break;

            popLoop();
            return code;
        }

        // ============================================================
        // BREAK / CONTINUE
        // ============================================================
        case ST_BREAK: {
            auto *L = currentBreak();
            if (!L) {
                Warn("ST_BREAK used outside of loop (ignored)");
                return code;
            }
            *code << code->GoTo(L);
            return code;
        }

        case ST_CONTINUE: {
            auto *L = currentContinue();
            if (!L) {
                Warn("ST_CONTINUE used outside of loop (ignored)");
                return code;
            }
            *code << code->GoTo(L);
            return code;
        }

        // ============================================================
        // Not needed right now (or rewritten in semantics)
        // ============================================================
        case ST_SWITCH:
        case ST_CASE:
        case ST_CASE_DEFAULT:
        case ST_FOREACH:
        case ST_THROW:
        case ST_CATCH:
        case ST_FINALLY:
        case ST_TRY:
        case ST_ELSE_IF:
        case ST_ELSE:
        default:
            Warn("no bytecode implementation for " + toString(type));
            return code;
    }
}

// List
StmtNode *StmtNode::StmtList(StmtNode *stmt) {
    auto node = new StmtNode();
    node->type = StmtType::ST_STMT_LIST;
    if (stmt != nullptr) {
        node->children.push_back(stmt);
    }
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::AppendToStmtList(StmtNode *stmtList, StmtNode *newStmt) {
    if (newStmt != nullptr) {
        stmtList->children.push_back(newStmt);
    }
    stmtList->WriteToFiles();
    return stmtList;
}

StmtNode *StmtNode::ExprStmt(ExprNode *expr) {
    auto node = new StmtNode();
    node->type = ST_EXPRESSION;
    node->expr = expr;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::While(ExprNode *condition, StmtNode *stmt) {
    auto node = new StmtNode();
    node->type = ST_WHILE;
    node->condition = condition;
    node->stmt = stmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::DoWhile(ExprNode *condition, StmtNode *stmt) {
    auto node = new StmtNode();
    node->type = ST_DO_WHILE;
    node->condition = condition;
    node->stmt = stmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::For(ExprNode *initializer, ExprNode *condition, ExprNode *endAction, StmtNode *stmt) {
    auto node = new StmtNode();
    node->type = ST_FOR;
    node->loopInitializer = initializer;
    node->condition = condition;
    node->loopEndAction = endAction;
    node->stmt = stmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::ForEachSimple(ExprNode *collection, ExprNode *value, StmtNode *stmt) {
    auto node = new StmtNode();
    node->type = ST_FOREACH;
    node->foreachCollection = collection;
    node->foreachValue = value;
    node->stmt = stmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::ForEachKeyValue(ExprNode *collection, ExprNode *key, ExprNode *value, StmtNode *stmt) {
    auto node = new StmtNode();
    node->type = ST_FOREACH;
    node->foreachCollection = collection;
    node->foreachKey = key;
    node->foreachValue = value;
    node->stmt = stmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::If(ExprNode *condition, StmtNode *stmt) {
    auto node = new StmtNode();
    node->type = ST_IF;
    node->condition = condition;
    node->stmt = stmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::If_ElifElse(ExprNode *condition, StmtNode *stmt, StmtNode *elseIfStmt, StmtNode *elseStmt) {
    auto node = new StmtNode();
    node->type = ST_IF;
    node->condition = condition;
    node->stmt = stmt;
    node->elseIfStmt = elseIfStmt;
    node->elseStmt = elseStmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::If_Elif(ExprNode *condition, StmtNode *stmt, StmtNode *elseIfStmt) {
    auto node = new StmtNode();
    node->type = ST_IF;
    node->condition = condition;
    node->stmt = stmt;
    node->elseIfStmt = elseIfStmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::If_Else(ExprNode *condition, StmtNode *stmt, StmtNode *elseStmt) {
    auto node = new StmtNode();
    node->type = ST_IF;
    node->condition = condition;
    node->stmt = stmt;
    node->elseStmt = elseStmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::ElseIf(ExprNode *condition, StmtNode *stmt) {
    auto node = new StmtNode();
    node->type = ST_ELSE_IF;
    node->condition = condition;
    node->stmt = stmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::Else(StmtNode *stmt) {
    auto node = new StmtNode();
    node->type = ST_ELSE;
    node->stmt = stmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::Switch(ExprNode *expr, StmtNode *stmt) {
    auto node = new StmtNode();
    node->type = ST_SWITCH;
    node->expr = expr;
    node->stmt = stmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::Case(ExprNode *expr, StmtNode *stmt) {
    auto node = new StmtNode();
    node->type = ST_CASE;
    node->expr = expr;
    node->stmt = stmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::CaseDefault(StmtNode *stmt) {
    auto node = new StmtNode();
    node->type = ST_CASE_DEFAULT;
    node->stmt = stmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::Echo(ExprNode *expr) {
    auto node = new StmtNode();
    node->type = ST_ECHO;
    node->expr = expr;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::ReturnStmt() {
    auto node = new StmtNode();
    node->type = ST_RETURN;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::ReturnStmt(ExprNode *expr) {
    auto node = new StmtNode();
    node->type = ST_RETURN;
    node->expr = expr;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::ThrowStmt(ExprNode *expr) {
    auto node = new StmtNode();
    node->type = ST_THROW;
    node->expr = expr;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::CatchStmt(StmtNode *stmt, ValueNode *catchType, string *catchId) {
    auto node = new StmtNode();
    node->type = ST_CATCH;
    node->stmt = stmt;
    node->catchType = catchType;
    node->catchId = *catchId;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::FinallyStmt(StmtNode *stmt) {
    auto node = new StmtNode();
    node->type = ST_FINALLY;
    node->stmt = stmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::TryCatchStmt(StmtNode *stmt, StmtNode *catchStmt) {
    auto node = new StmtNode();
    node->type = ST_TRY;
    node->stmt = stmt;
    node->catchStmt = catchStmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::TryFinallyStmt(StmtNode *stmt, StmtNode *finallyStmt) {
    auto node = new StmtNode();
    node->type = ST_TRY;
    node->stmt = stmt;
    node->finallyStmt = finallyStmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::TryCatchFinallyStmt(StmtNode *stmt, StmtNode *catchStmt, StmtNode *finallyStmt) {
    auto node = new StmtNode();
    node->type = ST_TRY;
    node->stmt = stmt;
    node->catchStmt = catchStmt;
    node->finallyStmt = finallyStmt;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::BreakStmt() {
    auto node = new StmtNode();
    node->type = ST_BREAK;
    node->WriteToFiles();
    return node;
}

StmtNode *StmtNode::ContinueStmt() {
    auto node = new StmtNode();
    node->type = ST_CONTINUE;
    node->WriteToFiles();
    return node;
}
