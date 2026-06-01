#!/usr/bin/perl
use strict;
use warnings;

sub assert_eq {
    my ($got, $expected, $name) = @_;
    if ($got eq $expected) {
        print "PASS: $name\n";
    } else {
        print "FAIL: $name — expected '$expected' got '$got'\n";
    }
}

assert_eq(1 + 1,            "2",     "addition");
assert_eq(uc("hello"),      "HELLO", "uc");
assert_eq(length("Qt6"),    "3",     "length");
assert_eq(join(",", "a","b","c"), "a,b,c", "join");

# Intentional failure
assert_eq("foo", "bar", "intentional_failure");
