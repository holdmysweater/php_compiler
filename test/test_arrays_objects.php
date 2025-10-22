<?php
// Простые массивы
$simple_array = [1, 2, 3, 4, 5];
$assoc_array = [
    'name' => 'John',
    'age' => 30,
    'city' => 'Moscow'
];

// Доступ к элементам
$first = $simple_array[0];
$name = $assoc_array['name'];

// Многомерные массивы
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

// Объекты и доступ к свойствам
$user = new stdClass();
$user->name = "John";
$user->age = 25;

$user_name = $user->name;
$user_age = $user->age;

// Цепочки вызовов
$result = $object->method1()->method2()->property;

// Массивы объектов
$users = [
    $user,
    new stdClass()
];
?>