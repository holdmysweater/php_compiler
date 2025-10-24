<?php
// Heredoc с интерполяцией
$heredoc = <<<HTML
<div class="container">
    <h1>Hello $username</h1>
    <p>Email: {$user->getEmail()}</p>
    <p>Array: {$data[key]}</p>
    Escape: \\ \$ \n
</div>
HTML;

$hi = 'lol';

// Nowdoc без интерполяции
$nowdoc = <<<'TEXT'
Это nowdoc строка
Переменные не интерполируются: $username
Escape последовательности тоже: \n \t
Просто текст {фигурные} скобки
TEXT;

// Heredoc с простыми вставками
$simple_heredoc = <<<EOT
Простая переменная: $var
Массив: $arr[0]
Свойство: $obj->prop
EOT;

// Сложные вставки в heredoc
$complex_heredoc = <<<SQL
SELECT * FROM users 
WHERE name = '{$user->getName()}'
AND age > {$minAge}
AND status = '{$status}'
SQL;

// Многострочный heredoc
$multiline = <<<MD
# Заголовок

- Пункт 1: $value1
- Пункт 2: {$value2 + $value3}
- Пункт 3: {$object->method()}

`Код: {$code}`
MD;
?>