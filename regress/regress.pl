#!/usr/bin/env perl
#
# $Id$
#
# Copyright (c) 2017 Ingo Schwarze <schwarze@openbsd.org>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

use warnings;
use strict;

# Used because open(3p) and open2(3p) provide no way for handling
# STDERR of the child process, neither for appending it to STDOUT,
# nor for piping it into the Perl program.
use IPC::Open3 qw(open3);

# Define this at one place such that it can easily be changed
# if diff(1) does not support the -a option.
my @diff = qw(diff -au);

# --- utility functions ------------------------------------------------

sub usage ($) {
	warn shift;
	print STDERR "usage: $0 [directory[:test] [modifier ...]]\n";
	exit 1;
}

# Run a command and send STDOUT and STDERR to a file.
# 1st argument: path to the output file
# 2nd argument: command name
# The remaining arguments are passed to the command.
sub sysout ($@) {
	my $outfile = shift;
	local *OUT_FH;
	open OUT_FH, '>', $outfile or die "$outfile: $!";
	my $pid = open3 undef, ">&OUT_FH", undef, @_;
	close OUT_FH;
	waitpid $pid, 0;
	return $? >> 8;
}

# Simlar, but filter the output as needed for the lint test.
sub syslint ($@) {
	my $outfile = shift;
	open my $outfd, '>', $outfile or die "$outfile: $!";
	my $infd;
	my $pid = open3 undef, $infd, undef, @_;
	while (<$infd>) {
		s/^mandoc: [^:]+\//mandoc: /;
		print $outfd $_;
	}
	close $outfd;
	close $infd;
	waitpid $pid, 0;
	return 0;
}

# Simlar, but filter the output as needed for the html test.
sub syshtml ($@) {
	my $outfile = shift;
	open my $outfd, '>', $outfile or die "$outfile: $!";
	my $infd;
	my $pid = open3 undef, $infd, undef, @_;
	my $state;
	while (<$infd>) {
		chomp;
		if (!$state && s/.*<math class="eqn">//) {
			$state = 1;
			next unless length;
		}
		$state = 1 if /^BEGINTEST/;
		if ($state && s/<\/math>.*//) {
			s/^ *//;
			print $outfd "$_\n" if length;
			undef $state;
			next;
		}
		s/^ *//;
		print $outfd "$_\n" if $state;
		undef $state if /^ENDTEST/;
	}
	close $outfd;
	close $infd;
	waitpid $pid, 0;
	return 0;
}

my @failures;
sub fail ($$$) {
	warn "FAILED: @_\n";
	push @failures, [@_];
}


# --- process command line arguments -----------------------------------

my ($subdir, $onlytest) = split ':', (shift // '.');
my $displaylevel = 2;
my %targets;
for (@ARGV) {
	if (/^[0123]$/) {
		$displaylevel = int;
		next;
	}
	/^(all|ascii|utf8|man|html|markdown|lint|clean|verbose)$/
	    or usage "$_: invalid modifier";
	$targets{$_} = 1;
}
$targets{all} = 1
    unless $targets{ascii} || $targets{utf8} || $targets{man} ||
      $targets{html} || $targets{markdown} ||
      $targets{lint} || $targets{clean};
$targets{ascii} = $targets{utf8} = $targets{man} = $targets{html} =
    $targets{markdown} = $targets{lint} = 1 if $targets{all};
$displaylevel = 3 if $targets{verbose};


# --- parse Makefiles --------------------------------------------------

my %vars = (MOPTS => '');
sub parse_makefile ($) {
	my $filename = shift;
	open my $fh, '<', $filename or die "$filename: $!";
	while (<$fh>) {
		chomp;
		next unless /\S/;
		last if /^# OpenBSD only/;
		next if /^#/;
		next if /^\.include/;
		/^(\w+)\s*([?+]?)=\s*(.*)/
		    or die "$filename: parse error: $_";
		my $var = $1;
		my $opt = $2;
		my $val = $3;
		$val =~ s/\$\{(\w+)\}/$vars{$1}/;
		$val = "$vars{$var} $val" if $opt eq '+';
		$vars{$var} = $val
		    unless $opt eq '?' && defined $vars{$var};
	}
	close $fh;
}

if ($subdir eq '.') {
	$vars{SUBDIR} = 'roff char mdoc man tbl eqn';
} else {
	parse_makefile "$subdir/Makefile";
	parse_makefile "$subdir/../Makefile.inc"
	    if -e "$subdir/../Makefile.inc";
}

my @mandoc = '../mandoc';
my @subdir_names;
my (@regress_testnames, @utf8_testnames, @lint_testnames);
my (@html_testnames, @markdown_testnames);
my (%skip_ascii, %skip_man, %skip_markdown);

push @mandoc, split ' ', $vars{MOPTS} if $vars{MOPTS};
delete $vars{MOPTS};
delete $vars{SKIP_GROFF};
delete $vars{SKIP_GROFF_ASCII};
delete $vars{TBL};
delete $vars{EQN};
if (defined $vars{SUBDIR}) {
	@subdir_names = split ' ', $vars{SUBDIR};
	delete $vars{SUBDIR};
}
if (defined $vars{REGRESS_TARGETS}) {
	@regress_testnames = split ' ', $vars{REGRESS_TARGETS};
	delete $vars{REGRESS_TARGETS};
}
if (defined $vars{UTF8_TARGETS}) {
	@utf8_testnames = split ' ', $vars{UTF8_TARGETS};
	delete $vars{UTF8_TARGETS};
}
if (defined $vars{HTML_TARGETS}) {
	@html_testnames = split ' ', $vars{HTML_TARGETS};
	delete $vars{HTML_TARGETS};
}
if (defined $vars{MARKDOWN_TARGETS}) {
	@markdown_testnames = split ' ', $vars{MARKDOWN_TARGETS};
	delete $vars{MARKDOWN_TARGETS};
}
if (defined $vars{LINT_TARGETS}) {
	@lint_testnames = split ' ', $vars{LINT_TARGETS};
	delete $vars{LINT_TARGETS};
}
if (defined $vars{SKIP_ASCII}) {
	for (split ' ', $vars{SKIP_ASCII}) {
		$skip_ascii{$_} = 1;
		$skip_man{$_} = 1;
	}
	delete $vars{SKIP_ASCII};
}
if (defined $vars{SKIP_TMAN}) {
	$skip_man{$_} = 1 for split ' ', $vars{SKIP_TMAN};
	delete $vars{SKIP_TMAN};
}
if (defined $vars{SKIP_MARKDOWN}) {
	$skip_markdown{$_} = 1 for split ' ', $vars{SKIP_MARKDOWN};
	delete $vars{SKIP_MARKDOWN};
}
if (keys %vars) {
	my @vars = keys %vars;
	die "unknown var(s) @vars";
}
map { $skip_ascii{$_} = 1; } @regress_testnames if $skip_ascii{ALL};
map { $skip_man{$_} = 1; } @regress_testnames if $skip_man{ALL};
map { $skip_markdown{$_} = 1; } @regress_testnames if $skip_markdown{ALL};

# --- run targets ------------------------------------------------------

my $count_total = 0;
for my $dirname (@subdir_names) {
	$count_total++;
	print "\n" if $targets{verbose};
	system './regress.pl', "$subdir/$dirname", keys %targets,
	    ($displaylevel ? $displaylevel - 1 : 0),
	    and fail $subdir, $dirname, 'subdir';
}

my $count_ascii = 0;
my $count_man = 0;
for my $testname (@regress_testnames) {
	next if $onlytest && $testname ne $onlytest;
	my $i = "$subdir/$testname.in";
	my $o = "$subdir/$testname.mandoc_ascii";
	my $w = "$subdir/$testname.out_ascii";
	if ($targets{ascii} && !$skip_ascii{$testname}) {
		$count_ascii++;
		$count_total++;
		print "@mandoc -T ascii $i\n" if $targets{verbose};
		sysout $o, @mandoc, qw(-T ascii), $i
		    and fail $subdir, $testname, 'ascii:mandoc';
		system @diff, $w, $o
		    and fail $subdir, $testname, 'ascii:diff';
	}
	my $m = "$subdir/$testname.in_man";
	my $mo = "$subdir/$testname.mandoc_man";
	if ($targets{man} && !$skip_man{$testname}) {
		$count_man++;
		$count_total++;
		print "@mandoc -T man $i\n" if $targets{verbose};
		sysout $m, @mandoc, qw(-T man), $i
		    and fail $subdir, $testname, 'man:man';
		print "@mandoc -man -T ascii $m\n" if $targets{verbose};
		sysout $mo, @mandoc, qw(-man -T ascii -O mdoc), $m
		    and fail $subdir, $testname, 'man:mandoc';
		system @diff, $w, $mo
		    and fail $subdir, $testname, 'man:diff';
	}
	if ($targets{clean}) {
		print "rm $o\n"
		    if $targets{verbose} && !$skip_ascii{$testname};
		unlink $o;
		print "rm $m $mo\n"
		    if $targets{verbose} && !$skip_man{$testname};
		unlink $m, $mo;
	}
}

my $count_utf8 = 0;
for my $testname (@utf8_testnames) {
	next if $onlytest && $testname ne $onlytest;
	my $i = "$subdir/$testname.in";
	my $o = "$subdir/$testname.mandoc_utf8";
	my $w = "$subdir/$testname.out_utf8";
	if ($targets{utf8}) {
		$count_utf8++;
		$count_total++;
		print "@mandoc -T utf8 $i\n" if $targets{verbose};
		sysout $o, @mandoc, qw(-T utf8), $i
		    and fail $subdir, $testname, 'utf8:mandoc';
		system @diff, $w, $o
		    and fail $subdir, $testname, 'utf8:diff';
	}
	if ($targets{clean}) {
		print "rm $o\n" if $targets{verbose};
		unlink $o;
	}
}

my $count_html = 0;
for my $testname (@html_testnames) {
	next if $onlytest && $testname ne $onlytest;
	my $i = "$subdir/$testname.in";
	my $o = "$subdir/$testname.mandoc_html";
	my $w = "$subdir/$testname.out_html";
	if ($targets{html}) {
		$count_html++;
		$count_total++;
		print "@mandoc -T html $i\n" if $targets{verbose};
		syshtml $o, @mandoc, qw(-T html), $i
		    and fail $subdir, $testname, 'html:mandoc';
		system @diff, $w, $o
		    and fail $subdir, $testname, 'html:diff';
	}
	if ($targets{clean}) {
		print "rm $o\n" if $targets{verbose};
		unlink $o;
	}
}

my $count_markdown = 0;
for my $testname (@regress_testnames) {
	next if $onlytest && $testname ne $onlytest;
	my $i = "$subdir/$testname.in";
	my $o = "$subdir/$testname.mandoc_markdown";
	my $w = "$subdir/$testname.out_markdown";
	if ($targets{markdown} && !$skip_markdown{$testname}) {
		$count_markdown++;
		$count_total++;
		print "@mandoc -T markdown $i\n" if $targets{verbose};
		sysout $o, @mandoc, qw(-T markdown), $i
		    and fail $subdir, $testname, 'markdown:mandoc';
		system @diff, $w, $o
		    and fail $subdir, $testname, 'markdown:diff';
	}
	if ($targets{clean}) {
		print "rm $o\n" if $targets{verbose};
		unlink $o;
	}
}

my $count_lint = 0;
for my $testname (@lint_testnames) {
	next if $onlytest && $testname ne $onlytest;
	my $i = "$subdir/$testname.in";
	my $o = "$subdir/$testname.mandoc_lint";
	my $w = "$subdir/$testname.out_lint";
	if ($targets{lint}) {
		$count_lint++;
		$count_total++;
		print "@mandoc -T lint $i\n" if $targets{verbose};
		syslint $o, @mandoc, qw(-T lint), $i
		    and fail $subdir, $testname, 'lint:mandoc';
		system @diff, $w, $o
		    and fail $subdir, $testname, 'lint:diff';
	}
	if ($targets{clean}) {
		print "rm $o\n" if $targets{verbose};
		unlink $o;
	}
}

exit 0 unless $displaylevel or @failures;

print "\n" if $targets{verbose};
if ($onlytest) {
	print "test $subdir:$onlytest finished";
} else {
	print "testsuite $subdir finished";
}
print ' ', (scalar @subdir_names), ' subdirectories' if @subdir_names;
print " $count_ascii ascii" if $count_ascii;
print " $count_man man" if $count_man;
print " $count_utf8 utf8" if $count_utf8;
print " $count_html html" if $count_html;
print " $count_markdown markdown" if $count_markdown;
print " $count_lint lint" if $count_lint;

if (@failures) {
	print " (FAIL)\n\nSOME TESTS FAILED:\n\n";
	print "@$_\n" for @failures;
	print "\n";
	exit 1;
} elsif ($count_total == 1) {
	print " (OK)\n";
} elsif ($count_total) {
	print " (all $count_total tests OK)\n";
} else {
	print " (no tests run)\n";
} 
exit 0;
