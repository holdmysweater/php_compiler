<?php

$a = 10 + 5 * 2;
$b = (10 + 5) * 2;
$c = 15 % 4;
$d = 2 ** 3;

$name = "John";
$greeting = "Hello " . $name . "!";

$bool1 = true && false;
$bool2 = true || false;
$bool3 = !true;
$bool4 = $a > 5 and $b < 30;
$bool5 = $a > 5 or $b < 30;
$bool6 = $a > 5 xor $b < 30;

$i = 0;
$i++;
++$i;
$j = $i--;

$result = $a > 10 ? "big" : "small";
?>