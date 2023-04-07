#!/usr/bin/env perl

=head1 NAME
C<GroundLift::OBEXEnumGenerator> - Generates OBEX enumerators.
=cut

package GroundLift::OBEXEnumGenerator;

use strict;
use warnings;
use autodie;
use utf8;

=head1 ENUMERATOR DEFINITIONS

=over 4

=item I<\%opcodes>

Enum generation hash containing a list of valid OBEX standard opcodes. Each item
contains a hash reference with the opcode (I<value>) including the final bit set
whenever applicable and a human-readable I<name>.

=cut

our $opcodes = {
	name => "obex_opcodes_t",
	prefix => "OBEX_OPCODE_",
	items => [
		{
			name => "Connect",
			value => 0x80
		},
		{
			name => "Disconnect",
			value => 0x81
		},
		{
			name => "Put",
			value => 0x02
		},
		{
			name => "Get",
			value => 0x03
		},
		{
			name => "Reserved",
			value => 0x04
		},
		{
			name => "SetPath",
			value => 0x85
		},
		{
			name => "Action",
			value => 0x06
		},
		{
			name => "Session",
			value => 0x87
		},
		{
			name => "Abort",
			value => 0xFF
		}
	]
};

=item I<\%headers>

Enum generation hash containing a list of valid OBEX standard headers. Each item
contains a hash reference with the header ID (I<value>) including the encoding
bits and a human-readable header I<name>.

=cut

our $headers = {
	name => "obex_header_id_t",
	prefix => "OBEX_HEADER_",
	items => [
		{
			value => 0xC0,
			name => "Count"
		},
		{
			value => 0x01,
			name => "Name"
		},
		{
			value => 0x42,
			name => "Type"
		},
		{
			value => 0xC3,
			name => "Length"
		},
		{
			value => 0x44,
			name => "Time"
		},
		{
			value => 0xC4,
			name => "Time Obsolete"
		},
		{
			value => 0x05,
			name => "Description"
		},
		{
			value => 0x46,
			name => "Target"
		},
		{
			value => 0x47,
			name => "HTTP"
		},
		{
			value => 0x48,
			name => "Body"
		},
		{
			value => 0x49,
			name => "End Body"
		},
		{
			value => 0x4A,
			name => "Who"
		},
		{
			value => 0xCB,
			name => "Conn ID"
		},
		{
			value => 0x4C,
			name => "App Params"
		},
		{
			value => 0x4D,
			name => "Auth Challenge"
		},
		{
			value => 0x4E,
			name => "Auth Response"
		},
		{
			value => 0xCF,
			name => "Creator ID"
		},
		{
			value => 0x50,
			name => "WAN UUID"
		},
		{
			value => 0x51,
			name => "Obj Class"
		},
		{
			value => 0x52,
			name => "Session Params"
		},
		{
			value => 0x93,
			name => "Session Seq Num"
		},
		{
			value => 0x94,
			name => "Action ID"
		},
		{
			value => 0x15,
			name => "Dest Name"
		},
		{
			value => 0xD6,
			name => "Permissions"
		},
		{
			value => 0x97,
			name => "SRM"
		},
		{
			value => 0x98,
			name => "SRM Params"
		}
	]
};

=back

=head1 METHODS

=over 4

=item C<main>()

The script's main entry point.

=cut

sub main {
	if (scalar(@ARGV) == 0) {
		# Print all our enum definitions.
		print_enum($headers);
		print "\n";
	} elsif ($ARGV[0] eq "headers") {
		print_enum($headers);
	} elsif ($ARGV[0] eq "opcodes") {
		print_enum($opcodes);
	} else {
		print "Invalid enum identifier.\n";
		print "Valid identifiers: headers, opcodes\n";
	}
}

=item C<print_enum>(I<\%enum_def>)

Prints a C enum definition based on a configured enum definition in this script.

=cut

sub print_enum {
	my ($enum_def) = @_;

	# Typedef declaration.
	print "typedef enum {\n";

	# Go through the items to be declated in the enum.
	foreach my $item (@{$enum_def->{items}}) {
		# Print the actual declaration of an item.
		print "\t" . item_name($enum_def->{prefix}, $item) . " = " .
			as_hex($item->{value});

		# Are there any more items left?
		if (\$item != \$enum_def->{items}[-1]) {
			print ",";
		}

		print "\n";
	}

	# Closing and enum name definition.
	print "} " . $enum_def->{name} . ";\n";
}

=item I<$name> = C<item_name>(I<$prefix>, I<\%item>)

Constructs an enum item name from an enum I<$item> in the enum definition and a
name I<$prefix>.

=cut

sub item_name {
	my ($prefix, $item) = @_;

	# Build up a C enum item compatible name.
	my $name = uc($item->{name});
	$name =~ tr/ ]/_/;

	return $prefix . $name;
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
