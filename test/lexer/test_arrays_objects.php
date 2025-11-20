<?php

$simple_array = [1, 2, 3, 4, 5];
$assoc_array = [
    'name' => 'John',
    'age' => 30,
    'city' => 'Moscow'
];

$first = $simple_array[0];
$name = $assoc_array['name'];

$multi_array = [
    'users' => [
        ['id' => 1, 'name' => 'Alice'],
        ['id' => 2, 'name' => 'Bob']
    ],
    'settings' => [
        'theme' => 'dark',
        'language' => 'ru'
    ]
];

$user = new stdClass();
$user->name = "John";
$user->age = 25;

$user_name = $user->name;
$user_age = $user->age;

$result = $object->method1()->method2()->property;

$users = [
    $user,
    new stdClass()
];

$arr[0] = "zero";
$arr["one"] = 1;
$arr['two'] = 2;
$arr[true] = "bool";   
$arr[null] = "null";   
$arr[1.5] = "float";   

?>