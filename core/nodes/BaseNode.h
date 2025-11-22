#ifndef PHP_COMPILER_BASENODE_H
#define PHP_COMPILER_BASENODE_H

#define DOT_DEBUG

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

    virtual string toJson() const = 0;

    virtual string toDot() const = 0;

    // virtual string doSemantics() const = 0; TODO add to each class
};

#endif //PHP_COMPILER_BASENODE_H
