<?php

$a = 10;
$a += 5;
$a -= 3;
$a *= 2;
$a /= 4;
$a %= 3;
$a **= 2;
$a .= "text";

$bit1 = 5 & 3;
$bit2 = 5 | 3;
$bit3 = 5 ^ 3;
$bit4 = ~5;
$bit5 = 5 << 1;
$bit6 = 5 >> 1;

$comp1 = $a == $b;
$comp2 = $a === $b;
$comp3 = $a != $b;
$comp4 = $a !== $b;
$comp5 = $a <=> $b;

$obj = new stdClass();
$isObject = $obj instanceof stdClass;
?>