#!/usr/bin/env perl

use POSIX;
use warnings;
use strict;

my %makeoptions = (
	"CC" => "cc",
	"CFLAGS" => "",
	"AS" => "as",
	"ASFLAGS" => "",
	"LD" => "ld",
	"LDFLAGS" => "",
	"AR" => "ar",
	"ARFLAGS" => "",
	"OBJCOPY" => "objcopy",
	"OCFLAGS" => "",
	"STRIP" => "strip",
	"CPP" => "cpp",
	"CPPFLAGS" => "",
	"KERNEL" => "kernel",
	"FLOPPY" => "floppy.img",
	);
my %options = ();
my @files = ();
my @devices = ();
my @devhashes = ();
my @locators = ();
my $ident = "";
my $machine = "";
my $os_date = ctime(time());

$os_date =~ s/\r?\n$//;

open(IN, "<", "sys/include/sys/device.h");
while(my $l = <IN>){
	$l =~ s/\r?\n//g;

	if($l =~ /locator: ([^ ]+)/){
		push(@locators, $1);
	}
}
close(IN);

sub parse_config {
	open(my $fh, "<", $_[0]) or die;
	while(my $l = <$fh>){
		$l =~ s/[\r\n]//g;
		my @tokens = $l =~ /("(?:\\.|[^"\\])*"|[^\s"]+)/g;
		my $discard = 0;

		map { s/^"(.*)"$/$1/g } @tokens;
		map { s/\\(.)/$1/g } @tokens;
		@tokens = grep {
			if($discard){
				0;
			}else{
				my $m = $_ =~ /#/;

				if($m){
					$discard = 1;
				}

				!$m;
			}
		} @tokens;
		map { s/#.+// } @tokens;

		if(@tokens == 0){
			next;
		}

		if(($tokens[0] eq "options") or ($tokens[0] eq "makeoptions")){
			my $key = $tokens[1];
			my $value = $tokens[2];

			$key =~ s/=$//;
			if($tokens[0] eq "options"){
				$options{$key} = $value;
			}else{
				$makeoptions{$key} = $value;
			}
		}elsif($tokens[0] eq "include"){
			if(-f "sys/" . $tokens[1]){
				parse_config("sys/" . $tokens[1]);
			}
		}elsif($tokens[0] eq "file"){
			if(!$tokens[2] or (grep /^$tokens[2]$/, @devices)){
				push(@files, "sys/" . $tokens[1]);
			}
		}elsif($tokens[0] eq "ident"){
			$ident = $tokens[1];
		}elsif($tokens[0] eq "machine"){
			$machine = $tokens[1];
		}elsif($tokens[1] eq "at"){
			my $dev = $tokens[0];
			$dev =~ s/([0-9]+)$//;
			my $unit = $1;

			my $parent = $tokens[2];
			$parent =~ s/([0-9]+)$//;
			my $parent_unit = $1;

			if(!(grep /^$dev$/, @devices)){
				push(@devices, $dev);
			}

			my @dev_locators = ();

			for(my $i = 0; $i < @locators; $i++){
				push(@dev_locators, "DEVICE_LOCATOR_UNSET");
			}

			for(my $i = 3; $i < @tokens; $i++){
				if(grep(/^$tokens[$i]$/, @locators)){
					my $index = 0;
					for(; $index < @locators; $index++){
						if($locators[$index] eq $tokens[$i]){
							last;
						}
					}
					$dev_locators[$index] = $tokens[$i + 1];
					$i++;
					next;
				}
			}

			push(@devhashes, {
				"dev" => $dev,
				"unit" => $unit,
				"parent" => $parent,
				"parent_unit" => $parent_unit,
				"locators" => \@dev_locators
			});
		}
	};

	close($fh);
}

parse_config($ARGV[0]);

if(!$ident){
	print("ident is missing\n");
	die;
}

if(!$machine){
	print("machine is missing\n");
	die;
}

if(!grep /^mainbus$/, @devices){
	print("sure you do not need main bus?\n");
	die;
}

open(MKCFG, ">", "compile/Makefile.config");
foreach my $makeoption (sort { $a cmp $b } keys(%makeoptions)){
	print(MKCFG "$makeoption = $makeoptions{$makeoption}\n");
}
foreach my $option (sort { $a cmp $b } keys(%options)){
	print(MKCFG "KCFLAGS += -D$option");
	if($options{$option}){
		print(MKCFG "=$options{$option}");
	}
	print(MKCFG "\n");
}
close(MKCFG);

open(FILES, ">", "compile/files.kernel");
close(FILES);
foreach my $file (@files){
	system("echo \"$file\" | ./tools/makedep.sh sys >> compile/files.kernel");
}

open(IOCONF, ">", "compile/sys/ioconf.c");
print(IOCONF "#include <sys/device.h>\n");
print(IOCONF "#include <sys/types.h>\n");
print(IOCONF "\n");
print(IOCONF "#include <stddef.h>\n");
print(IOCONF "\n");
foreach my $device (@devices){
	print(IOCONF "extern struct device_driver ${device}_driver;\n");
}
print(IOCONF "\n");
print(IOCONF "struct device_config devices[] = {\n");
foreach my $device (@devhashes){
	print(IOCONF "	{\n");
	print(IOCONF "		\"$device->{dev}\",\n");
	print(IOCONF "		$device->{unit},\n");
	print(IOCONF "		\"$device->{parent}\",\n");
	print(IOCONF "		$device->{parent_unit},\n");
	print(IOCONF "		&$device->{dev}_driver,\n");
	print(IOCONF "		{\n");
	foreach my $locator (@{$device->{locators}}){
		if($locator eq "DEVICE_LOCATOR_UNSET"){
			print(IOCONF "			$locator,\n");
		}else{
			print(IOCONF "			PADDR_C($locator),\n");
		}
	}
	print(IOCONF "		}\n");
	print(IOCONF "	},\n");
}
print(IOCONF "	{\n");
print(IOCONF "		NULL\n");
print(IOCONF "	}\n");
print(IOCONF "};\n");
close(IOCONF);

open(OSINFO, ">", "compile/sys/osinfo.c");
print(OSINFO "#include <sys/osinfo.h>\n");
print(OSINFO "\n");
print(OSINFO "const char* os_ident = \"$ident\";\n");
print(OSINFO "const char* os_machine = \"$machine\";\n");
print(OSINFO "const char* os_date = \"$os_date\";\n");
print(OSINFO "const char* os_version = OS_VERSION;\n");
close(OSINFO);
