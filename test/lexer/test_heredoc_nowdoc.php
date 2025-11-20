<?php
$heredoc = <<<HTML
<div class="container">
    <h1>Hello $username</h1>
    <p>Email: {$user->getEmail()}</p>
    <p>Array: {$data['key']}</p>
    Escape: \\ \$ \n
</div>
HTML;

$hi = 'lol';

$nowdoc = <<<'TEXT'
This is nowdoc string
Variables are not interpolated: $username
Escape sequences also: \n \t
Just text {curly} braces
TEXT;

$simple_heredoc = <<<EOT
Simple variable: $var
Array: {$arr[0]}
Property: {$obj->prop}
EOT;

$complex_heredoc = <<<SQL
SELECT * FROM users 
WHERE name = '{$user->getName()}'
AND age > {$minAge}
AND status = '{$status}'
SQL;

$multiline = <<<MD
# Header

- Item 1: $value1
- Item 2: {$value2 + $value3}
- Item 3: {$something->method()}

`Code: {$code}`
MD;

$text = <<<HTML
    content
    HTML; // PHP 7.3+

?>