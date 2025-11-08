<?php
// Indexed array
$arr = [10, 20, 30];

// Associative array with string keys
$arr2 = ["name" => "Alice", "age" => 30];

// Mixed keys
$arr3 = [0 => "zero", "1" => "one", "two" => 2];

// Nested arrays
$arr4 = [
    "fruits" => ["apple", "banana"],
    "numbers" => [1, 2, 3]
];

// Keys can be boolean or null (cast accordingly)
$arr5 = [true => "yes", false => "no", null => "empty"];


$arr[0] = "zero";
$arr["one"] = 1;
$arr['two'] = 2;
$arr[true] = "bool";   // same as $arr[1]
$arr[null] = "null";   // same as $arr[""]
$arr[1.5] = "float";   // same as $arr[1]
