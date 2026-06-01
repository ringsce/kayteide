<?php
// Sample PHP unit-style test file

function assertEqual($a, $b, $name) {
    if ($a === $b) {
        echo "PASS: $name\n";
    } else {
        echo "FAIL: $name — expected " . var_export($b, true) . " got " . var_export($a, true) . "\n";
    }
}

assertEqual(1 + 1,              2,       "addition");
assertEqual(strtoupper("hello"), "HELLO", "strtoupper");
assertEqual(strlen("Qt6"),       3,       "strlen");
assertEqual(implode(",", ["a","b","c"]), "a,b,c", "implode");
assertEqual(10 % 3,             1,       "modulo");

// Intentional failure
assertEqual(42, 99, "intentional_failure");
