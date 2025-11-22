<?php
$var = "test";

function typedFunction(string|SimpleClass $param): string|int|null
{
    $something = "string content";

    if ($something == "string") {
        return "string";
    }

    return "Number: " . $param;
}

class SimpleClass
{
    function simpleFunc(): string|int|null
    {
        return null;
    }
}

$result = typedFunction("hi")
?>