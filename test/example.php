<?php
function typedFunction(string|SimpleClass $param, int $anotherParam): string|int|null
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

    static function simpleFunc2(void|int $hi): void
    {
    }

//    static function simpleFunc3(): string
//    {
//    }
}

echo SimpleClass::simpleFunc2()
?>