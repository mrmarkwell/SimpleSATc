#!/usr/bin/perl
#use strict;

my $command1;
my $count;
my $output;
my $match;
print "\nWhat variable count would you like to start from?\n";
my $start = <STDIN>;
print "\nWhat variable count would you like to end with?\n";
my $end = <STDIN>;
for ($count = $start; $count < $end; $count++) {
   print "\nTest " . ($count - $start + 1) . "  ";
   $command1 = "./Tests/mkcnf $count " . $count*1.5 . " 2 > Tests/test.cnf 2> /dev/null";
   $output = `$command1`;
   $output = `./minisat Tests/test.cnf`;
   if ($output =~ m/UNSATISFIABLE/) {
      print "minisat: UNSATISFIABLE\t";
      $match = 0;
   } else {
      print "minisat: SATISFIABLE  \t";
      $match = 1;
   }
   `./SimpleSATc Tests/test.cnf`;
   $output = `tail SimpleSATc.out`;
   if ($output =~ m/UNSATISFIABLE/) {
      print "SimpleSATc: UNSATISFIABLE\t";
      if ($match == 0) {
         print "MATCH!";
      } else {
         print "NO MATCH!";
      }
   } else {
      print "SimpleSATc: SATISFIABLE  \t";
      if ($match == 0) {
         print "NO MATCH!";
      } else {
         print "MATCH!";
      }
   }
}
print "\n\n";
