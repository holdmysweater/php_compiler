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
</body>
</html>

<?php
echo "Script completed";
?>