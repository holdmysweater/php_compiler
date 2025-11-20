<?php

if ($condition) {
    echo "Condition is true";
} elseif ($other_condition) {
    echo "Other condition";
} else {
    echo "All are false";
}


if ($condition):
    echo "Alternative syntax";
endif;


switch ($value) {
    case 1:
        echo "One";
        break;
    case 2:
        echo "Two";
        break;
    default:
        echo "Default";
}


for ($i = 0; $i < 10; $i++) {
    echo $i;
}

foreach ($arrays as $key => $value) {
    echo "Key: $key, Value: $value";
}

while ($condition) {
    $condition = checkSomething();
}

do {
    $counter--;
} while ($counter > 0);


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