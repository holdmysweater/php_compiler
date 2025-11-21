<?php

if ($a > 10) {
    echo "A is greater than 10";
} elseif ($a == 10) {
    echo "A is 10";
} else {
    echo "A is less than 10";
}

if ($a > 10):
    echo "Greater";
elseif ($a == 10):
    echo "Equal";
else:
    echo "Less";
endif;

switch ($a) {
    case 10:
        echo "Ten";
        break;
    case 20:
        echo "Twenty";
        break;
    default:
        echo "Other";
}

while ($i < 10) {
    echo $i;
    $i++;
}

for ($k = 0; $k < 5; $k++) {
    echo $k;
}

foreach ([1, 2, 3] as $value) {
    echo $value;
}

foreach (['a' => 1, 'b' => 2] as $key => $value) {
    echo "$key: $value";
}

do {
    echo "At least once";
} while (false);
?>