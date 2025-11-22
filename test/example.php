<!DOCTYPE html>
<html>
<head>
    <title>Test Page</title>
    <!-- HTML comment -->
</head>
<body>
<h1>Mixed Content</h1>

<?php
$title = "Dynamic Title";
$items = ['Item 1', 'Item 2', 'Item 3'];
?>

<div class="content">
    <h2><?php echo $title; ?></h2>

    <ul>
        <?php foreach ($items as $item): ?>
            <li><?= $item ?></li>
        <?php endforeach ?>
    </ul>

    <?php if (count($items) > 0): ?>
        <p>Total items: <?= count($items) ?></p>
    <?php endif ?>
</div>

<?php
$footer = "Page Footer";
?>

<footer>
    <?= $footer ?>
</footer>
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
</body>
</html>