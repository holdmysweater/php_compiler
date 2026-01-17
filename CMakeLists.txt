cmake_minimum_required(VERSION 3.16)
project(php_compiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Flex and Bison
find_program(FLEX_EXECUTABLE flex REQUIRED)
find_program(BISON_EXECUTABLE bison REQUIRED)

message(STATUS "Flex: ${FLEX_EXECUTABLE}")
message(STATUS "Bison: ${BISON_EXECUTABLE}")

# Set paths
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}")
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")

# Create output directory for generated files
file(MAKE_DIRECTORY "${GENERATED_DIR}")

# Bison generates both .cpp and .hpp files
set(GENERATED_PARSER_CPP "${GENERATED_DIR}/php_parser.cpp")
set(GENERATED_PARSER_HPP "${GENERATED_DIR}/php_parser.hpp")

# Flex generates .cpp file
set(GENERATED_LEXER_CPP "${GENERATED_DIR}/lex.yy.cpp")

# Bison command - use -o with just filename, let working directory handle the path
add_custom_command(
        OUTPUT "${GENERATED_PARSER_CPP}" "${GENERATED_PARSER_HPP}"
        COMMAND "${BISON_EXECUTABLE}"
        ARGS -Wall -d -o php_parser.cpp "${SOURCE_DIR}/php_parser.y"
        DEPENDS "${SOURCE_DIR}/php_parser.y"
        WORKING_DIRECTORY "${GENERATED_DIR}"
        COMMENT "Generating parser with Bison"
        VERBATIM
)

# Flex command
add_custom_command(
        OUTPUT "${GENERATED_LEXER_CPP}"
        COMMAND "${FLEX_EXECUTABLE}"
        ARGS -o "${GENERATED_DIR}/lex.yy.cpp" "${SOURCE_DIR}/php_lexer.l"
        DEPENDS "${SOURCE_DIR}/php_lexer.l" "${GENERATED_PARSER_HPP}"
        WORKING_DIRECTORY "${GENERATED_DIR}"
        COMMENT "Generating lexer with Flex"
        VERBATIM
)

# Add jvm-class-builder
add_subdirectory(jvm-class-builder)

# Core sources
set(CORE_SOURCES
        core/helpers/Config.cpp
        core/helpers/Token.cpp
        core/helpers/TokenQueue.cpp
        core/helpers/Console.cpp
        core/helpers/Color.cpp
        core/helpers/Buffer.cpp
        core/helpers/StatesTree.cpp
        core/helpers/IndentTracker.cpp
        core/helpers/OutputManager.cpp
)

# AST sources
set(AST_SOURCES
        core/nodes/enums/DeclType.h
        core/nodes/enums/ElementType.h
        core/nodes/enums/ExprType.h
        core/nodes/enums/StmtType.h
        core/nodes/enums/ValueType.h
        core/nodes/enums/VisibilityType.h
        core/nodes/json.hpp
        core/nodes/ValueNode.cpp
        core/nodes/BaseNode.cpp
        core/nodes/ExprNode.cpp
        core/nodes/StmtNode.cpp
        core/nodes/DeclNode.cpp
        core/nodes/ElementNode.cpp
        core/nodes/RawDeclModifier.cpp
)

# Byte code sources
set(BYTE_CODE_SOURCES
        core/bytecode/ByteCodeHelper.cpp
)

# Create executable
add_executable(php_compiler
        main.cpp
        ${CORE_SOURCES}
        ${AST_SOURCES}
        ${BYTE_CODE_SOURCES}
        ${GENERATED_LEXER_CPP}
        ${GENERATED_PARSER_CPP}
)

# Set language for generated files
set_source_files_properties(${GENERATED_LEXER_CPP} PROPERTIES LANGUAGE CXX)
set_source_files_properties(${GENERATED_PARSER_CPP} PROPERTIES LANGUAGE CXX)

# Include directories
target_include_directories(php_compiler PRIVATE
        "${SOURCE_DIR}"
        "${GENERATED_DIR}"
)

# Link libraries
target_link_libraries(php_compiler PRIVATE
        jvm::ClassBuilder
)
