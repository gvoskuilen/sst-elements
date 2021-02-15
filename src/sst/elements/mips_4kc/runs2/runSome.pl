#!/usr/bin/perl -w

# ./runSome.pl <execFile> <outputFile>


# name is the fault location and 'D' for data, 'C' for control, 'I'
# for special INST_ADDR case
%probabilities = (    
    "0x40D" => 64, #ALU	64
    #"" => , #CONTROL	52
    "0x200I" => 40, #INST_ADDR	data
    "0x200C" => 16, #INST_ADDR	control
    "0x400d" => 32, #INST_TYPE - data	
    "0x400C" => 5, #INST_TYPE - control	
    "0x4D" => 233, #MDU - data	
    "0x4C" => 122, #MDU - control	
    "0x10D" => 160, #MEM_POST -data	
    "0x10C" => 8, #MEM_POST - control	
    "0x8D" => 203, #MEM_PRE_ADDR -data
    "0x8C" => 48, #MEM_PRE_ADDR -control
    "0x8B" => 16, #MEM_PRE_ADDR -byte	
    "0x1000D" => 256, #MEM_PRE_DATA -data	
    "0x1000C" => 8, #MEM_PRE_DATA -control	
    "0x1000B" => 23, #MEM_PRE_DATA -byte	
    "0x1D" => 992, #RF data	992
    "0x800D" => 5, #RF control (WB_ADDR)	
    "0x80D" => 32, #MEM_bp_val -data	
    "0x20D" => 32, # WB
    "0x2000d" => 64 #PC
    );
$maxChance = 0;

#initialize the probability table
sub initProb() {
    my $sum = 0;
    foreach my $k (sort keys %probabilities) {
        #printf("%s: %d ", $k, $probabilities{$k});
        $sum += $probabilities{$k};
        $probabilities{$k} = $sum;
        #printf("%d\n", $probabilities{$k});
    }
    $maxChance = $sum+1;
}

sub getFault() {
    my $roll = int(rand($maxChance));
    my $f = "0x1";  # fault type
    $b = "0"; # bits to flip (0= choose one random)

    # see where we fall
    foreach my $k (sort keys %probabilities) {
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
        printf("Bad bit flip type: $f $type $roll !!!!!!!\n");
        exit(-1);
    }


    
    return ($f, $b);
}

#"main"
$i = 0;
$e = $ARGV[0];
$file = $ARGV[1];
srand();
initProb();
while ($i < 5) {
    my ($f, $b) = getFault();
    $str = "sst ../test_by_time.py -- -e $e -f $f -b $b";
    if (0) { # test
        printf("$str\n");
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


