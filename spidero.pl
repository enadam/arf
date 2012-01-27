#!/usr/bin/perl -w
#
# spidero.pl -- visualize and analyse libero's output <<<
#
# This script generates graphs and extracts statistical properties of the data
# dumped by libero.  The different analysis options used wise may allow you to
# recognize patterns and locate problematic codepaths.  You can run multiple
# analysis methods at once and you can process multiple logfiles in the same
# run.  Each analysis method places the results in a different output file.
# Graphs are constructed with gnuplot and graphviz and saved in PNG format.
#
# Synopsis:
#   ./spidero.pl [<options>] [-mcnpbst] [<logfile>]...
#
# Options:
# --prefix,   -t <prefix>	Designate a different prefix for the result
#				files.  If unspecified the input file's base
#				name is used, or 'spidero' if the input comed
#				from the standard input.
# --rounds,   -r <round>	Exclude all but results for <round> from the
#				output.  Analysis is still done on all other
#				rounds if the selected analysis methods have
#				some inter-round state.  Multiple <round>s
#				can be specified by repeating this option.
# --no-graph, -G		Instruct the graph-emitting analysis methods
#				to not actually create the graphs but save
#				the grapher's input data.
#
# Analysis methods:
# --memo,    -m		Plot the number of allocated bytes/round.
# --calloc,  -c		Plot the number of allocated chunks/round.
# --numa,    -n		Plot the number of allocation operations/round.
# --ptrsize, -p		Plot the average chunk size/round.
# --baro,    -b		Combines --memo and --calloc in a bar graph.
#			Each bar represents a round, where the bar's width
#			corresponds to the number of allocated chunks in
#			that round and the area of the bar is the number
#			of allocated bytes in that run.
# --spider,  -s		Generate a dot graph for each round, where all leaves
#			are points in the code which allocated memory in the
#			round and the path leading to them is the code path.
#			Numbers in each node help seeing which are the heavy
#			branches in the code.
# --trends,  -t		Helps you to identify memory-growing code paths.
#			Internally creates --memo graphs for all paths
#			and relates them to the overall plot, so you can
#			pick the ones which are mostly responsible for the
#			general memory charactersitic of your program.
#			This is useful when you know that the consumption
#			of your program is growing but it's not leaking.
# --summary, -S		Output the summaries of the selected rounds.
# --cat,     -C		Output the selected rounds.
#
# If no <logfile> name is supplied the standard input is read.  Output files
# are overridden without concern.
# >>>
#

package Spidero; # <<<
use strict;

sub new { bless({ }, shift) }

sub log_started		{ }
sub round_started	{ }
sub process		{ }
sub round_finished	{ }
sub log_finished	{ }
# Spidero >>>

package Spidero::GNUPlot; # <<<
use strict;

our @ISA = qw(Spidero);

sub open_gnuplot
{
	my ($self, $what, $how) = (shift, shift, pop);
	local *GNUPLOT;
	my $graph;

	$graph = lc(ref($self));
	$graph =~ s/^.*:://;
	unless ($main::Opt_nograph)
	{
		open(GNUPLOT, '|-', 'gnuplot');
		print GNUPLOT "set terminal png";
		print GNUPLOT "set output '${main::Prefix}_$graph.png'";
		print GNUPLOT "set key off";
		print GNUPLOT "set title '$what'";
		print GNUPLOT join("\n", @_) if @_;
		print GNUPLOT "plot '-' with $how";
	} else
	{
		open(GNUPLOT, '>', "${main::Prefix}_$graph.data");
	}
	$$self{'gnuplot'} = *GNUPLOT;
}

sub close_gnuplot
{
	my $self = shift;

	print { $$self{'gnuplot'} } 'e'
		unless $main::Opt_nograph;
	close($$self{'gnuplot'});
	delete $$self{'gnuplot'};
}

sub log_finished
{
	shift->close_gnuplot();
}
# Spidero::GNUPlot >>>

package Spidero::Memo; # <<<
use strict;

our @ISA = qw(Spidero::GNUPlot);

sub log_started
{
	shift->open_gnuplot(
		"Number of bytes allocated at the end of rounds",
		"set xlabel 'rounds'",
		"set ylabel 'allocated bytes'",
		"set grid",
		"lines")
}

sub process
{
	my ($self, $line) = @_;
	print { $$self{'gnuplot'} } $1
		if $line =~ /^current allocation:\s*(\d+)/;
}
# Spidero::Memo >>>

package Spidero::CAlloc; # <<<
use strict;

our @ISA = qw(Spidero::GNUPlot);

sub log_started
{
	shift->open_gnuplot(
		"Number of individual memory chunks at the end of rounds",
		"set xlabel 'rounds'",
		"set ylabel 'number of allocated chunks'",
		"set grid",
		"lines")
}

sub process
{
	my ($self, $line) = @_;
	print { $$self{'gnuplot'} } $1
		if $line =~ /^number of allocations:\s*\d+\s*\(currently (\d+)\)/;
}
# Spidero::CAlloc >>>

package Spidero::NumA; # <<<
use strict;

our @ISA = qw(Spidero::GNUPlot);

sub log_started
{
	shift->open_gnuplot(
		"Number of allocations in each round",
		"set xlabel 'rounds'",
		"set ylabel 'number of allocations'",
		"set grid",
		"lines")
}

sub process
{
	my ($self, $line) = @_;
	print { $$self{'gnuplot'} } $1
		if $line =~ /^number of allocations:\s*(\d+)/;

}
# Spidero::NumA >>>

package Spidero::Bloat; # <<<
use strict;

our @ISA = qw(Spidero::GNUPlot);

sub log_started
{
	shift->open_gnuplot(
		"Average chunk size per round",
		"set xlabel 'rounds'",
		"set ylabel 'average chunk size'",
		"set grid",
		"lines")
}

sub round_started
{
	my $self = shift;
	$$self{'ptrsize'} = $$self{'nptr'} = 0;
}

sub process
{
	my ($self, $line) = @_;

	if ($line =~ /^ptr=.*\bsize=(\d+)/)
	{
		$$self{'ptrsize'} += $1;
		$$self{'nptr'}++;
	}
}

sub round_finished
{
	my $self = shift;
	print { $self->{'gnuplot'} } $$self{'nptr'}
		? $$self{'ptrsize'} / $$self{'nptr'} : 0;
}
# Spdero::Bloat >>>

package Spidero::Baro; # <<<
use strict;

our @ISA = qw(Spidero::Bloat);

my ($Cursor, @Points, @Tics);

sub log_started
{
	$Cursor = 0;
	@Points = @Tics = ();
}

sub round_finished
{
	my $self = shift;
	push(@Points, [ $Cursor+$$self{'nptr'},
			$$self{'ptrsize'}/$$self{'nptr'} ])
		if $$self{'nptr'};
	push(@Tics,   [ $main::Round, $Cursor+$$self{'nptr'}/2 ]);
	$Cursor += $$self{'nptr'};
}

sub log_finished
{
	my $self = shift;

	# Construct reasonable xtics.  Keep at most 12 tics (rounds)
	# and try to distribute the holes uniformly like this:
	# 1 2 3 4 5 6 7 8 9 10 11 12
	# 1 2 3 4 5 6   8 9 10 11 12 13
	# 1 2 3 4   6 7 8 9    11 12 13 14
	# 1 2 3   5 6 7   9 10 11    13 14 15
	# 1 2 3   5 6   8 9    11 12    14 15 16
	# 1 2   4 5   7 8   10 11    13 14    16 17

	return if !@Points;
	@Tics[map(int(@Tics/(@Tics-11) * $_), 1..@Tics-12)] = undef;
	@Tics = grep(defined, @Tics);
	$self->open_gnuplot(
		"Allocated memory/number of allocations/rounds",
		"set xtics (".join(', ', map("'$$_[0]' $$_[1]", @Tics)).")",
		"set xlabel 'rounds'",
		"unset ylabel", "unset ytics",
		"set grid",
		"boxes");
	print { $$self{'gnuplot'} } @$_ foreach @Points;
	$self->close_gnuplot();
}
# Spidero::Baro >>>

package Spidero::Spider; # <<<
use strict;

# @Branch:		the current backtrace, flushed by grow()
# $Branch_weight:	the ptrsize's held by that code path
# %Pool:		branches of the current round, represented in a tree
#       		and added weight per node
our @ISA = qw(Spidero);
our (%Pool, @Branch, $Branch_weight);

# Add @Branch to %Pool.
sub grow
{
	my ($site, %seen, $parent);

	while (defined ($site = shift(@Branch)))
	{
		my ($addr, $show) = @$site;
		my $node;

		defined ($node = $Pool{$addr})
			or $node = $Pool{$addr} = [ $show, 0, { } ];
		$$parent{$addr}++ if defined $parent;

		if (!$seen{$node})
		{	# Don't account for $size more than once.
			$$node[1] += $Branch_weight;
			$seen{$node} = 1;
		}

		$parent = $$node[2];
	}

	$Branch_weight = 0;
}

# Output %Pool as a dot-graph.
sub cut
{
	my ($addr, $node);

	local *DOT;
	$main::Opt_nograph
		? open(DOT, '>', "${main::Prefix}_spider-$main::Round.dot")
		: open(DOT, '|-', qw(dot -Tpng),
			"-o${main::Prefix}_spider-$main::Round.png");

	print DOT "digraph round_$main::Round";
	print DOT "{";
	while ((($addr, $node) = each(%Pool)))
	{
		print DOT "\t\"$addr\" [label=\"$$node[0]\\n$$node[1] B\"];";
		print DOT "\t\"$addr\" -> \"$_\";" foreach keys(%{$$node[2]});
	}
	print DOT "}\n";

	close(DOT);
}

sub round_started
{
	%Pool = ();
}

# Collect ptrsizes for the backtrace then shallow the backtrace itself,
# grow()ing the %Pool.
sub process
{
	my ($self, $line) = @_;

	if ($line =~ /^ptr=.*\bsize=(\d+)\b/)
	{
		grow() if @Branch;
		$Branch_weight += $1;
	} elsif ($line =~ s/^\s*\d+\.\s+//)
	{
		my (@parts, $isaddr, $show, $addr);

		# dso, [<addr>]			disp: dso, addr; addr=addr
		# dso, [<fun>]			disp: dso, fun; addr=fun
		# dso, src:line [<addr>]	disp: dso, src:line; addr=addr
		# dso, src:line [<fun>]		disp: dso, src:line, fun;
		#					addr=src:line

		@parts = split(/\s+/, $line);
		$isaddr = ($parts[-1] =~ s/\[(0x[[:xdigit:]]+)\]$/$1/);
		$show = @parts == 2 || !$isaddr
			? join('\n', @parts)
			: join('\n', @parts[0,1]);
		$addr = @parts == 2 || $isaddr
			? $parts[-1] : $parts[1];
		unshift(@Branch, [ $addr, $show ]);
	}
}

sub round_finished
{
	grow();
	cut() if %Pool;
}
# Spidero::Spider >>>

package Spidero::Trendy; # <<<
use strict;
use Digest::MD5 qw(md5_hex);

our @ISA = qw(Spidero);

# Declare some variables as ours rather than mine because lexical variables
# span across package boudaries and would collide with Spidero::Spider.
#
# $Branch:		the backtrace as text, used as key in %Branches
# $Branch_size:		the number of ptr:s this the current branch has
# $Branch_weight:	the aggregated ptrsize:s the current branch has
# %Branches:		backtrace -> [ [ $main::Round, $Branch_size,
#				$Branch_weight ], ... ] association
my %Branches;
our ($Branch, $Branch_size, $Branch_weight);

sub log_started
{
	%Branches = ()
}

sub round_started
{
	$Branch_size = $Branch_weight = 0;
	$Branch = '';
}

sub grow
{
	my $points;

	defined ($points = $Branches{$Branch})
		or $points = $Branches{$Branch} = [ ];
	push(@$points, [ $main::Round, $Branch_size, $Branch_weight ]);

	round_started();
}

# Get $Branch_size and $Branch_weight, then $Branch, then store then
# in %Branches.
sub process
{
	my ($self, $line) = @_;

	if ($line =~ /^ptr=.*\bsize=(\d+)\b/)
	{
		grow() if $Branch ne '';
		$Branch_size++;
		$Branch_weight += $1;
	} elsif ($line =~ /^\s*\d+\.\s+/)
	{
		$Branch .= $line;
	}
}

sub round_finished
{
	grow()
}

sub log_finished
{
	my ($branch, $points);

	# Calculate the trend line for each $branch.
	open(TRENDS, '>', "$main::Prefix.trends");
	while ((($branch, $points) = each(%Branches)))
	{
		my ($alpha, $beta, $error);

		# The trend line will be $alpha*x + $beta.  The higher
		# the BETA and the lower the ERROR the branch is more
		# likely to be responsible for the overall trend.
		if (@$points > 1)
		{
			my ($n, $x, $y, $xy, $x2);

			$x = $y = $xy = $x2 = 0;
			foreach (@$points)
			{
				$x  += $$_[0];
				$y  += $$_[2];
				$xy += $$_[0] * $$_[2];
				$x2 += $$_[0] * $$_[0];
			}

			$n = @$points;
			$beta  = ($xy*$n - $x*$y) / ($x2*$n - $x*$x);
			$alpha = $y/$n - $beta * $x/$n;
			$error += ($$_[2] - ($beta*$$_[0] + $alpha))**2
				foreach @$points;
		} else
		{
			$alpha = $$points[0][2];
			$beta  = 0;
			$error = 0;
		}

		# Include a unique identifier of the branch to make it
		# easier to grep the output.
		print TRENDS $branch;
		print TRENDS "ID=" . md5_hex($branch);
		print TRENDS "BETA=$beta";
		print TRENDS "ERROR=$error";
		print TRENDS "TREND=$beta*x"
			. ($alpha < 0 ? '' : '+') . $alpha;
		print TRENDS '';
	}
	close(TRENDS);
}
# Spidero::Trendy >>>

package Spidero::Summary; # <<<
use strict;

our @ISA = qw(Spidero);
our $In_headers;

sub log_started
{
	my $self = shift;
	local *OUT;
	my $what;

	$what = lc(ref($self));
	$what =~ s/^.*:://;
	open(OUT, '>', "$main::Prefix.$what");
	$$self{'out'} = *OUT;
}

sub print_line
{
	local $\ = "";
	print { shift->{'out'} } shift
}

sub round_started
{
	shift->print_line(shift);
	$In_headers = 1;
}

sub round_finished
{
	my ($self, $line) = @_;
	$self->print_line($line) if defined $line;
}

sub process
{
	my ($self, $line) = @_;

	return if !$In_headers;
	if ($line =~ /^$/)
	{
		$In_headers = 0;
	} else
	{
		$self->print_line($line);
	}
}

sub log_finished
{
	close(delete shift->{'out'})
}
# >>>

package Spidero::Cat; # <<<
use strict;

our @ISA = qw(Spidero::Summary);

sub process
{
	shift->print_line(shift)
}
# >>>

package main; # <<<
use strict;
use Getopt::Long;

our ($Prefix, $Round, $Round_skipped);
our $Opt_nograph;

my (@opt_rounds, $opt_prefix);
my ($opt_graph_memo, $opt_graph_calloc, $opt_graph_numa);
my ($opt_graph_bloat, $opt_graph_baro, $opt_graph_spider);
my ($opt_trendy, $opt_summary, $opt_cat);
my (@all_tasks, @tasks, @rounds);

Getopt::Long::Configure(qw(bundling no_ignore_case no_getopt_compat));
exit 1 unless GetOptions(
	'prefix|T=s'	=> \$opt_prefix,
	'rounds|r=i@'	=> \@opt_rounds,
	'no-graph|G'	=> \$Opt_nograph,
	'memo|m'	=> \$opt_graph_memo,
	'calloc|c'	=> \$opt_graph_calloc,
	'numa|n'	=> \$opt_graph_numa,
	'ptrsize|p'	=> \$opt_graph_bloat,
	'baro|b'	=> \$opt_graph_baro,
	'spider|s'	=> \$opt_graph_spider,
	'trends|t'	=> \$opt_trendy,
	'summary|S'	=> \$opt_summary,
	'cat|C'		=> \$opt_cat);

@opt_rounds = sort({ $a <=> $b } @opt_rounds);
push(@all_tasks, Spidero::Memo->new())		if $opt_graph_memo;
push(@all_tasks, Spidero::CAlloc->new())	if $opt_graph_calloc;
push(@all_tasks, Spidero::NumA->new())		if $opt_graph_numa;
push(@all_tasks, Spidero::Baro->new())		if $opt_graph_baro;
push(@all_tasks, Spidero::Bloat->new())		if $opt_graph_bloat;
push(@all_tasks, Spidero::Spider->new())	if $opt_graph_spider;
push(@all_tasks, Spidero::Trendy->new())	if $opt_trendy;
push(@all_tasks, Spidero::Summary->new())	if $opt_summary;
push(@all_tasks, Spidero::Cat->new())		if $opt_cat;

die "Nothing to do" if !@all_tasks;
warn "Prefix will be ignored for the second and subsequent files"
	if defined $opt_prefix && @ARGV > 1;

# The main loop: scan the input line by line and emit events on @tasks
# to signal start/end of log/round and other lines.  Decide at the start
# of round about the active @tasks and don't bother the other if the round
# is to be skipped.
$\ = "\n";
$, = " ";
while (<>)
{
	my $line = $_;

	if (!defined $Prefix)
	{	# First line of the file.
		if (defined $opt_prefix)
		{
			$Prefix = $opt_prefix;
			undef $opt_prefix;
		} elsif ($ARGV ne '-')
		{
			$Prefix = $ARGV;
			$Prefix =~ s/^(.*)\..*/$1/;
		} else
		{
			$Prefix = 'spidero';
		}

		$_->log_started() foreach @all_tasks;
		@rounds = @opt_rounds;
	}

	if (/^report (\d+) created on\b/)
	{	# Start of round.
		$Round = $1;
		if (($Round_skipped = @rounds && $rounds[0] != $Round))
		{
			print STDERR "Round $Round... (skipping)";
			@tasks = grep($$_{'stateful'}, @all_tasks);
		} else
		{
			print STDERR "Round $Round...";
			@tasks = @all_tasks;
			shift(@rounds);
		}
		$_->round_started($line) foreach @tasks;
	} elsif (/^-+$/)
	{	# End of round.
		$_->round_finished($line) foreach @tasks;
		undef $Round;

		# Move onto the next file if last
		# non-skipped round has been processed.
		!@opt_rounds || @rounds
			or close(ARGV);
	} elsif (defined $Round)
	{
		$_->process($line) foreach @tasks;
	}
} continue
{
	if (eof)
	{	# End of current file.
		if (defined $Round)
		{	# Report ended prematurely.
			$_->round_finished() foreach @tasks;
			undef $Round;
		}

		$_->log_finished() foreach @all_tasks;
		undef $Prefix;
		close(ARGV);
	}
}
# main >>>

# vim: set foldmarker=<<<,>>> foldmethod=marker:
# End of spidero.pl
