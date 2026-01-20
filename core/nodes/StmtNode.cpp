#include "StmtNode.h"
#include "core/helpers/Console.h"
#include "json.hpp"
#include "core/bytecode/ExprBuilder.h"
#include "core/bytecode/StmtBuilder.h"
#include "jvm/attribute-code.h"
#include "jvm/descriptor-method.h"
#include "jvm/method.h"
#include <unordered_map>

#include "jvm/field.h"

using json = nlohmann::json;

struct ReturnCtx {
    jvm::Label *L_epilogue = nullptr;
    uint16_t retSlot = 0;
};

static thread_local std::unordered_map<const jvm::Method *, ReturnCtx> s_returnCtx;

static void setReturnCtx(const jvm::Method *m, jvm::Label *L_epilogue, uint16_t retSlot) {
    s_returnCtx[m] = ReturnCtx{L_epilogue, retSlot};
}

static bool getReturnCtx(const jvm::Method *m, ReturnCtx &out) {
    auto it = s_returnCtx.find(m);
    if (it == s_returnCtx.end()) return false;
    out = it->second;
    return true;
}

static void clearReturnCtx(const jvm::Method *m) {
    s_returnCtx.erase(m);
}

static thread_local std::unordered_map<const jvm::Method *, uint16_t> s_tempNextFree;

// Allocate a temp local even in main() (which has no ExprBuilder local-scope).
static uint16_t allocTempAny(jvm::Method *method) {
    uint16_t s = ExprBuilder::AllocTempLocal(method);
    if (s != 0) return s; // normal function/method scope

    // Fallback for main() (slot 0 is String[] args)
    uint16_t &next = s_tempNextFree[method];
    if (next == 0) next = 1;
    return next++;
}

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

        case ST_FINALLY:
        case ST_CATCH:
        case ST_THROW:
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
            Error("unknown enum type: " + toString(type));
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
                std::nullopt,
                {DescriptorField("java/lang/String", 1)}
            )
        );

        mainMethod->addFlag(Method::ACC_PUBLIC);
        mainMethod->addFlag(Method::ACC_STATIC);

        ExprBuilder::SetPhpMethodHasThis(mainMethod, false);
        ExprBuilder::SetPhpCallerClass(mainMethod, "");

        AttributeCode *code = mainMethod->getCodeAttribute();

        addStmt(root, mainMethod, code, true);

        *code << code->ReturnVoid();
    } catch (const std::exception &e) {
        isOk = false;
        Error(std::string("StmtNode::processClass failed: ") + e.what());
    }

    if (isOk) Log("finished semantics for " + toString(type));
    else Error("semantics for " + toString(type) + " failed");

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
    auto childOrNull = [](const ExprNode *e, size_t i) -> const ExprNode * {
        if (!e) return nullptr;
        if (i >= e->children.size()) return nullptr;
        return e->children[i];
    };

    auto idText = [](const ExprNode *e) -> std::string {
        if (!e) return "";
        if (!e->name.empty()) return e->name;
        if (e->value) {
            if (!e->value->name.empty()) return e->value->name;
            if (e->value->stringValue.data()) return std::string(e->value->stringValue);
        }
        return "";
    };

    auto normalizeVar = [](std::string s) -> std::string {
        if (!s.empty() && s[0] == '$') s.erase(s.begin());
        return s;
    };

    auto sanitizeJavaIdent = [](const std::string &raw) -> std::string {
        std::string s = raw;
        if (!s.empty() && s[0] == '$') s.erase(0, 1);

        std::string out;
        out.reserve(s.size() + 4);
        out += "g_";

        for (char c: s) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_')
                out.push_back(c);
            else out.push_back('_');
        }

        if (out.size() == 2) out += "var";
        if (out.size() >= 3 && (out[2] >= '0' && out[2] <= '9')) out.insert(out.begin() + 2, '_');
        return out;
    };

    auto ensureGlobalVarField = [&](const std::string &varName) -> jvm::ConstantFieldref * {
        std::string fieldName = sanitizeJavaIdent(varName);

        jvm::Field *f = root->getOrCreateField(fieldName, jvm::DescriptorField("com/phpjvm/BasePhpValue"));
        f->addFlag(jvm::Field::ACC_PUBLIC);
        f->addFlag(jvm::Field::ACC_STATIC);

        return root->getOrCreateFieldrefConstant(
            root->getClassName(),
            fieldName,
            jvm::DescriptorField("com/phpjvm/BasePhpValue")
        );
    };

    // Consumes a BasePhpValue from stack and stores it into the foreach var
    auto storeToPhpVar = [&](ExprNode *varExpr) {
        // accept ET_ID or ET_SIGIL(ET_ID)
        const ExprNode *v = varExpr;
        if (v && v->type == ExprType::ET_SIGIL) v = childOrNull(v, 0);

        std::string var = normalizeVar(idText(v));
        if (var.empty()) {
            Warn("ST_FOREACH: missing foreach variable (dropping value)");
            *code << code->PopOne();
            return;
        }

        uint16_t slot = 0;
        if (ExprBuilder::TryGetLocal(method, var, slot)) {
            *code << code->StoreReference(slot);
            return;
        }

        // If we are in a real local scope, allocate a slot and register it
        uint16_t newSlot = ExprBuilder::AllocTempLocal(method);
        if (newSlot != 0) {
            ExprBuilder::DefineLocal(method, var, newSlot);
            *code << code->StoreReference(newSlot);
            return;
        }

        // Otherwise: program scope => global static field
        auto *fieldRef = ensureGlobalVarField(var);
        *code << code->PutStatic(fieldRef);
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

            ReturnCtx rctx;
            if (getReturnCtx(method, rctx) && rctx.L_epilogue != nullptr) {
                if (expr != nullptr) {
                    ExprBuilder::EmitValue(root, method, code, expr);
                } else {
                    *code << code->GetStatic(nullValueField);
                }

                // store return value and jump
                *code << code->StoreReference(rctx.retSlot);
                *code << code->GoTo(rctx.L_epilogue);
                return code;
            }

            // fallback (shouldn’t happen once you wire DeclNode correctly)
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

        case ST_FOREACH: {
            // Runtime refs
            auto *foreachIter = root->getOrCreateMethodrefConstant(
                "com/phpjvm/BasePhpValue",
                "foreachIter",
                jvm::DescriptorMethod(
                    jvm::DescriptorField("com/phpjvm/BasePhpValue$ForeachIter"),
                    {jvm::DescriptorField("com/phpjvm/BasePhpValue")}
                )
            );

            auto *iterAdvance = root->getOrCreateMethodrefConstant(
                "com/phpjvm/BasePhpValue$ForeachIter",
                "advance",
                DescriptorMethod(DescriptorField("Z"), {})
            );

            auto *iterValue = root->getOrCreateMethodrefConstant(
                "com/phpjvm/BasePhpValue$ForeachIter",
                "value",
                DescriptorMethod(DescriptorField("com/phpjvm/BasePhpValue"), {})
            );

            auto *iterKey = root->getOrCreateMethodrefConstant(
                "com/phpjvm/BasePhpValue$ForeachIter",
                "key",
                DescriptorMethod(DescriptorField("com/phpjvm/BasePhpValue"), {})
            );

            // Build iterator: it = BasePhpValue.foreachIter(collection)
            ExprBuilder::EmitValue(root, method, code, foreachCollection); // BasePhpValue
            *code << code->InvokeStatic(foreachIter); // ForeachIter

            uint16_t itSlot = allocTempAny(method);
            *code << code->StoreReference(itSlot);

            auto *L_advance = code->CodeLabel(); // continue target
            auto *L_break = code->CodeLabel();

            pushLoop(L_break, L_advance);

            *code << L_advance;

            // if (!it.advance()) break;
            *code << code->LoadReference(itSlot);
            *code << code->InvokeVirtual(iterAdvance); // Z
            *code << code->If(jvm::Instruction::Compare::Equal, L_break);

            // $value = it.value();
            *code << code->LoadReference(itSlot);
            *code << code->InvokeVirtual(iterValue); // BasePhpValue
            storeToPhpVar(foreachValue);

            // optional: $key = it.key();
            if (foreachKey != nullptr) {
                *code << code->LoadReference(itSlot);
                *code << code->InvokeVirtual(iterKey); // BasePhpValue
                storeToPhpVar(foreachKey);
            }

            // body
            if (stmt) code = stmt->addStmt(root, method, code, isMain);

            // loop
            *code << code->GoTo(L_advance);

            *code << L_break;
            popLoop();
            return code;
        }

        case ST_THROW: {
            auto *toThrowable = root->getOrCreateMethodrefConstant(
                "com/phpjvm/PhpRuntime",
                "toThrowable",
                DescriptorMethod(
                    DescriptorField("java/lang/Throwable"),
                    {DescriptorField("com/phpjvm/BasePhpValue")}
                )
            );

            if (expr != nullptr) {
                ExprBuilder::EmitValue(root, method, code, expr); // BasePhpValue
            } else {
                *code << code->GetStatic(nullValueField); // BasePhpValue NULL_VALUE
            }

            *code << code->InvokeStatic(toThrowable); // Throwable
            *code << code->Throw(); // athrow
            return code;
        }

        case ST_TRY: {
            auto *thrClass = root->getOrCreateClassConstant("java/lang/Throwable");

            auto *unwrapThrowable = root->getOrCreateMethodrefConstant(
                "com/phpjvm/PhpRuntime",
                "unwrapThrowable",
                DescriptorMethod(
                    DescriptorField("com/phpjvm/BasePhpValue"),
                    {DescriptorField("java/lang/Throwable")}
                )
            );

            auto *isInstanceOf = root->getOrCreateMethodrefConstant(
                "com/phpjvm/PhpRuntime",
                "isInstanceOf",
                DescriptorMethod(
                    DescriptorField("Z"),
                    {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("java/lang/String")}
                )
            );

            auto collectTypeNames = [](ValueNode *vt) -> std::vector<std::string> {
                std::vector<std::string> out;
                if (!vt) return out;
                try {
                    for (auto *el: vt->valueList) {
                        if (!el) continue;
                        if (!el->name.empty()) out.push_back(el->name);
                    }
                } catch (...) {
                }
                return out;
            };

            auto storeToVarNameFromStack = [&](const std::string &varName) {
                std::string var = varName;
                if (!var.empty() && var[0] == '$') var.erase(var.begin());

                if (var.empty()) {
                    *code << code->PopOne();
                    return;
                }

                uint16_t slot = 0;
                if (ExprBuilder::TryGetLocal(method, var, slot)) {
                    *code << code->StoreReference(slot);
                    return;
                }

                uint16_t newSlot = ExprBuilder::AllocTempLocal(method);
                if (newSlot != 0) {
                    ExprBuilder::DefineLocal(method, var, newSlot);
                    *code << code->StoreReference(newSlot);
                    return;
                }

                auto *fieldRef = ensureGlobalVarField(var);
                *code << code->PutStatic(fieldRef);
            };

            bool hasFinally = (finallyStmt != nullptr);
            bool hasCatch = (catchStmt != nullptr);

            uint16_t thrSlot = allocTempAny(method); // Throwable
            uint16_t exValSlot = allocTempAny(method); // BasePhpValue (unwrapped)

            auto *L_tryStart = code->CodeLabel();
            auto *L_tryEnd = code->CodeLabel();
            auto *L_handler = code->CodeLabel();
            auto *L_after = code->CodeLabel();

            jvm::Label *L_finallyEntry = nullptr;
            jvm::Label *L_handlerFromCatchBody = nullptr;

            if (hasFinally) {
                L_finallyEntry = code->CodeLabel();
                L_handlerFromCatchBody = code->CodeLabel();

                // thrSlot = null (so finally knows "normal exit")
                *code << code->PushNull();
                *code << code->StoreReference(thrSlot);
            }

            // --- try block ---
            *code << L_tryStart;
            if (stmt) code = stmt->addStmt(root, method, code, isMain);
            *code << L_tryEnd;

            *code << code->GoTo(hasFinally ? L_finallyEntry : L_after);

            // --- handler for exceptions thrown in try ---
            *code << L_handler;
            *code << code->StoreReference(thrSlot); // store Throwable caught by JVM

            if (hasCatch) {
                // exValSlot = PhpRuntime.unwrapThrowable(thr)
                *code << code->LoadReference(thrSlot);
                *code << code->InvokeStatic(unwrapThrowable);
                *code << code->StoreReference(exValSlot);

                // normalize catch list
                std::vector<StmtNode *> catches;
                if (catchStmt->type == ST_STMT_LIST) {
                    for (auto *c: catchStmt->children) if (c && c->type == ST_CATCH) catches.push_back(c);
                } else if (catchStmt->type == ST_CATCH) {
                    catches.push_back(catchStmt);
                }

                for (auto *c: catches) {
                    auto *L_nextCatch = code->CodeLabel();
                    auto *L_matched = code->CodeLabel();

                    auto typeNames = collectTypeNames(c->catchType);
                    if (typeNames.empty()) {
                        // catch () { } is invalid in PHP; treat as non-match
                        *code << code->GoTo(L_nextCatch);
                    } else {
                        // if any type matches => L_matched
                        for (size_t i = 0; i < typeNames.size(); i++) {
                            auto *L_nextType = (i + 1 < typeNames.size()) ? code->CodeLabel() : nullptr;

                            *code << code->LoadReference(exValSlot);
                            *code << code->PushString(typeNames[i]);
                            *code << code->InvokeStatic(isInstanceOf); // Z
                            if (L_nextType) {
                                *code << code->If(jvm::Instruction::Compare::Equal, L_nextType); // 0 => next type
                                *code << code->GoTo(L_matched); // 1 => matched
                                *code << L_nextType;
                            } else {
                                *code << code->If(jvm::Instruction::Compare::Equal, L_nextCatch);
                                // last: 0 => next catch
                                *code << code->GoTo(L_matched); // 1 => matched
                            }
                        }
                    }

                    *code << L_matched;

                    // $e = exValSlot
                    if (!c->catchId.empty()) {
                        *code << code->LoadReference(exValSlot);
                        storeToVarNameFromStack(c->catchId);
                    }

                    // catch body (protect it for finally if present)
                    jvm::Label *L_catchBodyStart = nullptr;
                    jvm::Label *L_catchBodyEnd = nullptr;
                    if (hasFinally) {
                        L_catchBodyStart = code->CodeLabel();
                        L_catchBodyEnd = code->CodeLabel();
                        *code << L_catchBodyStart;
                    }

                    if (c->stmt) code = c->stmt->addStmt(root, method, code, isMain);

                    if (hasFinally) {
                        *code << L_catchBodyEnd;
                        // if catch body throws, we still want finally
                        code->addCatchAll(L_catchBodyStart, L_catchBodyEnd, L_handlerFromCatchBody);

                        // normal handled catch: thrSlot = null; goto finally
                        *code << code->PushNull();
                        *code << code->StoreReference(thrSlot);
                        *code << code->GoTo(L_finallyEntry);
                    } else {
                        *code << code->GoTo(L_after);
                    }

                    *code << L_nextCatch;
                }
            }

            // no catch matched (or no catches): rethrow
            if (hasFinally) {
                // thrSlot already holds the thrown throwable
                *code << code->GoTo(L_finallyEntry); // finally will rethrow (thrSlot != null)
            } else {
                *code << code->LoadReference(thrSlot);
                *code << code->Throw();
            }

            // --- handler for exceptions thrown inside catch bodies (finally only) ---
            if (hasFinally) {
                *code << L_handlerFromCatchBody;
                *code << code->StoreReference(thrSlot); // Throwable
                *code << code->GoTo(L_finallyEntry);

                // finally:
                *code << L_finallyEntry;
                if (finallyStmt->stmt) code = finallyStmt->stmt->addStmt(root, method, code, isMain);

                auto *L_noThrow = code->CodeLabel();
                *code << code->LoadReference(thrSlot);
                *code << code->IfNull(L_noThrow); // if null => normal continue
                *code << code->LoadReference(thrSlot);
                *code << code->Throw(); // rethrow
                *code << L_noThrow;

                *code << L_after;
            } else {
                *code << L_after;
            }

            // exception table: protect try region
            code->addTryCatch(L_tryStart, L_tryEnd, L_handler, thrClass);

            return code;
        }

        case ST_CATCH:
        case ST_FINALLY: {
            if (stmt) code = stmt->addStmt(root, method, code, isMain);
            return code;
        }

        // ============================================================
        // Not needed right now (or rewritten in semantics)
        // ============================================================
        case ST_SWITCH:
        case ST_CASE:
        case ST_CASE_DEFAULT:
        case ST_ELSE_IF:
        case ST_ELSE:
        default:
            Warn("no bytecode implementation for " + toString(type));
            return code;
    }
}

void StmtNode::BeginReturnCtx(jvm::Method *method, jvm::Label *L_epilogue, uint16_t retSlot) {
    setReturnCtx(method, L_epilogue, retSlot);
}

void StmtNode::EndReturnCtx(jvm::Method *method) {
    clearReturnCtx(method);
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
