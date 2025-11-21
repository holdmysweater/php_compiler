<?php
class TestModifiers {
    public $publicProp;
    private $privateProp;
    protected $protectedProp;
    static $staticProp;
    
    public static $publicStatic;
    private static $privateStatic;
    
    const PUBLIC_CONST = "public";
    private const PRIVATE_CONST = "private";
    
    public function publicMethod() {}
    private function privateMethod() {}
    protected function protectedMethod() {}
    static function staticMethod() {}
}
?>