<?php
$name = "World";
$array1 = ['key' => 'value'];

$simple = "Hello $name!";
$complex = "Hello {$name}!";
$arrayAccess = "Value: {$array1['key']}";

$heredoc = <<<EOT
Hello $name
This is a multiline string
EOT;

$obj = new stdClass();
$obj->property = "test";
echo "Property: {$obj->property}";
echo "Calculation: {2 + 3 * 4}";
?>