#!/usr/bin/perl 



$vortex_status                 = "vortex-status.txt";
@file_status                   = `cat vortex-status.txt`;


$number_of_items               = 0;
$number_of_items_completed     = 0;
$number_of_items_not_completed = 0;
foreach (@file_status) {
    if ($_ =~ /\[  OK  \]/ || $_ =~ /\[NOT OK\]/) {
	$number_of_items++;
    }

    if ($_ =~ /\[  OK  \]/) {
	$number_of_items_completed++;
    }

    if ($_ =~ /\[NOT OK\]/) {
	$number_of_items_not_completed++;
    }

}

$percentage_completed          = sprintf "%2.2f", ($number_of_items_completed / $number_of_items) * 100;
$percentage_not_completed      = sprintf "%2.2f", ($number_of_items_not_completed / $number_of_items) * 100;

if ($ARGV[0] =~ /--show/) {
    print "Actual Vortex Library Status:\n";
    print "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n";
    print "Number of items:               $number_of_items\n";
    print "Number of items completed:     $number_of_items_completed\n";
    print "Number of items not completed: $number_of_items_not_completed\n";
    print "Percentage completed:          $percentage_completed(%)\n";
    print "Percentage not completed:      $percentage_not_completed(%)\n";
    exit 0;
}

foreach (@file_status) {
    if ($_ =~ s/Number of Items not completed: .*/Number of Items not completed: $number_of_items_not_completed/) {
	next;
    }
    if ($_ =~ s/Number of Items completed: .*/Number of Items completed:     $number_of_items_completed/) {
	next;
    }
    if ($_ =~ s/Number of Items: (.*)/Number of Items:               $number_of_items/) {
	next;
    }
    if ($_ =~ s/Percentage not completed: .*/Percentage not completed:      $percentage_not_completed\(%\)/) {
	next;
    }
    if ($_ =~ s/Percentage completed: .*/Percentage completed:          $percentage_completed\(%\)/) {
	next;
    }

}

open STATUS, "> $vortex_status" or die "unable to update $vortex_status";
print STATUS @file_status;
close STATUS;


