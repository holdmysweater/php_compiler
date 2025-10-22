<?php
// Одинарные кавычки
$single = 'Простой текст \n не интерполируется $var';
$single = 'Простой текст \n не интерполируется $var';
$single_escaped = 'Экранирование: \\ \' ';

// Двойные кавычки
$double = "Интерполяция: $name и {$user->getName()}";
$double_escaped = "Escape: \\ \" \$ \n \t \r \v \e \f";
$double_octal = "Octal: \101 \102 \103"; // ABC
$double_hex = "Hex: \x41 \x42 \x43"; // ABC

// Простые вставки
$simple_var = "Переменная: $username";
$simple_array = "Элемент массива: $users[0]";
$simple_object = "Свойство: $user->name";

// Сложные вставки
$complex = "Выражение: {$a + $b}";
$complex_method = "Метод: {$user->getName()}";
$complex_nested = "Вложенное: {$array[0]->property}";

// Альтернативные вставки
$alt_simple = "Альтернатива: ${username}";
$alt_object = "Альтернатива: ${user->name}";

// Экранирование в интерполяции
$escape_dollar = "Знак доллара: \$100";
$escape_brace = "Фигурная скобка: \{ текст";
?>