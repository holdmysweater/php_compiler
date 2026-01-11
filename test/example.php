<?php

class InvalidAgeException extends Exception
{
}

function validateAge(int $age): void
{
    if ($age < 0) {
        throw new InvalidAgeException("Age cannot be negative");
    }

    if ($age < 18) {
        throw new Exception("User is underaged");
    }
}

try {
    validateAge(15);
    echo "Age is valid";
} catch (InvalidAgeException $e) {
    echo "InvalidAgeException: " . $e->getMessage();
} catch (Exception $e) {
    echo "Exception: " . $e->getMessage();
}

$try = 'f';

try {
    validateAge(123);
} finally {
    echo "hi";
}
