<?php
// Условные операторы
if ($condition) {
    echo "Условие истинно";
} elseif ($other_condition) {
    echo "Другое условие";
} else {
    echo "Все ложны";
}

// Альтернативный синтаксис if
if ($condition):
    echo "Альтернативный синтаксис";
endif;

// Switch
switch ($value) {
    case 1:
        echo "Один";
        break;
    case 2:
        echo "Два";
        break;
    default:
        echo "По умолчанию";
}

// Циклы
for ($i = 0; $i < 10; $i++) {
    echo $i;
}

foreach ($array as $key => $value) {
    echo "Key: $key, Value: $value";
}

while ($condition) {
    $condition = checkSomething();
}

do {
    $counter--;
} while ($counter > 0);

// Break и continue
for ($i = 0; $i < 10; $i++) {
    if ($i == 5) {
        break;
    }
    if ($i % 2 == 0) {
        continue;
    }
    echo $i;
}
?>