%code requires {
    #include <string.h>
    #include <memory>
    #include "core/helpers/Console.h"
    #include "core/nodes/BaseNode.h"
    #include "core/nodes/DeclNode.h"
    #include "core/nodes/ElementNode.h"
    #include "core/nodes/ExprNode.h"
    #include "core/nodes/RawDeclModifier.h"
    #include "core/nodes/StmtNode.h"
    #include "core/nodes/ValueNode.h"
    #include "core/nodes/enums/VisibilityType.h"
    #include "core/nodes/enums/ElementType.h"

    extern ElementNode* root;
}

%code provides {
    extern int yylex();
    void yyerror(const char* message);
}

%union {
    int int_const;
    float float_const;
    string* string_const;
    char* char_const;
    ElementNode* elementNode;
    DeclNode* declNode;
    StmtNode* stmtNode;
    ExprNode* exprNode;
    ValueNode* valueNode;
    RawDeclModifier* modifier;
}

%token <int_const> INT BOOL
%token <float_const> FLOAT
%token <string_const> STRING ID TYPE HTML_CONTENT

%type <char_const> error
%type <elementNode> program mixed_content_list mixed_content_element php_content php_program_list php_program_element
%type <declNode> function_definition class_declaration function_definition_header parameter_function_list_empty parameter_function_list parameter_function class_member_declarations_empty class_member_declarations class_member_declaration class_const_elements property_declaration method_declaration const_elements const_element property_elements property_element
%type <stmtNode> statement while_statement for_statement statement_list_empty foreach_statement if_statement switch_statement statement_list else_statement_empty_1 elseif_statements_1 else_statement_empty_2 elseif_statements_2 elseif_statement_1 elseif_statement_2 case_statements_empty case_statements case_statement
%type <exprNode> expression array_element array_element_list expression_list expression_list_empty string interpolatable_elements interpolatable_element simple_interpolated_expression complex_interpolated_expression
%type <valueNode> type_list type
%type <modifier> visibility_modifiers declaration_modifiers

%token ERROR
%token NIL
%token CONST
%token CLASS
%token EXTENDS
%token PUBLIC
%token PROTECTED
%token PRIVATE
%token AS
%token BREAK
%token CONTINUE
%token DO
%token WHILE
%token ENDWHILE
%token FOR
%token ENDFOR
%token FOREACH
%token ENDFOREACH
%token ECHO_KW
%token IF
%token ELSEIF
%token ELSE
%token ENDIF
%token SWITCH
%token CASE
%token DEFAULT
%token ENDSWITCH
%token FUNCTION
%token RETURN
%token STATIC
%token VAR
%token START_TAG
%token START_ECHO_TAG
%token END_TAG
%token INTERPOLATABLE_START
%token INTERPOLATABLE_END
%token SIMPLE_INTERPOLATION_START
%token COMPLEX_INTERPOLATION_START
%token INTERPOLATION_END
%token KEY_ACCESS

%left OR
%left AND
%left XOR
%right '='  MULT_ASSIGN POW_ASSIGN DIV_ASSIGN PLUS_ASSIGN MINUS_ASSIGN CONCAT_ASSIGN LEFT_SHIFT_ASSIGN RIGHT_SHIFT_ASSIGN
%left '?' ':'
%left LOGIC_OR
%left LOGIC_AND
%left '|'
%left '^'
%left '&'
%nonassoc EQUAL NOT_EQUAL IDENTICALLY_EQUAL IDENTICALLY_NOT_EQUAL NOT_EQUAL_BITWISE
%nonassoc '<' LESS_OR_EQUAL '>' GREAT_OR_EQUAL
%left LEFT_SHIFT RIGHT_SHIFT
%left '+' '-' '.'
%left '*' '/' '%'
%right POW
%precedence '!' '~'
%precedence UMINUS UPLUS
%nonassoc INSTANCEOF
%nonassoc INCREMENT DECREMENT
%nonassoc '[' PROPERTY_ACCESS STATIC_PROPERTY_ACCESS
%precedence '$'
%precedence '('
%precedence NEW

%%

program : %empty           { $$ = root = ElementNode::EmptyElement(); }
        | mixed_content_list    { $$ = root = $1; }
        | error                 { Console::ParserError(); $$ = root = nullptr; }
        | ERROR                 { Console::ParserError("lexer returned an error"); $$ = root = nullptr; }
        ;

mixed_content_list : mixed_content_element                      { $$ = ElementNode::ElementList($1, ElementType::ELEMENT_PROGRAM_LIST); }
                   | mixed_content_list mixed_content_element   { $$ = ElementNode::AppendToElementList($1, $2); }
                   ;

mixed_content_element : HTML_CONTENT    { $$ = ElementNode::HtmlContent($1); }
                      | php_content     { $$ = $1; }
                      ;

php_content : START_TAG php_program_list END_TAG        { $$ = $2; }
            | START_TAG END_TAG                         { $$ = nullptr; }
            | START_ECHO_TAG expression END_TAG         { $$ = ElementNode::PhpEchoContent($2); }
            ;

php_program_list : php_program_element                  { $$ = ElementNode::ElementList($1, ElementType::ELEMENT_PHP_LIST); }
                 | php_program_list php_program_element { $$ = ElementNode::AppendToElementList($1, $2); }
                 ;

php_program_element : statement             { $$ = ElementNode::PhpStmt($1); }
                    | function_definition   { $$ = ElementNode::PhpDecl($1); }
                    | class_declaration     { $$ = ElementNode::PhpDecl($1); }
                    ;

expression : expression LOGIC_OR expression                 { $$ = ExprNode::Or($1, $3); }
          | expression OR expression                        { $$ = ExprNode::OrLower($1, $3); }
          | expression XOR expression                       { $$ = ExprNode::Xor($1, $3); }
          | expression '^' expression                       { $$ = ExprNode::XorBitwise($1, $3); }
          | expression LOGIC_AND expression                 { $$ = ExprNode::And($1, $3); }
          | expression AND expression                       { $$ = ExprNode::AndLower($1, $3); }
          | expression '|' expression                       { $$ = ExprNode::OrBitwise($1, $3); }
          | expression '&' expression                       { $$ = ExprNode::AndBitwise($1, $3); }
          | expression '=' expression                       { $$ = ExprNode::Assign($1, $3); }
          | expression '?' expression ':' expression        { $$ = ExprNode::Ternary($1, $3, $5); }
          | expression MULT_ASSIGN expression               { $$ = ExprNode::MultAssign($1, $3); }
          | expression POW_ASSIGN expression                { $$ = ExprNode::PowAssign($1, $3); }
          | expression DIV_ASSIGN expression                { $$ = ExprNode::DivAssign($1, $3); }
          | expression PLUS_ASSIGN expression               { $$ = ExprNode::PlusAssign($1, $3); }
          | expression MINUS_ASSIGN expression              { $$ = ExprNode::MinusAssign($1, $3); }
          | expression CONCAT_ASSIGN expression             { $$ = ExprNode::ConcatAssign($1, $3); }
          | expression LEFT_SHIFT_ASSIGN expression         { $$ = ExprNode::LeftShiftAssign($1, $3); }
          | expression RIGHT_SHIFT_ASSIGN expression        { $$ = ExprNode::RightShiftAssign($1, $3); }
          | expression EQUAL expression                     { $$ = ExprNode::Equal($1, $3); }
          | expression NOT_EQUAL expression                 { $$ = ExprNode::NotEqual($1, $3); }
          | expression NOT_EQUAL_BITWISE expression         { $$ = ExprNode::NotEqualBitwise($1, $3); }
          | expression IDENTICALLY_EQUAL expression         { $$ = ExprNode::IdenticallyEqual($1, $3); }
          | expression IDENTICALLY_NOT_EQUAL expression     { $$ = ExprNode::IdenticallyNotEqual($1, $3); }
          | expression '<' expression                       { $$ = ExprNode::LessThan($1, $3); }
          | expression '>' expression                       { $$ = ExprNode::GreaterThan($1, $3); }
          | expression LESS_OR_EQUAL expression             { $$ = ExprNode::LessOrEqual($1, $3); }
          | expression GREAT_OR_EQUAL expression            { $$ = ExprNode::GreatOrEqual($1, $3); }
          | expression LEFT_SHIFT expression                { $$ = ExprNode::LeftShift($1, $3); }
          | expression RIGHT_SHIFT expression               { $$ = ExprNode::RightShift($1, $3); }
          | expression '+' expression                       { $$ = ExprNode::Add($1, $3); }
          | expression '-' expression                       { $$ = ExprNode::Subtract($1, $3); }
          | expression '.' expression                       { $$ = ExprNode::Concat($1, $3); }
          | expression '*' expression                       { $$ = ExprNode::Mult($1, $3); }
          | expression '%' expression                       { $$ = ExprNode::Mod($1, $3); }
          | expression '/' expression                       { $$ = ExprNode::Div($1, $3); }
          | expression INSTANCEOF expression                { $$ = ExprNode::Instanceof($1, $3); }
          | expression POW expression                       { $$ = ExprNode::Pow($1, $3); }
          | expression PROPERTY_ACCESS expression           { $$ = ExprNode::PropertyAccess($1, $3); }
          | expression STATIC_PROPERTY_ACCESS expression    { $$ = ExprNode::StaticPropertyAccess($1, $3); }
          | expression INCREMENT                            { $$ = ExprNode::IncrementPost($1); }
          | expression DECREMENT                            { $$ = ExprNode::DecrementPost($1); }
          | INCREMENT expression                            { $$ = ExprNode::IncrementPre($2); }
          | DECREMENT expression                            { $$ = ExprNode::DecrementPre($2); }
          | '!' expression                                  { $$ = ExprNode::Not($2); }
          | '~' expression                                  { $$ = ExprNode::NotBitwise($2); }
          | '+' expression %prec UPLUS                      { $$ = ExprNode::Uplus($2); }
          | '-' expression %prec UMINUS                     { $$ = ExprNode::Uminus($2); }
          | '$' expression                                  { $$ = ExprNode::Sigil($2); }
          | '[' array_element_list ']'                      { $$ = ExprNode::ArrayElementList($2); }
          | expression '[' expression ']'                   { $$ = ExprNode::ArrayIndex($1, $3); }
          | expression '[' ']'                              { $$ = ExprNode::ArrayAppend($1); }
          | '(' expression ')'                              { $$ = ExprNode::Parenthesized($2); }
          | expression '(' expression_list_empty ')'        { $$ = ExprNode::FunctionCall($1, $3); }
          | NEW expression  { $$ = ExprNode::New($2); }
          | string          { $$ = $1; }
          | ID              { $$ = ExprNode::Id($1); }
          | INT             { $$ = ExprNode::Int($1); }
          | FLOAT           { $$ = ExprNode::Float($1); }
          | BOOL            { $$ = ExprNode::Bool($1); }
          | NIL             { $$ = ExprNode::Nil(); }
          ;

array_element_list : array_element                          { $$ = ExprNode::ExprList($1); }
                   | array_element_list ',' array_element   { $$ = ExprNode::AppendToExprList($1, $3); }
                   ;

array_element : expression KEY_ACCESS expression    { $$ = ExprNode::ArrayKeyAccess($1, $3); }
              | expression                          { $$ = $1; }
              ;

expression_list : expression                        { $$ = ExprNode::ExprList($1); }
                | expression_list ',' expression    { $$ = ExprNode::AppendToExprList($1, $3); }
                ;

expression_list_empty : %empty     { $$ = nullptr; }
                      | expression_list { $$ = $1; }
                      ;


statement : '{' statement_list_empty '}'    { $$ = $2; }
          | expression ';'                  { $$ = StmtNode::ExprStmt($1); }
          | while_statement                 { $$ = $1; }
          | for_statement                   { $$ = $1; }
          | foreach_statement               { $$ = $1; }
          | if_statement                    { $$ = $1; }
          | switch_statement                { $$ = $1; }
          | ECHO_KW expression_list ';'     { $$ = StmtNode::Echo($2); }
          | RETURN expression ';'           { $$ = StmtNode::ReturnStmt($2); }
          | RETURN ';'                      { $$ = StmtNode::ReturnStmt(); }
          | BREAK ';'                       { $$ = StmtNode::BreakStmt(); }
          | CONTINUE ';'                    { $$ = StmtNode::ContinueStmt(); }
          | ';'                             { $$ = nullptr; }   
          ;

statement_list : statement                  { $$ = StmtNode::StmtList($1); }
               | statement_list statement   { $$ = StmtNode::AppendToStmtList($1, $2); }
               ;

statement_list_empty : %empty      { $$ = nullptr; }
                     | statement_list   { $$ = $1; }
                     ;


while_statement : WHILE '(' expression ')' statement                        { $$ = StmtNode::While($3, $5); }
                | WHILE '(' expression ')' ':' statement_list ENDWHILE ';'  { $$ = StmtNode::While($3, $6); }
                | DO statement WHILE '(' expression ')' ';'                 { $$ = StmtNode::DoWhile($5, $2); }
                ;

for_statement : FOR '(' expression_list_empty ';' expression_list_empty ';' expression_list_empty ')' statement                     { $$ = StmtNode::For($3, $5, $7, $9); }
              | FOR '(' expression_list_empty ';' expression_list_empty ';' expression_list_empty ')' ':' statement_list ENDFOR ';' { $$ = StmtNode::For($3, $5, $7, $10); }
              ;

foreach_statement : FOREACH '(' expression AS '$' ID KEY_ACCESS '$' ID ')' statement                            { $$ = StmtNode::ForEachKeyValue($3, ExprNode::Id($6), ExprNode::Id($9), $11); }
                  | FOREACH '(' expression AS '$' ID ')' statement                                              { $$ = StmtNode::ForEachSimple($3, ExprNode::Id($6), $8); }
                  | FOREACH '(' expression AS '$' ID KEY_ACCESS '$' ID ')' ':' statement_list ENDFOREACH ';'    { $$ = StmtNode::ForEachKeyValue($3, ExprNode::Id($6), ExprNode::Id($9), $12); }
                  | FOREACH '(' expression AS '$' ID ')' ':' statement_list ENDFOREACH ';'                      { $$ = StmtNode::ForEachSimple($3, ExprNode::Id($6), $9); }
                  ;

if_statement : IF '(' expression ')' statement else_statement_empty_1                                           { $$ = StmtNode::If_Else($3, $5, $6); }
             | IF '(' expression ')' statement elseif_statements_1 else_statement_empty_1                       { $$ = StmtNode::If_ElifElse($3, $5, $6, $7); }
             | IF '(' expression ')' ':' statement_list else_statement_empty_2 ENDIF ';'                        { $$ = StmtNode::If_Else($3, $6, $7); }
             | IF '(' expression ')' ':' statement_list elseif_statements_2 else_statement_empty_2 ENDIF ';'    { $$ = StmtNode::If_ElifElse($3, $6, $7, $8); }
             ;

else_statement_empty_1 : %empty    { $$ = nullptr; }
                       | ELSE statement { $$ = StmtNode::Else($2); }
                       ;

else_statement_empty_2 : %empty                { $$ = nullptr; }
                       | ELSE ':' statement_list    { $$ = StmtNode::Else($3); }
                       ;

elseif_statements_1 : elseif_statement_1                        { $$ = StmtNode::StmtList($1); }
                    | elseif_statements_1 elseif_statement_1    { $$ = StmtNode::AppendToStmtList($1, $2); }
                    ;

elseif_statement_1 : ELSEIF '(' expression ')' statement    { $$ = StmtNode::ElseIf($3, $5); }
              ;

elseif_statements_2 : elseif_statement_2                    { $$ = StmtNode::StmtList($1); }
                | elseif_statements_2 elseif_statement_2    { $$ = StmtNode::AppendToStmtList($1, $2); }
                ;

elseif_statement_2 : ELSEIF '(' expression ')' ':' statement_list   { $$ = StmtNode::ElseIf($3, $6); }
                ;

switch_statement : SWITCH '(' expression ')' '{' case_statements_empty '}'          { $$ = StmtNode::Switch($3, $6); }
              | SWITCH '(' expression ')' ':' case_statements_empty ENDSWITCH ';'   { $$ = StmtNode::Switch($3, $6); }
              ;

case_statements_empty : %empty     { $$ = nullptr; }
                    | case_statements   { $$ = $1; }
                    ;

case_statements : case_statement                { $$ = StmtNode::StmtList($1); }
             | case_statements case_statement   { $$ = StmtNode::AppendToStmtList($1, $2); }
             ;

case_statement : CASE expression ':' statement_list_empty   { $$ = StmtNode::Case($2, $4); }
             | CASE expression ';' statement_list_empty     { $$ = StmtNode::Case($2, $4); }
             | DEFAULT ':' statement_list_empty             { $$ = StmtNode::CaseDefault($3); }
             | DEFAULT ';' statement_list_empty             { $$ = StmtNode::CaseDefault($3); }
             ;

function_definition : function_definition_header '{' statement_list_empty '}'   { $$ = DeclNode::FunctionAddBody($1, $3); }
                ;

function_definition_header : FUNCTION ID '(' parameter_function_list_empty ')'              { $$ = DeclNode::FunctionDecl($2, $4); }
                      | FUNCTION ID '(' parameter_function_list_empty ')' ':' type_list     { $$ = DeclNode::FunctionDecl($2, $4, $7); }
                      ;

type_list : type                { $$ = ValueNode::ValueList($1); }
          | type_list '|' type  { $$ = ValueNode::AppendToValueList($1, $3); }
          ;

type : TYPE { $$ = ValueNode::CreateType($1); }
     | ID   { $$ = ValueNode::CreateIdentifier($1); }
     | NIL  { $$ = ValueNode::CreateTypeNull(); }
     ;

parameter_function_list_empty : %empty         { $$ = nullptr; }
                         | parameter_function_list  { $$ = $1; }
                         ;

parameter_function_list : parameter_function                        { $$ = DeclNode::DeclList($1); }
                   | parameter_function_list ',' parameter_function { $$ = DeclNode::AppendToDeclList($1, $3); }
                   ;

parameter_function : '$' ID                         { $$ = DeclNode::ParamDecl($2); }
                | '$' ID '=' expression             { $$ = DeclNode::ParamDeclExpr($2, $4); }
                | type_list '$' ID                  { $$ = DeclNode::ParamDeclType($3, $1); }
                | type_list '$' ID '=' expression   { $$ = DeclNode::ParamDeclExprType($3, $5, $1); }
                ;


class_declaration : CLASS ID '{' class_member_declarations_empty '}'            { $$ = DeclNode::ClassDecl($2, $4); }
               | CLASS ID EXTENDS ID '{' class_member_declarations_empty '}'    { $$ = DeclNode::ClassDecl($2, $4, $6); }
               ;

class_member_declarations_empty : %empty           { $$ = nullptr; }
                           | class_member_declarations  { $$ = $1; }
                           ;

class_member_declarations : class_member_declaration                        { $$ = DeclNode::DeclList($1); }
                     | class_member_declarations class_member_declaration   { $$ = DeclNode::AppendToDeclList($1, $2); }
                     ;

class_member_declaration : class_const_elements { $$ = $1; }
                    | property_declaration      { $$ = $1; }
                    | method_declaration        { $$ = $1; }
                    ;

class_const_elements : CONST const_elements ';'                     { $$ = $2; }
                 | visibility_modifiers CONST const_elements ';'    { $$ = DeclNode::SetModsToDecl($3, $1); }
                 ;

const_elements : const_element                      { $$ = DeclNode::DeclList($1); }
             | const_elements ',' const_element     { $$ = DeclNode::AppendToDeclList($1, $3); }
             ;

const_element : ID '=' expression   { $$ = DeclNode::ConstDecl($1, $3); }
            ;

visibility_modifiers : PUBLIC       { $$ = RawDeclModifier::VisibilityMod(VisibilityType::VISIBILITY_PUBLIC); }
                     | PROTECTED    { $$ = RawDeclModifier::VisibilityMod(VisibilityType::VISIBILITY_PROTECTED); }
                     | PRIVATE      { $$ = RawDeclModifier::VisibilityMod(VisibilityType::VISIBILITY_PRIVATE); }
                     ;

declaration_modifiers : visibility_modifiers        { $$ = $1; }
                      | visibility_modifiers STATIC { $$ = RawDeclModifier::StaticMod($1, true); }
                      | STATIC visibility_modifiers { $$ = RawDeclModifier::StaticMod($2, true); }
                      | STATIC                      { $$ = RawDeclModifier::StaticMod(true); }
                      ;

property_declaration : VAR property_elements ';'                { $$ = $2; }
                 | declaration_modifiers property_elements ';'  { $$ = DeclNode::SetModsToDecl($2, $1); }
                 ;

property_elements : property_element                    { $$ = DeclNode::DeclList($1); }
               | property_elements ',' property_element { $$ = DeclNode::AppendToDeclList($1, $3); }
               ;

property_element : '$' ID               { $$ = DeclNode::PropertyDecl($2); }
              | '$' ID '=' expression   { $$ = DeclNode::PropertyDecl($2, $4); }
              ;

method_declaration : function_definition                    { $$ = $1; }
                | declaration_modifiers function_definition { $$ = DeclNode::SetModsToDecl($2, $1); }
                ;

string: STRING                                                              { $$ = ExprNode::String($1); }
      | INTERPOLATABLE_START interpolatable_elements INTERPOLATABLE_END     { $$ = $2; }
      ;

interpolatable_elements: interpolatable_element                             { $$ = ExprNode::ComplexString($1); }
                       | interpolatable_elements interpolatable_element     { $$ = ExprNode::AppendToExprList($1, $2); }
                       ;

interpolatable_element: STRING { $$ = ExprNode::String($1); }
                      | SIMPLE_INTERPOLATION_START simple_interpolated_expression INTERPOLATION_END     { $$ = $2; }
                      | COMPLEX_INTERPOLATION_START complex_interpolated_expression INTERPOLATION_END   { $$ = $2; }
                      ;

simple_interpolated_expression : '$' ID PROPERTY_ACCESS ID  { $$ = ExprNode::PropertyAccess(ExprNode::Id($2), ExprNode::Id($4)); }
                               | '$' ID '[' INT ']'         { $$ = ExprNode::ArrayIndex(ExprNode::Id($2), ExprNode::Int($4)); }
                               | '$' ID '[' '$' ID ']'      { $$ = ExprNode::ArrayIndex(ExprNode::Id($2), ExprNode::Id($5)); }
                               | '$' ID                     { $$ = ExprNode::Sigil(ExprNode::Id($2)); }
                               ;

complex_interpolated_expression : complex_interpolated_expression PROPERTY_ACCESS complex_interpolated_expression   { $$ = ExprNode::PropertyAccess($1, $3); }
                  | complex_interpolated_expression STATIC_PROPERTY_ACCESS complex_interpolated_expression          { $$ = ExprNode::StaticPropertyAccess($1, $3); }
                  | complex_interpolated_expression '[' expression ']'                  { $$ = ExprNode::ArrayIndex($1, $3); }
                  | complex_interpolated_expression '(' expression_list_empty ')'       { $$ = ExprNode::FunctionCall($1, $3); }
                  | '$' ID                                                              { $$ = ExprNode::Sigil(ExprNode::Id($2)); }
                  ;

%%

void yyerror(const char* message) {
    Console::ParserError(message);;
}
