<?php

class base {
    private function p() {
        echo "p";
    }

    protected function q() {
        echo "q";
    }

    public function r() {
        $this->p();
        echo "|";
        $this->q();
    }
}

class child extends base {
    public function t() {
        $this->q();
    }

    public function u() {
        $this->p();
    } // should be forbidden in real PHP
}

$o = new child();
//$o->p();
//$o->q();
$o->r();
$o->t();
//$o->u();
