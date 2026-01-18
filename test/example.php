<?php

class A
{
    function __toString()
    {
        return "A";
    }
}

$a = new A();
echo $a;
