#!/usr/bin/perl -w

# ./runSome.pl <execFile> <faultLoc> <outputFile>


# name is the fault location and 'D' for data, 'C' for control, 'I'
# for special INST_ADDR case
%probabilities = (    
    "0x40_D" => 64, #ALU	64
    #"" => , #CONTROL	52
    "0x200_I" => 40, #INST_ADDR	data
    "0x200_C" => 16, #INST_ADDR	control
    "0x400_d" => 32, #INST_TYPE - data	
    "0x400_C" => 5, #INST_TYPE - control	
    "0x4_D" => 233, #MDU - data	
    "0x4_C" => 122, #MDU - control	
    "0x10_D" => 160, #MEM_POST -data	
    "0x10_C" => 8, #MEM_POST - control	
    "0x8_D" => 203, #MEM_PRE_ADDR -data
    "0x8_C" => 48, #MEM_PRE_ADDR -control
    "0x8_B" => 16, #MEM_PRE_ADDR -byte	
    "0x1000_D" => 256, #MEM_PRE_DATA -data	
    "0x1000_C" => 8, #MEM_PRE_DATA -control	
    "0x1000_B" => 23, #MEM_PRE_DATA -byte	
    "0x1_D" => 992, #RF data	992
    "0x800_D" => 5, #RF control (WB_ADDR)	
    "0x80_D" => 32, #MEM_bp_val -data	
    "0x20_D" => 32, # WB
    "0x2000_d" => 64 #PC
    );

sub getFault($) {
    my $f = shift;  # fault type
    my $of = $f;
    $b = "0"; # bits to flip (0= choose one random)

    # make subset of the probabilities for our fault type
    my @subset_keys = grep /${f}_/, keys %probabilities;

    #find probability
    my $maxChance = 0;
    for my $k (sort @subset_keys) {
        $maxChance += $probabilities{$k};
        $probabilities{$k} = $maxChance;
        #printf("- %s %d\n", $k, $maxChance);
    }

    my $roll = int(rand($maxChance));
    for my $k (sort @subset_keys) {
        if ($roll <= $probabilities{$k}) {
            $f = $k;
            last;
        }
    }

    #determine which bits to flip
    my $type = chop($f);
    if ($type eq 'I') {
        # try to get a 'valid' instruction which is still the wrong
        # instruction. Pick one bit in 0b0011111100
        my $roll2 = int(rand(6));
        my $b = 1 << ($roll2 + 2);
        #printf("I $roll2 %x\n", $b);
    } elsif ($type eq 'B') {
        # pick one byte
        my $roll2 = int(rand(4));
        my $b = 0xff << ($roll2 * 4);
    } elsif ($type eq 'd') {
        # pick one byte
        my $roll2 = int(rand(30));
        my $b = 1 << ($roll2 + 2);
    } elsif ($type eq 'D') {
        $b = 0;
    } elsif ($type eq 'C') {
        $b = int(rand(0xffffffff));
    } else {
        printf("Bad bit flip type: $f $of $type $roll !!!!!!!\n");
        exit(-1);
    }
    
    return $b;
}

#"main"
$i = 0;
$e = $ARGV[0];
$f = $ARGV[1];
$file = $ARGV[2];
srand();
while ($i < 9) {
    my $b = getFault($f);
    $str = "sst ../test_by_event.py -- -e $e -f $f -b $b";
    if (0) { # test
        printf("$str\n");
        printf("$file\n");
    } else {
        system("echo \"---------------------$f-$b-$e\" 1>>${file} 2>&1");
        $ret = system("${str} 1>>${file} 2>&1");
        if ($ret != 0) {
            # got an error
            printf("Err %d\n", $ret);
            system("echo \"UNRECOVERABLE ERROR: ".$ret."\" 1>>${file} 2>&1");
        }
    }
    $i++;
}


