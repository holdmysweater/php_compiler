#include "StmtNode.h"
#include "core/helpers/Console.h"
#include "json.hpp"

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

    if (condition != nullptr && condition->type == ExprType::ET_EXPR_LIST) {
        if (condition->children.size() == 1) {
            auto list = condition;
            condition = list->children[0];
            delete list;
        } else {
            Warn("condition is a list");
        }
    }

    if (expr != nullptr && expr->type == ExprType::ET_EXPR_LIST) {
        if (expr->children.size() == 1) {
            auto list = expr;
            expr = list->children[0];
            delete list;
        } else {
            Warn("expr is a list");
        }
    }

    if (loopInitializer != nullptr && loopInitializer->type == ExprType::ET_EXPR_LIST) {
        if (loopInitializer->children.size() == 1) {
            auto list = loopInitializer;
            loopInitializer = list->children[0];
            delete list;
        } else {
            Warn("loopInitializer is a list");
        }
    }

    if (loopEndAction != nullptr && loopEndAction->type == ExprType::ET_EXPR_LIST) {
        if (loopEndAction->children.size() == 1) {
            auto list = loopEndAction;
            loopEndAction = list->children[0];
            delete list;
        } else {
            Warn("loopEndAction is a list");
        }
    }
    if (foreachCollection != nullptr && foreachCollection->type == ExprType::ET_EXPR_LIST) {
        if (foreachCollection->children.size() == 1) {
            auto list = foreachCollection;
            foreachCollection = list->children[0];
            delete list;
        } else {
            Warn("foreachCollection is a list");
        }
    }

    if (foreachKey != nullptr && foreachKey->type == ExprType::ET_EXPR_LIST) {
        if (foreachKey->children.size() == 1) {
            auto list = foreachKey;
            foreachKey = list->children[0];
            delete list;
        } else {
            Warn("foreachKey is a list");
        }
    }

    if (foreachValue != nullptr && foreachValue->type == ExprType::ET_EXPR_LIST) {
        if (foreachValue->children.size() == 1) {
            auto list = foreachValue;
            foreachValue = list->children[0];
            delete list;
        } else {
            Warn("foreachValue is a list");
        }
    }

    if (stmt != nullptr && stmt->type == ST_STMT_LIST) {
        if (stmt->children.size() == 1) {
            auto list = stmt;
            stmt = list->children[0];
            delete list;
        } else {
            Warn("stmt is a list");
        }
    }

    if (elseIfStmt != nullptr && elseIfStmt->type == ST_STMT_LIST) {
        if (elseIfStmt->children.size() == 1) {
            auto list = elseIfStmt;
            elseIfStmt = list->children[0];
            delete list;
        } else {
            Warn("elseIfStmt is a list");
        }
    }

    if (elseStmt != nullptr && elseStmt->type == ST_STMT_LIST) {
        if (elseStmt->children.size() == 1) {
            auto list = elseStmt;
            elseStmt = list->children[0];
            delete list;
        } else {
            Warn("elseStmt is a list");
        }
    }

    if (elseStmt != nullptr && elseStmt->type == ST_ELSE) {
        auto list = elseStmt;
        elseStmt = list->stmt;
        delete list;
    }

    if (catchStmt != nullptr && catchStmt->type == ST_STMT_LIST) {
        if (catchStmt->children.size() == 1) {
            auto list = catchStmt;
            catchStmt = list->children[0];
            delete list;
        } else {
            Warn("catchStmt is a list");
        }
    }

    if (finallyStmt != nullptr && finallyStmt->type == ST_STMT_LIST) {
        if (finallyStmt->children.size() == 1) {
            auto list = finallyStmt;
            finallyStmt = list->children[0];
            delete list;
        } else {
            Warn("finallyStmt is a list");
        }
    }

    if (isOk) {
        Log("finished semantics for " + toString(type) + "");
    } else {
        Error("semantics for " + toString(type) + " failed");
    }

    return isOk;
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
