#!/usr/bin/env perl

=head1 NAME
C<GroundLift::OBEXHeaderIdEnumGenerator> - Generates the OBEX header ID enum.
=cut

package GroundLift::OBEXHeaderIdEnumGenerator;

use strict;
use warnings;
use autodie;
use utf8;

=head1 CONFIGURATION PARAMETERS

=over 4

=item I<$enum_name>

Name of the enum typedef.

=cut

our $enum_name = "obex_header_id_t";

=item I<$item_prefix>

Prefix string for each one of the enum items.

=cut

our $item_prefix = "OBEX_HEADER_";

=item I<@headers>

List of valid headers. Each item contains a hash reference with the header I<id>
(including the encoding bits) and a human-readable header I<name>.

=cut

our @headers = (
	{
		id => 0xC0,
		name => "Count"
	},
	{
		id => 0x01,
		name => "Name"
	},
	{
		id => 0x42,
		name => "Type"
	},
	{
		id => 0xC3,
		name => "Length"
	},
	{
		id => 0x44,
		name => "Time"
	},
	{
		id => 0xC4,
		name => "Time Obsolete"
	},
	{
		id => 0x05,
		name => "Description"
	},
	{
		id => 0x46,
		name => "Target"
	},
	{
		id => 0x47,
		name => "HTTP"
	},
	{
		id => 0x48,
		name => "Body"
	},
	{
		id => 0x49,
		name => "End Body"
	},
	{
		id => 0x4A,
		name => "Who"
	},
	{
		id => 0xCB,
		name => "Conn ID"
	},
	{
		id => 0x4C,
		name => "App Params"
	},
	{
		id => 0x4D,
		name => "Auth Challenge"
	},
	{
		id => 0x4E,
		name => "Auth Response"
	},
	{
		id => 0xCF,
		name => "Creator ID"
	},
	{
		id => 0x50,
		name => "WAN UUID"
	},
	{
		id => 0x51,
		name => "Obj Class"
	},
	{
		id => 0x52,
		name => "Session Params"
	},
	{
		id => 0x93,
		name => "Session Seq Num"
	},
	{
		id => 0x94,
		name => "Action ID"
	},
	{
		id => 0x15,
		name => "Dest Name"
	},
	{
		id => 0xD6,
		name => "Permissions"
	},
	{
		id => 0x97,
		name => "SRM"
	},
	{
		id => 0x98,
		name => "SRM Params"
	}
);

=back

=head1 METHODS

=over 4

=item C<main>()

The script's main entry point.

=cut

sub main {
	# Typedef declaration.
	print "typedef enum {\n";

	# Go through the header identifiers to be declated in the enum.
	foreach my $item (@headers) {
		# Print the actual declaration of an item.
		print "\t" . header_name($item) . " = " . as_hex($item->{id});

		# Are there any more items left?
		if (\$item != \$headers[-1]) {
			print ",";
		}

		print "\n";
	}

	# Closing and enum name definition.
	print "} $enum_name;\n";
}

=item I<$name> = C<header_name>(I<\%item>)

Constructs a header name from a header item in the L<@headers> list.

=cut

sub header_name {
	my ($item) = @_;

	# Build up a C enum item compatible name.
	my $name = uc($item->{name});
	$name =~ tr/ ]/_/;

	return $item_prefix . $name;
}

=item I<$hex> = C<as_hex>(I<num>)

Converts a number its 2-character hexadecimal string representation.

=cut

sub as_hex {
	my ($num) = @_;
	return sprintf("0x%02X", $num);
}

# Call the main entry point.
main();

__END__

=back

=head1 AUTHOR

Nathan Campos <nathan@innoveworkshop.com>

=head1 COPYRIGHT

Copyright (c) 2023- Nathan Campos.

=cut
