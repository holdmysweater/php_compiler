#ifndef PHP_COMPILER_VALUE_H
#define PHP_COMPILER_VALUE_H

#include "BaseNode.h"
#include "enums/ValueType.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

class ValueNode : public BaseNode {
public:
    ValueType type = ValueType::TYPE_UNKNOWN;
    string name;
    int dimensions = 0;
    ValueNode *value = nullptr;

    int intValue = 0;
    float floatValue = 0;
    bool boolValue = false;
    string stringValue;
    vector<ValueNode *> valueList;

    string _getClassName() const override;

    string toJson() const override;

    string toDot() const override;

    bool doSemantics() override;

    // List methods
    static ValueNode *ValueList(ValueNode *value);

    static ValueNode *AppendToValueList(ValueNode *valueList, ValueNode *newValue);

    // Values
    static ValueNode *CreateInt(int val);

    static ValueNode *CreateFloat(float val);

    static ValueNode *CreateBool(bool val);

    static ValueNode *CreateString(string *val);

    static ValueNode *CreateIdentifier(string *name);

    static ValueNode *CreateClass(string *name);

    static ValueNode *CreateFunction(string *name);

    static ValueNode *CreateType(string *name);

    static ValueNode *CreateTypeNull();
};

#endif //PHP_COMPILER_VALUE_H
