%union {
	int int_const;
	float float_const;
	char* string_const;
	char* id_const;
	char* type_const;
}

%token <int_const> INT
%token <int_const> BOOL
%token <float_const> FLOAT
%token <string_const> STRING
%token <id_const> ID
%token <type_const> TYPE

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
%token ECHO
%token IF
%token ELSEIF
%token ELSE
%token ENDIF
%token SWITCH
%token CASE
%token DEFAULT
%token ENDSWITCH
%token FUNCTION
%token INSTANCEOF
%token RETURN
%token STATIC
%token VAR
%token KEY_ACCESS
%token PROPERTY_ACCESS
%token STATIC_PROPERTY_ACCESS
%token START_TAG
%token END_TAG

%left LOGIC_OR_2
%left LOGIC_XOR
%left LOGIC_AND_2
%right '='
%left LOGIC_OR_1
%left LOGIC_AND_1
%nonassoc EQUAL NOT_EQUAL_1 IDENTICALLY_EQUAL IDENTICALLY_NOT_EQUAL NOT_EQUAL_2
%nonassoc '<' LESSER_EQUAL '>' GREATER_EQUAL
%left '+' '-' '.'
%left '*' '/' '%'
%right '!'
%nonassoc INSTANCEOF
%right UMINUS UPLUS
%right POW
%nonassoc NEW
%left KEY_ACCESS
%left '[' PROPERTY_ACCESS STATIC_PROPERTY_ACCESS
%right '$'
%nonassoc '('

%%

program : START_TAG program_list END_TAG
		| START_TAG END_TAG
		| error
		;
		
program_list : program_element
			 | program_list program_element
			 ;
			 
program_element : statement
				| function_definition
				| class_declaration
				;

expression : expression LOGIC_OR_2 expression
		   | expression LOGIC_XOR expression
		   | expression LOGIC_AND_2 expression
		   | expression '=' expression
		   | expression LOGIC_OR_1 expression
		   | expression LOGIC_AND_1 expression
		   | expression EQUAL expression
		   | expression NOT_EQUAL_1 expression
		   | expression NOT_EQUAL_2 expression
		   | expression IDENTICALLY_EQUAL expression
		   | expression IDENTICALLY_NOT_EQUAL expression
		   | expression '<' expression
		   | expression '>' expression
		   | expression LESSER_EQUAL expression
		   | expression GREATER_EQUAL expression
		   | expression '+' expression
		   | expression '-' expression
		   | expression '.' expression
		   | expression '*' expression
		   | expression '%' expression
		   | expression '/' expression
		   | '!' expression
		   | '+' expression %prec UPLUS
		   | '-' expression %prec UMINUS
		   | expression INSTANCEOF expression
		   | expression POW expression
		   | expression PROPERTY_ACCESS expression
		   | expression STATIC_PROPERTY_ACCESS expression
		   | '$' expression
		   | '[' array_element_list ']'
		   | expression '[' expression ']'
		   | expression '[' ']'
		   | '(' expression ')'
		   | expression '(' expression_list ')'
		   | expression '(' ')'
		   | NEW expression
		   | ID
		   | INT
		   | FLOAT
		   | STRING
		   | BOOL
		   | NIL
		   ;

array_element_list : array_element
                   | array_element_list ',' array_element
                   ;

array_element : expression KEY_ACCESS expression
              | expression
              ;

expression_list : expression
				| expression_list ',' expression
				;

expression_list_empty : /* empty */
				      | expression_list
				      ;
						  
		
statement : '{' statement_list_empty '}'
		  | expression ';'
		  | while_statement
		  | for_statement
		  | foreach_statement
		  | if_statement
		  | switch_statement
		  | ECHO expression_list ';'
		  | RETURN expression ';'
          | RETURN ';'
		  | BREAK ';'
          | CONTINUE ';'
		  | ';'
		  ;

statement_list : statement
			   | statement_list statement
			   ;

statement_list_empty : /* empty */
                     | statement_list
                     ;

			   
while_statement : WHILE '(' expression ')' statement
				| WHILE '(' expression ')' ':' statement_list ENDWHILE ';'
				| DO statement WHILE '(' expression ')' ';'
				;
			 
for_statement : FOR '(' expression_list_empty ';' expression_list_empty ';' expression_list_empty ')' statement
			  | FOR '(' expression_list_empty ';' expression_list_empty ';' expression_list_empty ')' ':' statement_list ENDFOR ';'
			  ;
			   
foreach_statement : FOREACH '(' expression AS '$' ID KEY_ACCESS '$' ID ')' statement
                  | FOREACH '(' expression AS expression ')' statement
				  | FOREACH '(' expression AS '$' ID KEY_ACCESS '$' ID ')' ':' statement_list ENDFOREACH ';'
				  | FOREACH '(' expression AS expression ')' ':' statement_list ENDFOREACH ';'
				  ;
				  
if_statement : IF '(' expression ')' statement elseif_statements_1 ELSE statement
			 | IF '(' expression ')' statement elseif_statements_1
			 | IF '(' expression ')' statement ELSE statement
			 | IF '(' expression ')' statement
			 | IF '(' expression ')' ':' statement_list elseif_statements_2 ELSE ':' statement_list ENDIF ';'
			 | IF '(' expression ')' ':' statement_list elseif_statements_2 ENDIF ';'
			 | IF '(' expression ')' ':' statement_list ELSE ':' statement_list ENDIF ';'
			 | IF '(' expression ')' ':' statement_list ENDIF ';'
			 ;

elseif_statements_1 : elseif_statement_1
					| elseif_statements_1 elseif_statement_1
					;
					
elseif_statement_1 : ELSEIF '(' expression ')' statement
				   ;
				 
elseif_statements_2 : elseif_statement_2
					| elseif_statements_2 elseif_statement_2
					;
					
elseif_statement_2 : ELSEIF '(' expression ')' ':' statement_list
				   ;
					
switch_statement : SWITCH '(' expression ')' '{' case_statements '}'
				 | SWITCH '(' expression ')' '{' '}'
				 | SWITCH '(' expression ')' ':' case_statements ENDSWITCH ';'
				 | SWITCH '(' expression ')' ':' ENDSWITCH ';'
				 ;

case_statements : case_statement
				| case_statements case_statement
				;
				 
case_statement : CASE expression ':' statement_list
			   | CASE expression ':'
			   | CASE expression ';' statement_list
			   | CASE expression ';'
			   | DEFAULT ':' statement_list
			   | DEFAULT ':'
			   | DEFAULT ';' statement_list
			   | DEFAULT ';'
			   ;

function_definition : function_definition_header '{' statement_list_empty '}'
					;
					
function_definition_header : FUNCTION ID '(' parameter_function_list ')'
						   | FUNCTION ID '(' parameter_function_list ')' ':' type_list
						   | FUNCTION ID '(' ')'
						   | FUNCTION ID '(' ')' ':' type_list
						   ;

type_list : type
          | type_list '|' type
          ;

type : TYPE
     | ID
     ;

parameter_function_list : parameter_function
						| parameter_function_list ',' parameter_function
						;
						
parameter_function : '$' ID
				   | '$' ID '=' expression
				   | type_list '$' ID
				   | type_list '$' ID '=' expression
				   ;
				   


class_declaration : CLASS ID '{' class_member_declarations '}'
				  | CLASS ID '{' '}'
				  | CLASS ID EXTENDS ID '{' class_member_declarations '}'
				  | CLASS ID EXTENDS ID '{' '}'
				  ;

class_member_declarations : class_member_declaration
						  | class_member_declarations class_member_declaration
						  ;
						  
class_member_declaration : class_const_elements
						 | property_declaration
						 | method_declaration
						 ;

class_const_elements : CONST const_elements ';'
					 | visibility_modifiers CONST const_elements ';'
					 ;

const_elements : const_element
			   | const_elements ',' const_element
			   ;
			   
const_element : ID '=' expression
			  ;

visibility_modifiers : PUBLIC
                     | PROTECTED
                     | PRIVATE
                     ;

declaration_modifiers : visibility_modifiers
                      | visibility_modifiers STATIC
                      | STATIC visibility_modifiers
                      | STATIC
                      ;
			   
property_declaration : VAR property_elements ';'
					 | declaration_modifiers property_elements	';'
					 ;
					 
property_elements : property_element
				  | property_elements ',' property_element
				  ;
				  
property_element : '$' ID
				 | '$' ID '=' expression
				 ;
				
method_declaration : function_definition
				   | declaration_modifiers function_definition
				   ;

%%
