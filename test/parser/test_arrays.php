<?php

$array1 = [1, 2, 3];

$assoc = [
    'name' => 'John',
    'age' => 25,
    'city' => 'Moscow'
];

$multi = [
    'user' => [
        'profile' => [
            'name' => 'Alice'
        ]
    ]
];

echo $array1[0];
echo $assoc['name'];
echo $multi['user']['profile']['name'];

$array1[] = 4;
$assoc['email'] = 'john@example.com';
?>