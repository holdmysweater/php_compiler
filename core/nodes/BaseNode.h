#ifndef PHP_COMPILER_BASENODE_H
#define PHP_COMPILER_BASENODE_H

#include "../macros.h"

#include <cstdint>
#include <string>

using std::string;

class BaseNode {
protected:
    static uint32_t maxId;
    uint32_t id;

    BaseNode() : id(++maxId) {
    }

public:
    virtual ~BaseNode() = default;

    uint32_t GetId() const;

    void WriteToFiles() const;

    void Log(string message) const;

    void Warn(string message) const;

    void Error(string message) const;

    virtual string _getClassName() const = 0;

    virtual string toJson() const = 0;

    virtual string toDot() const = 0;

    virtual bool doSemantics() const = 0;
};

#endif //PHP_COMPILER_BASENODE_H
