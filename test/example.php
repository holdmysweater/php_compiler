<!DOCTYPE html>
<html>
<head>
    <title>Test Page</title>
    <!-- HTML comment -->
</head>
<body>
<h1>Mixed Content</h1>

<?php
$var = "test";

function typedFunction(int $param): string {
    return "Number: " . $param;
}

class SimpleClass {}

?>
</body>
</html>