<?php
$single = 'Simple text \n no interpolation $var';
$single_escaped = 'Escaping: \\ \' ';

$double = "Interpolation: $name and {$user->getName()}";
$double_escaped = "Escape: \\ \" \$ \n \t \r \v \e \f";
$double_octal = "Octal: \101 \102 \103";
$double_hex = "Hex: \x41 \x42 \x43";

$simple_var = "Variable: $username";
$simple_array = "Array element: $users[0]";
$simple_object = "Property: $user->name";

$complex = "Expression: {$a + $b}";
$complex_method = "Method: {$user->getName()}";
$complex_nested = "Nested: {$array[0]->property}";

$alt_simple = "Alternative: ${username}";
$alt_object = "Alternative: ${user->name}";

$escape_dollar = "Dollar sign: \$100";
$escape_brace = "Curly brace: \{ text";

$single_newline = 'Single quotes: line 1
line 2
line 3';

$double_mixed_newline = "Mixed newlines:
Line 1 with \n escape
Line 2 with actual
line break";
?>