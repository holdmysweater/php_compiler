<?php

function simpleFunction($param) {
    return $param * 2;
}

function typedFunction(int $param): string {
    return "Number: " . $param;
}

function defaultParam($param = "default") {
    return $param;
}

class SimpleClass {
    public $property = "value";
    private $privateProp;
    protected $protectedProp;
    
    const CONSTANT = "constant_value";
    
    public function __construct($value) {
        $this->privateProp = $value;
    }
    
    public function getValue(): string {
        return $this->privateProp;
    }
    
    public static function staticMethod() {
        return "static";
    }
}

class ExtendedClass extends SimpleClass {
    public function newMethod() {
        return $getValue;
    }
}

$obj = new SimpleClass("test");
echo $obj->getValue();
echo SimpleClass::staticMethod();
?>