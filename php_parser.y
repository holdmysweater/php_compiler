%code requires {
    #include <string.h>
    #include <memory>
    #include "core/helpers/Console.h"
    #include "core/nodes/ElementNode.h"

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
%token <string_const> STRING ID TYPE

%type <char_const> error
%type <elementNode> program php_program_list php_program_element
%type <declNode> function_definition class_declaration function_definition_header parameter_function_list_empty parameter_function_list parameter_function class_member_declarations_empty class_member_declarations class_member_declaration class_const_elements property_declaration method_declaration const_elements const_element property_elements property_element
%type <stmtNode> statement try_statement catch_list catch_clause finally_clause while_statement for_statement statement_list_empty foreach_statement if_statement switch_statement statement_list else_statement_empty_1 elseif_statements_1 else_statement_empty_2 elseif_statements_2 elseif_statement_1 elseif_statement_2 case_statements_empty case_statements case_statement
%type <exprNode> expression array_element array_element_list expression_list expression_list_empty string interpolatable_elements interpolatable_element simple_interpolated_expression complex_interpolated_expression expression_variable
%type <valueNode> type_list type id_list
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
%token INTERPOLATABLE_START
%token INTERPOLATABLE_END
%token SIMPLE_INTERPOLATION_START
%token COMPLEX_INTERPOLATION_START
%token INTERPOLATION_END
%token KEY_ACCESS
%token ARRAY
%token SPREAD_OPERATOR
%token TRY
%token CATCH
%token THROW
%token FINALLY

%left OR
%left AND
%left XOR
%right '='  MULT_ASSIGN POW_ASSIGN DIV_ASSIGN PLUS_ASSIGN MINUS_ASSIGN CONCAT_ASSIGN LEFT_SHIFT_ASSIGN RIGHT_SHIFT_ASSIGN MOD_ASSIGN
%left '?' ':'
%left NULL_COALESCING
%left LOGIC_OR
%left LOGIC_AND
%left '|'
%left '^'
%left '&'
%nonassoc EQUAL NOT_EQUAL IDENTICALLY_EQUAL IDENTICALLY_NOT_EQUAL NOT_EQUAL_BITWISE
%nonassoc '<' LESS_OR_EQUAL '>' GREAT_OR_EQUAL SPACESHIP
%left LEFT_SHIFT RIGHT_SHIFT
%left '+' '-' '.'
%left '*' '/' '%'
%right POW
%precedence '!' '~'
%precedence UMINUS UPLUS
%nonassoc INSTANCEOF
%nonassoc INCREMENT DECREMENT
%left '[' PROPERTY_ACCESS STATIC_PROPERTY_ACCESS
%precedence '('
%precedence NEW

%%

program : %empty                { Console::ParserLog("program (empty)"); $$ = root = ElementNode::EmptyElement(); }
        | php_program_list      { Console::ParserLog("program (mixed_content_list)"); $$ = root = $1; }
        | error                 { Console::ParserLog("program (error)"); Console::ParserError(); $$ = root = nullptr; }
        | ERROR                 { Console::ParserLog("program (ERROR)"); Console::ParserError("lexer returned an error"); $$ = root = nullptr; }
        ;

php_program_list : php_program_element                  { Console::ParserLog("php_program_list (php_program_element)"); $$ = ElementNode::ElementList($1); }
                 | php_program_list php_program_element { Console::ParserLog("php_program_list (php_program_list php_program_element)"); $$ = ElementNode::AppendToElementList($1, $2); }
                 ;

php_program_element : statement             { Console::ParserLog("php_program_element (statement)"); $$ = ElementNode::PhpStmt($1); }
                    | function_definition   { Console::ParserLog("php_program_element (function_definition)"); $$ = ElementNode::PhpFuncDecl($1); }
                    | class_declaration     { Console::ParserLog("php_program_element (class_declaration)"); $$ = ElementNode::PhpClassDecl($1); }
                    ;

expression : expression LOGIC_OR expression                 { Console::ParserLog("expression (expression LOGIC_OR expression)"); $$ = ExprNode::Or($1, $3); }
          | expression OR expression                        { Console::ParserLog("expression (expression OR expression)"); $$ = ExprNode::OrLower($1, $3); }
          | expression XOR expression                       { Console::ParserLog("expression (expression XOR expression)"); $$ = ExprNode::Xor($1, $3); }
          | expression '^' expression                       { Console::ParserLog("expression (expression '^' expression)"); $$ = ExprNode::XorBitwise($1, $3); }
          | expression LOGIC_AND expression                 { Console::ParserLog("expression (expression LOGIC_AND expression)"); $$ = ExprNode::And($1, $3); }
          | expression AND expression                       { Console::ParserLog("expression (expression AND expression)"); $$ = ExprNode::AndLower($1, $3); }
          | expression '|' expression                       { Console::ParserLog("expression (expression '|' expression)"); $$ = ExprNode::OrBitwise($1, $3); }
          | expression '&' expression                       { Console::ParserLog("expression (expression '&' expression)"); $$ = ExprNode::AndBitwise($1, $3); }
          | expression '=' expression                       { Console::ParserLog("expression (expression '=' expression)"); $$ = ExprNode::Assign($1, $3); }
          | expression '?' expression ':' expression        { Console::ParserLog("expression (expression '?' expression ':' expression)"); $$ = ExprNode::Ternary($1, $3, $5); }
          | expression MULT_ASSIGN expression               { Console::ParserLog("expression (expression MULT_ASSIGN expression)"); $$ = ExprNode::MultAssign($1, $3); }
          | expression POW_ASSIGN expression                { Console::ParserLog("expression (expression POW_ASSIGN expression)"); $$ = ExprNode::PowAssign($1, $3); }
          | expression DIV_ASSIGN expression                { Console::ParserLog("expression (expression DIV_ASSIGN expression)"); $$ = ExprNode::DivAssign($1, $3); }
          | expression MOD_ASSIGN expression                { Console::ParserLog("expression (expression MOD_ASSIGN expression)"); $$ = ExprNode::ModAssign($1, $3); }
          | expression PLUS_ASSIGN expression               { Console::ParserLog("expression (expression PLUS_ASSIGN expression)"); $$ = ExprNode::PlusAssign($1, $3); }
          | expression MINUS_ASSIGN expression              { Console::ParserLog("expression (expression MINUS_ASSIGN expression)"); $$ = ExprNode::MinusAssign($1, $3); }
          | expression CONCAT_ASSIGN expression             { Console::ParserLog("expression (expression CONCAT_ASSIGN expression)"); $$ = ExprNode::ConcatAssign($1, $3); }
          | expression LEFT_SHIFT_ASSIGN expression         { Console::ParserLog("expression (expression LEFT_SHIFT_ASSIGN expression)"); $$ = ExprNode::LeftShiftAssign($1, $3); }
          | expression RIGHT_SHIFT_ASSIGN expression        { Console::ParserLog("expression (expression RIGHT_SHIFT_ASSIGN expression)"); $$ = ExprNode::RightShiftAssign($1, $3); }
          | expression EQUAL expression                     { Console::ParserLog("expression (expression EQUAL expression)"); $$ = ExprNode::Equal($1, $3); }
          | expression NOT_EQUAL expression                 { Console::ParserLog("expression (expression NOT_EQUAL expression)"); $$ = ExprNode::NotEqual($1, $3); }
          | expression NOT_EQUAL_BITWISE expression         { Console::ParserLog("expression (expression NOT_EQUAL_BITWISE expression)"); $$ = ExprNode::NotEqualBitwise($1, $3); }
          | expression IDENTICALLY_EQUAL expression         { Console::ParserLog("expression (expression IDENTICALLY_EQUAL expression)"); $$ = ExprNode::IdenticallyEqual($1, $3); }
          | expression IDENTICALLY_NOT_EQUAL expression     { Console::ParserLog("expression (expression IDENTICALLY_NOT_EQUAL expression)"); $$ = ExprNode::IdenticallyNotEqual($1, $3); }
          | expression '<' expression                       { Console::ParserLog("expression (expression '<' expression)"); $$ = ExprNode::LessThan($1, $3); }
          | expression '>' expression                       { Console::ParserLog("expression (expression '>' expression)"); $$ = ExprNode::GreaterThan($1, $3); }
          | expression LESS_OR_EQUAL expression             { Console::ParserLog("expression (expression LESS_OR_EQUAL expression)"); $$ = ExprNode::LessOrEqual($1, $3); }
          | expression GREAT_OR_EQUAL expression            { Console::ParserLog("expression (expression GREAT_OR_EQUAL expression)"); $$ = ExprNode::GreatOrEqual($1, $3); }
          | expression SPACESHIP expression                 { Console::ParserLog("expression (expression SPACESHIP expression)"); $$ = ExprNode::Spaceship($1, $3); }
          | expression NULL_COALESCING expression           { Console::ParserLog("expression (expression NULL_COALESCING expression)"); $$ = ExprNode::NullCoaslescing($1, $3); }
          | expression LEFT_SHIFT expression                { Console::ParserLog("expression (expression LEFT_SHIFT expression)"); $$ = ExprNode::LeftShift($1, $3); }
          | expression RIGHT_SHIFT expression               { Console::ParserLog("expression (expression RIGHT_SHIFT expression)"); $$ = ExprNode::RightShift($1, $3); }
          | expression '+' expression                       { Console::ParserLog("expression (expression '+' expression)"); $$ = ExprNode::Add($1, $3); }
          | expression '-' expression                       { Console::ParserLog("expression (expression '-' expression)"); $$ = ExprNode::Subtract($1, $3); }
          | expression '.' expression                       { Console::ParserLog("expression (expression '.' expression)"); $$ = ExprNode::Concat($1, $3); }
          | expression '*' expression                       { Console::ParserLog("expression (expression '*' expression)"); $$ = ExprNode::Mult($1, $3); }
          | expression '%' expression                       { Console::ParserLog("expression (expression '%' expression)"); $$ = ExprNode::Mod($1, $3); }
          | expression '/' expression                       { Console::ParserLog("expression (expression '/' expression)"); $$ = ExprNode::Div($1, $3); }
          | expression INSTANCEOF expression                { Console::ParserLog("expression (expression INSTANCEOF expression)"); $$ = ExprNode::Instanceof($1, $3); }
          | expression POW expression                       { Console::ParserLog("expression (expression POW expression)"); $$ = ExprNode::Pow($1, $3); }
          | expression PROPERTY_ACCESS expression_variable  { Console::ParserLog("expression (expression PROPERTY_ACCESS expression)"); $$ = ExprNode::PropertyAccess($1, $3); }
          | expression PROPERTY_ACCESS expression_variable '(' expression_list_empty ')'    { Console::ParserLog("expression (expression PROPERTY_ACCESS expression_variable '(' expression_list_empty ')')"); $$ = ExprNode::MethodAccess($1, $3, $5); }
          | expression STATIC_PROPERTY_ACCESS expression_variable    { Console::ParserLog("expression (expression STATIC_PROPERTY_ACCESS expression)"); $$ = ExprNode::StaticPropertyAccess($1, $3); }
          | expression INCREMENT                            { Console::ParserLog("expression (expression INCREMENT)"); $$ = ExprNode::IncrementPost($1); }
          | expression DECREMENT                            { Console::ParserLog("expression (expression DECREMENT)"); $$ = ExprNode::DecrementPost($1); }
          | INCREMENT expression                            { Console::ParserLog("expression (INCREMENT expression)"); $$ = ExprNode::IncrementPre($2); }
          | DECREMENT expression                            { Console::ParserLog("expression (DECREMENT expression)"); $$ = ExprNode::DecrementPre($2); }
          | '!' expression                                  { Console::ParserLog("expression ('!' expression)"); $$ = ExprNode::Not($2); }
          | '~' expression                                  { Console::ParserLog("expression ('~' expression)"); $$ = ExprNode::NotBitwise($2); }
          | '+' expression %prec UPLUS                      { Console::ParserLog("expression ('+' expression %prec UPLUS)"); $$ = ExprNode::Uplus($2); }
          | '-' expression %prec UMINUS                     { Console::ParserLog("expression ('-' expression %prec UMINUS)"); $$ = ExprNode::Uminus($2); }
          | '[' array_element_list ']'                      { Console::ParserLog("expression ('[' array_element_list ']')"); $$ = ExprNode::ArrayElementList($2); }
          | '[' ']'                                         { Console::ParserLog("expression ('[' ']')"); $$ = ExprNode::Array(); }
          | expression '[' expression ']'                   { Console::ParserLog("expression (expression '[' expression ']')"); $$ = ExprNode::ArrayIndex($1, $3); }
          | expression '[' ']' '=' expression               { Console::ParserLog("expression (expression '[' ']' '=' expression )"); $$ = ExprNode::ArrayAppend($1, $5); }
          | '(' expression_list_empty ')'                   { Console::ParserLog("expression ('(' expression_list_empty ')')"); $$ = ExprNode::Parenthesized($2); }
          | expression '(' expression_list_empty ')'        { Console::ParserLog("expression (expression '(' expression_list_empty ')')"); $$ = ExprNode::FunctionCall($1, $3); }
          | ARRAY '(' array_element_list ')'                { Console::ParserLog("expression (ARRAY '(' array_element_list ')')"); $$ = ExprNode::ArrayElementList($3); }
          | ARRAY '(' ')'                                   { Console::ParserLog("expression (ARRAY '(' ')')"); $$ = ExprNode::Array(); }
          | NEW expression  { Console::ParserLog("expression (NEW expression)"); $$ = ExprNode::New($2); }
          | string          { Console::ParserLog("expression (string)"); $$ = $1; }
          | expression_variable { Console::ParserLog("expression (expression_variable)"); $$ = $1; }
          | INT             { Console::ParserLog("expression (INT)"); $$ = ExprNode::Int($1); }
          | FLOAT           { Console::ParserLog("expression (FLOAT)"); $$ = ExprNode::Float($1); }
          | BOOL            { Console::ParserLog("expression (BOOL)"); $$ = ExprNode::Bool($1); }
          | NIL             { Console::ParserLog("expression (NIL)"); $$ = ExprNode::Nil(); }
          ;

expression_variable : '$' expression_variable { Console::ParserLog("expression_variable ('$' expression_variable)"); $$ = ExprNode::Sigil($2); }
                    | ID    { Console::ParserLog("expression_variable (ID)"); $$ = ExprNode::Id($1); }
                    ;


array_element_list : array_element                          { Console::ParserLog("array_element_list (array_element)"); $$ = ExprNode::ExprList($1); }
                   | array_element_list ',' array_element   { Console::ParserLog("array_element_list (array_element_list ',' array_element)"); $$ = ExprNode::AppendToExprList($1, $3); }
                   ;

array_element : expression KEY_ACCESS expression    { Console::ParserLog("array_element (expression KEY_ACCESS expression)"); $$ = ExprNode::ArrayKeyAccess($1, $3); }
              | expression                          { Console::ParserLog("array_element (expression)"); $$ = $1; }
              | SPREAD_OPERATOR expression          { Console::ParserLog("array_element (SPREAD_OPERATOR expression)"); $$ = ExprNode::SpreadArray($2); }
              ;

expression_list : expression                        { Console::ParserLog("expression_list (expression)"); $$ = ExprNode::ExprList($1); }
                | expression_list ',' expression    { Console::ParserLog("expression_list (expression_list ',' expression)"); $$ = ExprNode::AppendToExprList($1, $3); }
                ;

expression_list_empty : %empty          { Console::ParserLog("expression_list_empty (empty)"); $$ = nullptr; }
                      | expression_list { Console::ParserLog("expression_list_empty (expression_list)"); $$ = $1; }
                      ;


statement : '{' statement_list_empty '}'    { Console::ParserLog("statement ('{' statement_list_empty '}')"); $$ = $2; }
          | expression ';'                  { Console::ParserLog("statement (expression ';')"); $$ = StmtNode::ExprStmt($1); }
          | try_statement                   { Console::ParserLog("statement (try_statement)"); $$ = $1; }
          | while_statement                 { Console::ParserLog("statement (while_statement)"); $$ = $1; }
          | for_statement                   { Console::ParserLog("statement (for_statement)"); $$ = $1; }
          | foreach_statement               { Console::ParserLog("statement (foreach_statement)"); $$ = $1; }
          | if_statement                    { Console::ParserLog("statement (if_statement)"); $$ = $1; }
          | switch_statement                { Console::ParserLog("statement (switch_statement)"); $$ = $1; }
          | ECHO_KW expression_list ';'     { Console::ParserLog("statement (ECHO_KW expression_list ';')"); $$ = StmtNode::Echo($2); }
          | RETURN expression ';'           { Console::ParserLog("statement (RETURN expression ';')"); $$ = StmtNode::ReturnStmt($2); }
          | THROW expression ';'            { Console::ParserLog("statement (THROW expression ';')"); $$ = StmtNode::ThrowStmt($2); }
          | RETURN ';'                      { Console::ParserLog("statement (RETURN ';')"); $$ = StmtNode::ReturnStmt(); }
          | BREAK ';'                       { Console::ParserLog("statement (BREAK ';')"); $$ = StmtNode::BreakStmt(); }
          | CONTINUE ';'                    { Console::ParserLog("statement (CONTINUE ';')"); $$ = StmtNode::ContinueStmt(); }
          | ';'                             { Console::ParserLog("statement (';')"); $$ = nullptr; }
          ;

statement_list : statement                  { Console::ParserLog("statement_list (statement)"); $$ = StmtNode::StmtList($1); }
               | statement_list statement   { Console::ParserLog("statement_list (statement_list statement)"); $$ = StmtNode::AppendToStmtList($1, $2); }
               ;

statement_list_empty : %empty           { Console::ParserLog("statement_list_empty (empty)"); $$ = nullptr; }
                     | statement_list   { Console::ParserLog("statement_list_empty (statement_list)"); $$ = $1; }
                     ;

try_statement : TRY '{' statement_list_empty '}' catch_list                 { Console::ParserLog("try_statement (TRY '{' statement_list_empty '}' catch_list)"); $$ = StmtNode::TryCatchStmt($3, $5); }
              | TRY '{' statement_list_empty '}' finally_clause             { Console::ParserLog("try_statement (TRY '{' statement_list_empty '}' finally_clause)"); $$ = StmtNode::TryFinallyStmt($3, $5); }
              | TRY '{' statement_list_empty '}' catch_list finally_clause  { Console::ParserLog("try_statement (TRY '{' statement_list_empty '}' catch_list finally_clause)"); $$ = StmtNode::TryCatchFinallyStmt($3, $5, $6); }
              ;

catch_list : catch_clause               { Console::ParserLog("catch_list (catch_clause)"); $$ = StmtNode::StmtList($1); }
           | catch_list catch_clause    { Console::ParserLog("catch_list (catch_list catch_clause)"); $$ = StmtNode::AppendToStmtList($1, $2); }
           ;

catch_clause : CATCH '(' id_list '$' ID ')' '{' statement_list_empty '}'     { Console::ParserLog("catch_clause (CATCH '(' ID '$' ID ')' '{' statement_list_empty '}')"); $$ = StmtNode::CatchStmt($8, $3, $5); }
             ;

id_list : ID                { Console::ParserLog("id_list (ID)"); $$ = ValueNode::ValueList(ValueNode::CreateIdentifier($1)); }
        | id_list '|' ID    { Console::ParserLog("id_list (id_list '|' ID)"); $$ = ValueNode::AppendToValueList($1, ValueNode::CreateIdentifier($3)); }
        ;

finally_clause : FINALLY '{' statement_list_empty '}'   { Console::ParserLog("finally_clause (FINALLY '{' statement_list_empty '}')"); $$ = StmtNode::FinallyStmt($3); }
               ;

while_statement : WHILE '(' expression ')' statement                        { Console::ParserLog("while_statement (WHILE '(' expression ')' statement)"); $$ = StmtNode::While($3, $5); }
                | WHILE '(' expression ')' ':' statement_list ENDWHILE ';'  { Console::ParserLog("while_statement (WHILE '(' expression ')' ':' statement_list ENDWHILE ';')"); $$ = StmtNode::While($3, $6); }
                | DO statement WHILE '(' expression ')' ';'                 { Console::ParserLog("while_statement (DO statement WHILE '(' expression ')' ';')"); $$ = StmtNode::DoWhile($5, $2); }
                ;

for_statement : FOR '(' expression_list_empty ';' expression_list_empty ';' expression_list_empty ')' statement                     { Console::ParserLog("for_statement (FOR '(' expression_list_empty ';' expression_list_empty ';' expression_list_empty ')' statement)"); $$ = StmtNode::For($3, $5, $7, $9); }
              | FOR '(' expression_list_empty ';' expression_list_empty ';' expression_list_empty ')' ':' statement_list ENDFOR ';' { Console::ParserLog("for_statement (FOR '(' expression_list_empty ';' expression_list_empty ';' expression_list_empty ')' ':' statement_list ENDFOR ';')"); $$ = StmtNode::For($3, $5, $7, $10); }
              ;

foreach_statement : FOREACH '(' expression AS '$' ID KEY_ACCESS '$' ID ')' statement                            { Console::ParserLog("foreach_statement (FOREACH '(' expression AS '$' ID KEY_ACCESS '$' ID ')' statement)"); $$ = StmtNode::ForEachKeyValue($3, ExprNode::Id($6), ExprNode::Id($9), $11); }
                  | FOREACH '(' expression AS '$' ID ')' statement                                              { Console::ParserLog("foreach_statement (FOREACH '(' expression AS '$' ID ')' statement)"); $$ = StmtNode::ForEachSimple($3, ExprNode::Id($6), $8); }
                  | FOREACH '(' expression AS '$' ID KEY_ACCESS '$' ID ')' ':' statement_list ENDFOREACH ';'    { Console::ParserLog("foreach_statement (FOREACH '(' expression AS '$' ID KEY_ACCESS '$' ID ')' ':' statement_list ENDFOREACH ';')"); $$ = StmtNode::ForEachKeyValue($3, ExprNode::Id($6), ExprNode::Id($9), $12); }
                  | FOREACH '(' expression AS '$' ID ')' ':' statement_list ENDFOREACH ';'                      { Console::ParserLog("foreach_statement (FOREACH '(' expression AS '$' ID ')' ':' statement_list ENDFOREACH ';')"); $$ = StmtNode::ForEachSimple($3, ExprNode::Id($6), $9); }
                  ;

if_statement : IF '(' expression ')' statement else_statement_empty_1                                           { Console::ParserLog("if_statement (IF '(' expression ')' statement else_statement_empty_1)"); $$ = StmtNode::If_Else($3, $5, $6); }
             | IF '(' expression ')' statement elseif_statements_1 else_statement_empty_1                       { Console::ParserLog("if_statement (IF '(' expression ')' statement elseif_statements_1 else_statement_empty_1)"); $$ = StmtNode::If_ElifElse($3, $5, $6, $7); }
             | IF '(' expression ')' ':' statement_list else_statement_empty_2 ENDIF ';'                        { Console::ParserLog("if_statement (IF '(' expression ')' ':' statement_list else_statement_empty_2 ENDIF ';')"); $$ = StmtNode::If_Else($3, $6, $7); }
             | IF '(' expression ')' ':' statement_list elseif_statements_2 else_statement_empty_2 ENDIF ';'    { Console::ParserLog("if_statement (IF '(' expression ')' ':' statement_list elseif_statements_2 else_statement_empty_2 ENDIF ';')"); $$ = StmtNode::If_ElifElse($3, $6, $7, $8); }
             ;

else_statement_empty_1 : %empty         { Console::ParserLog("else_statement_empty_1 (empty)"); $$ = nullptr; }
                       | ELSE statement { Console::ParserLog("else_statement_empty_1 (ELSE statement)"); $$ = StmtNode::Else($2); }
                       ;

else_statement_empty_2 : %empty                     { Console::ParserLog("else_statement_empty_2 (empty)"); $$ = nullptr; }
                       | ELSE ':' statement_list    { Console::ParserLog("else_statement_empty_2 (ELSE ':' statement_list)"); $$ = StmtNode::Else($3); }
                       ;

elseif_statements_1 : elseif_statement_1                        { Console::ParserLog("elseif_statements_1 (elseif_statement_1)"); $$ = StmtNode::StmtList($1); }
                    | elseif_statements_1 elseif_statement_1    { Console::ParserLog("elseif_statements_1 (elseif_statements_1 elseif_statement_1)"); $$ = StmtNode::AppendToStmtList($1, $2); }
                    ;

elseif_statement_1 : ELSEIF '(' expression ')' statement    { Console::ParserLog("elseif_statement_1 (ELSEIF '(' expression ')' statement)"); $$ = StmtNode::ElseIf($3, $5); }
              ;

elseif_statements_2 : elseif_statement_2                    { Console::ParserLog("elseif_statements_2 (elseif_statement_2)"); $$ = StmtNode::StmtList($1); }
                | elseif_statements_2 elseif_statement_2    { Console::ParserLog("elseif_statements_2 (elseif_statements_2 elseif_statement_2)"); $$ = StmtNode::AppendToStmtList($1, $2); }
                ;

elseif_statement_2 : ELSEIF '(' expression ')' ':' statement_list   { Console::ParserLog("elseif_statement_2 (ELSEIF '(' expression ')' ':' statement_list)"); $$ = StmtNode::ElseIf($3, $6); }
                ;

switch_statement : SWITCH '(' expression ')' '{' case_statements_empty '}'          { Console::ParserLog("switch_statement (SWITCH '(' expression ')' '{' case_statements_empty '}')"); $$ = StmtNode::Switch($3, $6); }
              | SWITCH '(' expression ')' ':' case_statements_empty ENDSWITCH ';'   { Console::ParserLog("switch_statement (SWITCH '(' expression ')' ':' case_statements_empty ENDSWITCH ';')"); $$ = StmtNode::Switch($3, $6); }
              ;

case_statements_empty : %empty          { Console::ParserLog("case_statements_empty (empty)"); $$ = nullptr; }
                    | case_statements   { Console::ParserLog("case_statements_empty (case_statements)"); $$ = $1; }
                    ;

case_statements : case_statement                { Console::ParserLog("case_statements (case_statement)"); $$ = StmtNode::StmtList($1); }
             | case_statements case_statement   { Console::ParserLog("case_statements (case_statements case_statement)"); $$ = StmtNode::AppendToStmtList($1, $2); }
             ;

case_statement : CASE expression ':' statement_list_empty   { Console::ParserLog("case_statement (CASE expression ':' statement_list_empty)"); $$ = StmtNode::Case($2, $4); }
             | CASE expression ';' statement_list_empty     { Console::ParserLog("case_statement (CASE expression ';' statement_list_empty)"); $$ = StmtNode::Case($2, $4); }
             | DEFAULT ':' statement_list_empty             { Console::ParserLog("case_statement (DEFAULT ':' statement_list_empty)"); $$ = StmtNode::CaseDefault($3); }
             | DEFAULT ';' statement_list_empty             { Console::ParserLog("case_statement (DEFAULT ';' statement_list_empty)"); $$ = StmtNode::CaseDefault($3); }
             ;

function_definition : function_definition_header '{' statement_list_empty '}'   { Console::ParserLog("function_definition (function_definition_header '{' statement_list_empty '}')"); $$ = DeclNode::FunctionAddBody($1, $3); }
                ;

function_definition_header : FUNCTION ID '(' parameter_function_list_empty ')'              { Console::ParserLog("function_definition_header (FUNCTION ID '(' parameter_function_list_empty ')')"); $$ = DeclNode::FunctionDecl($2, $4); }
                      | FUNCTION ID '(' parameter_function_list_empty ')' ':' type_list     { Console::ParserLog("function_definition_header (FUNCTION ID '(' parameter_function_list_empty ')' ':' type_list)"); $$ = DeclNode::FunctionDecl($2, $4, $7); }
                      ;

type_list : type                { Console::ParserLog("type_list (type)"); $$ = ValueNode::ValueList($1); }
          | type_list '|' type  { Console::ParserLog("type_list (type_list '|' type)"); $$ = ValueNode::AppendToValueList($1, $3); }
          ;

type : TYPE { Console::ParserLog("type (TYPE)"); $$ = ValueNode::CreateType($1); }
     | ID   { Console::ParserLog("type (ID)"); $$ = ValueNode::CreateIdentifier($1); }
     | NIL  { Console::ParserLog("type (NIL)"); $$ = ValueNode::CreateTypeNull(); }
     | ARRAY  { Console::ParserLog("type (ARRAY)"); $$ = ValueNode::CreateTypeArray(); }
     ;

parameter_function_list_empty : %empty              { Console::ParserLog("parameter_function_list_empty (empty)"); $$ = nullptr; }
                         | parameter_function_list  { Console::ParserLog("parameter_function_list_empty (parameter_function_list)"); $$ = $1; }
                         ;

parameter_function_list : parameter_function                        { Console::ParserLog("parameter_function_list (parameter_function)"); $$ = DeclNode::DeclList($1); }
                   | parameter_function_list ',' parameter_function { Console::ParserLog("parameter_function_list (parameter_function_list ',' parameter_function)"); $$ = DeclNode::AppendToDeclList($1, $3); }
                   ;

parameter_function : '$' ID                         { Console::ParserLog("parameter_function ('$' ID)"); $$ = DeclNode::ParamDecl($2); }
                | '$' ID '=' expression             { Console::ParserLog("parameter_function ('$' ID '=' expression)"); $$ = DeclNode::ParamDeclExpr($2, $4); }
                | type_list '$' ID                  { Console::ParserLog("parameter_function (type_list '$' ID)"); $$ = DeclNode::ParamDeclType($3, $1); }
                | type_list '$' ID '=' expression   { Console::ParserLog("parameter_function (type_list '$' ID '=' expression)"); $$ = DeclNode::ParamDeclExprType($3, $5, $1); }
                ;


class_declaration : CLASS ID '{' class_member_declarations_empty '}'            { Console::ParserLog("class_declaration (CLASS ID '{' class_member_declarations_empty '}')"); $$ = DeclNode::ClassDecl($2, $4); }
               | CLASS ID EXTENDS ID '{' class_member_declarations_empty '}'    { Console::ParserLog("class_declaration (CLASS ID EXTENDS ID '{' class_member_declarations_empty '}')"); $$ = DeclNode::ClassDecl($2, $4, $6); }
               ;

class_member_declarations_empty : %empty                { Console::ParserLog("class_member_declarations_empty (empty)"); $$ = nullptr; }
                           | class_member_declarations  { Console::ParserLog("class_member_declarations_empty (class_member_declarations)"); $$ = $1; }
                           ;

class_member_declarations : class_member_declaration                        { Console::ParserLog("class_member_declarations (class_member_declaration)"); $$ = DeclNode::DeclList($1); }
                     | class_member_declarations class_member_declaration   { Console::ParserLog("class_member_declarations (class_member_declarations class_member_declaration)"); $$ = DeclNode::AppendToDeclList($1, $2); }
                     ;

class_member_declaration : class_const_elements { Console::ParserLog("class_member_declaration (class_const_elements)"); $$ = $1; }
                    | property_declaration      { Console::ParserLog("class_member_declaration (property_declaration)"); $$ = $1; }
                    | method_declaration        { Console::ParserLog("class_member_declaration (method_declaration)"); $$ = $1; }
                    ;

class_const_elements : CONST const_elements ';'                     { Console::ParserLog("class_const_elements (CONST const_elements ';')"); $$ = $2; }
                 | visibility_modifiers CONST const_elements ';'    { Console::ParserLog("class_const_elements (visibility_modifiers CONST const_elements ';')"); $$ = DeclNode::SetModsToDecl($3, $1); }
                 ;

const_elements : const_element                      { Console::ParserLog("const_elements (const_element)"); $$ = DeclNode::DeclList($1); }
             | const_elements ',' const_element     { Console::ParserLog("const_elements (const_elements ',' const_element)"); $$ = DeclNode::AppendToDeclList($1, $3); }
             ;

const_element : ID '=' expression   { Console::ParserLog("const_element (ID '=' expression)"); $$ = DeclNode::ConstDecl($1, $3); }
            ;

visibility_modifiers : PUBLIC       { Console::ParserLog("visibility_modifiers (PUBLIC)"); $$ = RawDeclModifier::VisibilityMod(VisibilityType::VISIBILITY_PUBLIC); }
                     | PROTECTED    { Console::ParserLog("visibility_modifiers (PROTECTED)"); $$ = RawDeclModifier::VisibilityMod(VisibilityType::VISIBILITY_PROTECTED); }
                     | PRIVATE      { Console::ParserLog("visibility_modifiers (PRIVATE)"); $$ = RawDeclModifier::VisibilityMod(VisibilityType::VISIBILITY_PRIVATE); }
                     ;

declaration_modifiers : visibility_modifiers        { Console::ParserLog("declaration_modifiers (visibility_modifiers)"); $$ = $1; }
                      | visibility_modifiers STATIC { Console::ParserLog("declaration_modifiers (visibility_modifiers STATIC)"); $$ = RawDeclModifier::StaticMod($1, true); }
                      | STATIC visibility_modifiers { Console::ParserLog("declaration_modifiers (STATIC visibility_modifiers)"); $$ = RawDeclModifier::StaticMod($2, true); }
                      | STATIC                      { Console::ParserLog("declaration_modifiers (STATIC)"); $$ = RawDeclModifier::StaticMod(true); }
                      ;

property_declaration : VAR property_elements ';'                { Console::ParserLog("property_declaration (VAR property_elements ';')"); $$ = $2; }
                 | VAR type_list property_elements ';'          { Console::ParserLog("property_declaration (VAR type_list property_elements ';')"); $$ = DeclNode::SetValueTypeToDecl($3, $2); }
                 | declaration_modifiers property_elements ';'  { Console::ParserLog("property_declaration (declaration_modifiers property_elements ';')"); $$ = DeclNode::SetModsToDecl($2, $1); }
                 | declaration_modifiers type_list property_elements ';'  { Console::ParserLog("property_declaration (declaration_modifiers type_list property_elements ';')"); $$ = DeclNode::SetModsToDecl($3, $1, $2); }
                 ;

property_elements : property_element                    { Console::ParserLog("property_elements (property_element)"); $$ = DeclNode::DeclList($1); }
               | property_elements ',' property_element { Console::ParserLog("property_elements (property_elements ',' property_element)"); $$ = DeclNode::AppendToDeclList($1, $3); }
               ;

property_element : '$' ID               { Console::ParserLog("property_element ('$' ID)"); $$ = DeclNode::PropertyDecl($2); }
              | '$' ID '=' expression   { Console::ParserLog("property_element ('$' ID '=' expression)"); $$ = DeclNode::PropertyDecl($2, $4); }
              ;

method_declaration : function_definition                    { Console::ParserLog("method_declaration (function_definition)"); $$ = DeclNode::SetTypeToDecl($1, DeclType::DT_METHOD); }
                | declaration_modifiers function_definition { Console::ParserLog("method_declaration (declaration_modifiers function_definition)"); $$ = DeclNode::SetTypeToDecl(DeclNode::SetModsToDecl($2, $1), DeclType::DT_METHOD); }
                ;

string: STRING                                                              { Console::ParserLog("string (STRING)"); $$ = ExprNode::String($1); }
      | INTERPOLATABLE_START interpolatable_elements INTERPOLATABLE_END     { Console::ParserLog("string (INTERPOLATABLE_START interpolatable_elements INTERPOLATABLE_END)"); $$ = $2; }
      ;

interpolatable_elements: interpolatable_element                             { Console::ParserLog("interpolatable_elements (interpolatable_element)"); $$ = ExprNode::ComplexString($1); }
                       | interpolatable_elements interpolatable_element     { Console::ParserLog("interpolatable_elements (interpolatable_elements interpolatable_element)"); $$ = ExprNode::AppendToExprList($1, $2); }
                       ;

interpolatable_element: STRING { Console::ParserLog("interpolatable_element (STRING)"); $$ = ExprNode::String($1); }
                      | SIMPLE_INTERPOLATION_START simple_interpolated_expression INTERPOLATION_END     { Console::ParserLog("interpolatable_element (SIMPLE_INTERPOLATION_START simple_interpolated_expression INTERPOLATION_END)"); $$ = $2; }
                      | COMPLEX_INTERPOLATION_START complex_interpolated_expression INTERPOLATION_END   { Console::ParserLog("interpolatable_element (COMPLEX_INTERPOLATION_START complex_interpolated_expression INTERPOLATION_END)"); $$ = $2; }
                      ;

simple_interpolated_expression : '$' ID PROPERTY_ACCESS ID  { Console::ParserLog("simple_interpolated_expression ('$' ID PROPERTY_ACCESS ID)"); $$ = ExprNode::PropertyAccess(ExprNode::Id($2), ExprNode::Id($4)); }
                               | '$' ID '[' INT ']'         { Console::ParserLog("simple_interpolated_expression ('$' ID '[' INT ']')"); $$ = ExprNode::ArrayIndex(ExprNode::Id($2), ExprNode::Int($4)); }
                               | '$' ID '[' '$' ID ']'      { Console::ParserLog("simple_interpolated_expression ('$' ID '[' '$' ID ']')"); $$ = ExprNode::ArrayIndex(ExprNode::Id($2), ExprNode::Id($5)); }
                               | '$' ID                     { Console::ParserLog("simple_interpolated_expression ('$' ID)"); $$ = ExprNode::Sigil(ExprNode::Id($2)); }
                               ;

complex_interpolated_expression : complex_interpolated_expression PROPERTY_ACCESS complex_interpolated_expression   { Console::ParserLog("complex_interpolated_expression (complex_interpolated_expression PROPERTY_ACCESS complex_interpolated_expression)"); $$ = ExprNode::PropertyAccess($1, $3); }
                  | complex_interpolated_expression STATIC_PROPERTY_ACCESS complex_interpolated_expression          { Console::ParserLog("complex_interpolated_expression (complex_interpolated_expression STATIC_PROPERTY_ACCESS complex_interpolated_expression)"); $$ = ExprNode::StaticPropertyAccess($1, $3); }
                  | complex_interpolated_expression '[' expression ']'                  { Console::ParserLog("complex_interpolated_expression (complex_interpolated_expression '[' expression ']')"); $$ = ExprNode::ArrayIndex($1, $3); }
                  | complex_interpolated_expression '(' expression_list_empty ')'       { Console::ParserLog("complex_interpolated_expression (complex_interpolated_expression '(' expression_list_empty ')')"); $$ = ExprNode::FunctionCall($1, $3); }
                  | '(' expression_list_empty ')'                                       { Console::ParserLog("complex_interpolated_expression ('(' expression_list_empty ')')"); $$ = ExprNode::Parenthesized($2); }
                  | expression_variable  { Console::ParserLog("complex_interpolated_expression (expression_variable)"); $$ = $1; }
                  ;

%%

void yyerror(const char* message) {
    Console::ParserError(message);;
}
