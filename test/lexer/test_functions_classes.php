<?php
function simpleFunction($param1, $param2) {
    return $param1 + $param2;
}

function withTypes(int $number, string $text): float {
    return $number + floatval($text);
}

class BaseClass {
    public $publicProperty;
    protected $protectedProperty;
    private $privateProperty;
    static $staticProperty;
    
    public function __construct($param) {
        $this->publicProperty = $param;
    }
    
    public function publicMethod() {
        return $this->publicProperty;
    }
    
    protected function protectedMethod() {
        return $this->protectedProperty;
    }
    
    static function staticMethod() {
        return self::$staticProperty;
    }
}

class DerivedClass extends BaseClass {
    public function derivedMethod() {
        return $this->protectedMethod();
    }
}

$obj = new DerivedClass("test");
$obj->publicMethod();
DerivedClass::staticMethod();

$check = $obj instanceof BaseClass;

class Constants {
    const CONSTANT = 'value';
    public const PUBLIC_CONST = 'public';
}
?>