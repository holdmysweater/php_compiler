<!DOCTYPE html>
<html>
<head>
    <title>Тестовая страница</title>
    <!-- HTML комментарий -->
</head>
<body>
    <h1>Смешанный контент</h1>
    
    <?php
    // PHP код внутри HTML
    $title = "Динамический заголовок";
    $items = ['Пункт 1', 'Пункт 2', 'Пункт 3'];
    ?>
    
    <div class="content">
        <h2><?php echo $title; ?></h2>
        
        <ul>
            <?php foreach ($items as $item): ?>
            <li><?= $item ?></li>
            <?php endforeach; ?>
        </ul>
        
        <?php if (count($items) > 0): ?>
        <p>Всего элементов: <?= count($items) ?></p>
        <?php endif; ?>
    </div>
    
    <?php
    // Еще PHP код
    $footer = "Подвал страницы";
    ?>
    
    <footer>
        <?= $footer ?>
    </footer>
</body>
</html>

<?php
// PHP после HTML
echo "Скрипт завершен";
?>