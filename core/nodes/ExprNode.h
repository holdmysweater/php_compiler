#ifndef PHP_COMPILER_EXPRNODE_H
#define PHP_COMPILER_EXPRNODE_H

#include "BaseNode.h"
#include "enums/ExprType.h"

#include <vector>

#include "ValueNode.h"

using std::vector;

class ExprNode : public BaseNode {
public:
    ExprType type = ExprType::ET_UNKNOWN;
    ValueNode *value = nullptr;
    vector<ExprNode *> children;

    string toJson() const override;

    string toDot() const override;

    // List
    static ExprNode *ExprList(ExprNode *expr);

    static ExprNode *AppendToExprList(ExprNode *exprList, ExprNode *newExpr);

    // Binary operators (left and right operands)
    static ExprNode *Or(ExprNode *left, ExprNode *right);

    static ExprNode *OrLower(ExprNode *left, ExprNode *right);

    static ExprNode *Xor(ExprNode *left, ExprNode *right);

    static ExprNode *XorBitwise(ExprNode *left, ExprNode *right);

    static ExprNode *And(ExprNode *left, ExprNode *right);

    static ExprNode *AndLower(ExprNode *left, ExprNode *right);

    static ExprNode *OrBitwise(ExprNode *left, ExprNode *right);

    static ExprNode *AndBitwise(ExprNode *left, ExprNode *right);

    // Assignment operators (left and right operands)
    static ExprNode *Assign(ExprNode *left, ExprNode *right);

    static ExprNode *MultAssign(ExprNode *left, ExprNode *right);

    static ExprNode *PowAssign(ExprNode *left, ExprNode *right);

    static ExprNode *DivAssign(ExprNode *left, ExprNode *right);

    static ExprNode *ModAssign(ExprNode *left, ExprNode *right);

    static ExprNode *PlusAssign(ExprNode *left, ExprNode *right);

    static ExprNode *MinusAssign(ExprNode *left, ExprNode *right);

    static ExprNode *ConcatAssign(ExprNode *left, ExprNode *right);

    static ExprNode *LeftShiftAssign(ExprNode *left, ExprNode *right);

    static ExprNode *RightShiftAssign(ExprNode *left, ExprNode *right);

    // Comparison operators (left and right operands)
    static ExprNode *Equal(ExprNode *left, ExprNode *right);

    static ExprNode *NotEqual(ExprNode *left, ExprNode *right);

    static ExprNode *NotEqualBitwise(ExprNode *left, ExprNode *right);

    static ExprNode *IdenticallyEqual(ExprNode *left, ExprNode *right);

    static ExprNode *IdenticallyNotEqual(ExprNode *left, ExprNode *right);

    static ExprNode *Spaceship(ExprNode *left, ExprNode *right);

    static ExprNode *NullCoaslescing(ExprNode *left, ExprNode *right);

    static ExprNode *LessThan(ExprNode *left, ExprNode *right);

    static ExprNode *GreaterThan(ExprNode *left, ExprNode *right);

    static ExprNode *LessOrEqual(ExprNode *left, ExprNode *right);

    static ExprNode *GreatOrEqual(ExprNode *left, ExprNode *right);

    // Bitwise shift operators (left and right operands)
    static ExprNode *LeftShift(ExprNode *left, ExprNode *right);

    static ExprNode *RightShift(ExprNode *left, ExprNode *right);

    // Arithmetic operators (left and right operands)
    static ExprNode *Add(ExprNode *left, ExprNode *right);

    static ExprNode *Subtract(ExprNode *left, ExprNode *right);

    static ExprNode *Concat(ExprNode *left, ExprNode *right);

    static ExprNode *Mult(ExprNode *left, ExprNode *right);

    static ExprNode *Mod(ExprNode *left, ExprNode *right);

    static ExprNode *Div(ExprNode *left, ExprNode *right);

    static ExprNode *Pow(ExprNode *left, ExprNode *right);

    // Other binary operators
    static ExprNode *Instanceof(ExprNode *left, ExprNode *right);

    static ExprNode *PropertyAccess(ExprNode *left, ExprNode *right);

    static ExprNode *MethodAccess(ExprNode *left, ExprNode *right, ExprNode *params);

    static ExprNode *StaticPropertyAccess(ExprNode *left, ExprNode *right);

    static ExprNode *ArrayIndex(ExprNode *array, ExprNode *index);

    static ExprNode *ArrayKeyAccess(ExprNode *key, ExprNode *value);

    static ExprNode *ArrayAppend(ExprNode *array, ExprNode *element);

    static ExprNode *Array();

    static ExprNode *SpreadArray(ExprNode *array);

    // Ternary operator (condition, trueExpr, falseExpr)
    static ExprNode *Ternary(ExprNode *condition, ExprNode *trueExpr, ExprNode *falseExpr);

    // Unary operators (single operand)
    static ExprNode *IncrementPost(ExprNode *operand);

    static ExprNode *DecrementPost(ExprNode *operand);

    static ExprNode *IncrementPre(ExprNode *operand);

    static ExprNode *DecrementPre(ExprNode *operand);

    static ExprNode *Not(ExprNode *operand);

    static ExprNode *NotBitwise(ExprNode *operand);

    static ExprNode *Uplus(ExprNode *operand);

    static ExprNode *Uminus(ExprNode *operand);

    static ExprNode *Sigil(ExprNode *operand);

    // Array operations
    static ExprNode *ArrayElementList(ExprNode *elements);

    static ExprNode *Parenthesized(ExprNode *expr);

    // Function/method calls
    static ExprNode *FunctionCall(ExprNode *function, ExprNode *arguments);

    // Object instantiation
    static ExprNode *New(ExprNode *className, ExprNode *arguments);

    static ExprNode *New(ExprNode *arguments);

    // Literals
    static ExprNode *ComplexString(ExprNode *arguments);

    static ExprNode *String(string *value);

    static ExprNode *Id(string *name);

    static ExprNode *Int(int value);

    static ExprNode *Float(float value);

    static ExprNode *Bool(bool value);

    static ExprNode *Nil();
};

#endif //PHP_COMPILER_EXPRNODE_H
