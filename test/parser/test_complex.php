<?php

class ComplexExample {
    public function process(array $data): string {
        if (empty($data)) {
            return null;
        }
        
        $result = "";
        foreach ($data as $key => $value) {
            if ($value instanceof DateTime) {
                $result .= $key . ": " . $value->format('Y-m-d') . "\n";
            } elseif (is_array($value)) {
                $result .= $key . ": [" . implode(', ', $value) . "]\n";
            } else {
                $result .= "{$key}: {$value}\n";
            }
        }
        
        return $result;
    }
    
    public static function create(): self {
        return new self();
    }
}

$processor = ComplexExample::create();
$data = [
    'date' => new DateTime(),
    'numbers' => [1, 2, 3],
    'name' => "Test"
];
echo $processor->process($data);
?>