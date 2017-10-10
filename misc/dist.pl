#!/usr/bin/perl
use strict;
use warnings;
use Text::LevenshteinXS;

sub _T {
	my ($a, $b) = @_;
	print distance($a, $b) . " '$a' '$b'\n";
}
_T("hello","yullo");
_T("he","hello");
_T("hello","helo");
_T("helo","hello");
_T("OOE","OOA");
_T("OOE","OOAE");
_T("OOEA","OOAE");
_T("","abc");
_T("abc","");
_T("hello","h");
_T("x","hello");
_T("","");
