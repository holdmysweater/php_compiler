<?php
// Функции
function simpleFunction($param1, $param2) {
    return $param1 + $param2;
}

function withTypes(int $number, string $text): float {
    return $number + floatval($text);
}

// Классы
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

// Использование классов
$obj = new DerivedClass("test");
$obj->publicMethod();
DerivedClass::staticMethod();

// instanceof
$check = $obj instanceof BaseClass;

// Константы класса
class Constants {
    const CONSTANT = 'value';
    public const PUBLIC_CONST = 'public';
}
?>