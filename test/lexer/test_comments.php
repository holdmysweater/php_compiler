<?php
// Regular single-line comment

# Shell-style comment

/*
 Multiline comment
 spanning several lines
 */

/*
 * Multiline comment * /
 * / with multiple lines
 ***/

class Test {
    // Comment inside class
    
    /**
     * DocBlock comment
     * @param string $name
     * @return bool
     */
    public function method($name) {
        // Comment inside method
        return true;
    }
}

/* Nested /* comment */ // doesn't work in PHP */

// Comment after code
$vars = "value"; // inline comment

/* Comment
   with * asterisks
   in the middle */
?>