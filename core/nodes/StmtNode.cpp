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

    result += "  node" + std::to_string(GetId()) + " [label=\"" + label + "\", fillcolor=\"#ADD8E6\", style=filled];\n";

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

    for (const auto &child: children) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(child->GetId()) +
                " [label=children];\n";
        result += child->toDot();
    }

    return result;
}

bool StmtNode::doSemantics() {
    Log("starting semantics for " + toString(type) + "...");

    bool isOk = true;
    switch (type) {
        case ST_UNKNOWN:
            Warn("unknown type");
            return true;
        case ST_STMT_LIST:
            for (const auto &child: children) {
                isOk = isOk && child->doSemantics();
            }
            break;
        case ST_EXPRESSION:
            // TODO ST_EXPRESSION
            Warn("ST_EXPRESSION not implemented");
            break;
        case ST_WHILE:
            // TODO ST_WHILE
            Warn("ST_WHILE not implemented");
            break;
        case ST_DO_WHILE:
            // TODO ST_DO_WHILE
            Warn("ST_DO_WHILE not implemented");
            break;
        case ST_FOR:
            // TODO ST_FOR
            Warn("ST_FOR not implemented");
            break;
        case ST_FOREACH:
            // TODO ST_FOREACH
            Warn("ST_FOREACH not implemented");
            break;
        case ST_IF:
            // TODO ST_IF
            Warn("ST_IF not implemented");
            break;
        case ST_ELSE_IF:
            // TODO ST_ELSE_IF
            Warn("ST_ELSE_IF not implemented");
            break;
        case ST_ELSE:
            // TODO ST_ELSE
            Warn("ST_ELSE not implemented");
            break;
        case ST_SWITCH:
            // TODO ST_SWITCH
            Warn("ST_SWITCH not implemented");
            break;
        case ST_CASE:
            // TODO ST_CASE
            Warn("ST_CASE not implemented");
            break;
        case ST_CASE_DEFAULT:
            // TODO ST_CASE_DEFAULT
            Warn("ST_CASE_DEFAULT not implemented");
            break;
        case ST_ECHO:
            // TODO ST_ECHO
            Warn("ST_ECHO not implemented");
            break;
        case ST_RETURN:
            // TODO ST_RETURN
            Warn("ST_RETURN not implemented");
            break;
        case ST_BREAK:
            // TODO ST_BREAK
            Warn("ST_BREAK not implemented");
            break;
        case ST_CONTINUE:
            // TODO ST_CONTINUE
            Warn("ST_CONTINUE not implemented");
            break;
        case ST_TRY:
            // TODO ST_TRY
            Warn("ST_TRY not implemented");
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
