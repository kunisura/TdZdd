#!/usr/bin/perl -w
use strict;

my $cols = 0;
my $rows = 0;
my $max = 0;
my @board;

while (<>) {
    s/\s+//g;
    if (/SIZE\s*(\d+)X(\d+)/i) {
        ($cols, $rows) = ($1, $2);
    }
    elsif (/LINE#(\d+)\s*\((\d+),(\d+)\)-\((\d+),(\d+)\)/i) {
        $board[$2][$3] = $board[$4][$5] = $1;
        $max = $1 if $max < $1;
    }
    elsif (/^(?:\d\d)+$/) {
        my @data;
        for (my $j = 0; /(\d\d)/g; ++$j) {
            my $x = $1 + 0;
            push(@data, $x);
            $max = $x if $max < $x;
        }
        $cols = @data + 0 if $cols < @data + 0;
        push(@board, \@data);
    }
}

$rows = @board + 0 if $rows < @board + 0;
$cols >= 1 and $rows >= 1 or die "INPUT ERROR.\n";

my $w = length($max);

printf "%d %d\n", $cols, $rows;

for my $i (0 .. $rows - 1) {
    for my $j (0 .. $cols - 1) {
        print " " if $j >= 1;
        printf "%${w}s", $board[$j][$i] || "-";
    }
    print "\n";
}
