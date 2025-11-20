<?php
$error1 = "Incorrect interpolation: $obj->method()";
$error2 = "Error: $array[0]->property";

$correct1 = "Correct: {$obj->method()}";
$correct2 = "Correct: {$array[0]->property}";

$café = "coffee";
$piñata = "game";
$переменная = "cyrillic";
$_123 = "number at start";

$complex1 = "Result: {($a + $b) * $c}";
$complex2 = "Call: {strtoupper($name)}";

$large_int = 1234567890;
$small_float = 0.0000001;
$negative = -42;
$zero = 0;

$all_escapes = "All escapes: \\ \n \r \t \v \e \f \" \$ \101 \x41";

$empty_string = "";
$empty_array = [];
$null_var = null;
?>