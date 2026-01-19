<?php
function add(int $a, int $b, bool $asString = false): int|string {
    return $asString ? $a . $b : $a ? $b;
}

echo add(2, 3) . "\n";
