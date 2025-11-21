<!DOCTYPE html>
<html>
<head>
    <title>Test Page</title>
</head>
<body>
    <h1>Welcome</h1>
    
    <?php
    $user = "John";
    if ($user): ?>
        <p>Hello, <?= $user ?>!</p>
    <?php else: ?>
        <p>Hello, Guest!</p>
    <?php endif; ?>
    
    <ul>
    <?php for ($i = 1; $i <= 3; $i++): ?>
        <li>Item <?= $i ?></li>
    <?php endfor; ?>
    </ul>
</body>
</html>