<?php

function buildUser(array $input): array
{
    return array(
        "id" => $input["id"],
        "name" => $input["name"],
        "active" => true
    );
}

$userData = array(
    "id" => 10,
    "name" => "Alice"
);

$user = buildUser($userData);
print_r($user);
