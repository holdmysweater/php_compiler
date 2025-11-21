<!DOCTYPE html>
<html>
<head>
    <title>Test Page</title>
</head>
<body>
    <h1>Welcome</h1>
    
    <?php
    $user = "John";
    if ($user) {
        echo "<p>Hello, " . htmlspecialchars($user) . "!</p>";
    } else {
        echo "<p>Hello, Guest!</p>";
    }
    ?>

    <ul>

    <?php
    for ($i = 1; $i <= 3; $i++) {
        echo "<li>Item " . $i . "</li>";
    }
    ?>
    </ul>
</body>
</html>
