#!/opt/local/bin/perl -w

sub getFStr($) {
    my $f = shift;

    if ($f eq "0x1") {
        $fStr = "RF";
    } elsif ($f eq "0x2") {
        $fStr = "ID";
    } elsif ($f eq "0x4") {
        $fStr = "MDU";
    } elsif ($f eq "0x8") {
        $fStr = "MEM_PRE_A";
    } elsif ($f eq "0x10") {
        $fStr = "MEM_POST";
    } elsif ($f eq "0x20") {
        $fStr = "WB";
    } elsif ($f eq "0x40") {
        $fStr = "ALU";
    } elsif ($f eq "0x80") {
        $fStr = "MEM_BP";
    } elsif ($f eq "0x100") {
        $fStr = "CONTROL";
    } elsif ($f eq "0x200") {
        $fStr = "INST_ADDR";
    } elsif ($f eq "0x400") {
        $fStr = "INST_TYPE";
    } elsif ($f eq "0x800") {
        $fStr = "WB_ADDR";
    } elsif ($f eq "0x1000") {
        $fStr = "MEM_PRE_D";
    } elsif ($f eq "0x2000") {
        $fStr = "PC";
    } else {
        $fStr = "Unknown Fault". $f;
    }

    return $fStr;
}

sub getNameStr($) {
    my $nameStr = shift;

    $nameStr =~ s/matmat//;
    $nameStr =~ s/_//;
    if ($nameStr eq "") {
        $nameStr = "matmat";
    } elsif ($nameStr eq "O3") {
        $nameStr = "matmatO3";
    }
    return $nameStr;
}

sub calcFailDist($$$$) {
    my ($line, $injPoint, $execFile, $faultLoc) = @_;
    my @ws = split(/ /,$line);
    my $failPoint = $ws[-1];
    my $failDist = $failPoint - $injPoint;
    # bin
    my $useDist = int(log($failDist)/log(2));
    # store
    $failDists{$execFile}{$faultLoc}{$useDist}++;
    $failNum{$execFile}{$faultLoc}++;
    if (defined($failSum{$execFile}{$faultLoc})) {
        $failSum{$execFile}{$faultLoc} += $failDist;
    } else {
        $failSum{$execFile}{$faultLoc} = $failDist;
    }
}


@resCat = ("SDC", "terminated", "Timeout", "CORRECT");


my $dir ="./out";
if (defined($ARGV[0])) {
    $dir = $ARGV[0];
}
my $numP = 0;
my $tNumFound = 0;
my $verbose = 0;
my $debug = 0;  # for debuging results if numFound / somFound don't match

foreach my $fp (glob("$dir/sstOut*")) {
    if ($verbose) {
        printf "processing %s (#%d) found: ", $fp, ++$numP;
    }

    #if ($numP > 200) {last;}
    
    @fp_w = split(/-/,$fp);
    $fp_w[1] =~ s/\..*//;  #remove end
    $execFile = $fp_w[1];
    $isQSort = ($execFile =~ /qsort/);
    if ($verbose) {
        printf(" exec:%s Qs:%d\n", $execFile, $isQSort);
    }

    $done = 0;
    $injPoint = -2;
    my $numFound = 0;


  open my $fh, "<", $fp or die "can't read open '$fp': $OS_ERROR";
  while (defined($line = <$fh>)) {
      chop($line);

      if (!$done && $line =~ /PRINT_INT/) {
          @ws = split(/ /,$line);
          if ($ws[1] != 0) {
              #SDC detected
              $results{$execFile}{$faultLoc}{"SDC"}++;
          } else {
              #finished fine
              $results{$execFile}{$faultLoc}{"CORRECT"}++;
          }
          if ($debug) {printf("C");}
          $done = 1;  # mark done so we only count once
      } elsif (!$done && $line =~ /INJECTING/) {
          @ws = split(/ /,$line);
          if (!($ws[-1] =~ /effect/)) {  # avoid 'INJECTING fault to
                                         # data on load: no effect'
              $injPoint = $ws[-1];
          }
      } elsif (!$done && $line =~ /UNRECOVERABLE ERROR: 65280/) {
          # special case, probably caused by INST_TYPE
          $results{$execFile}{$faultLoc}{"terminated"}++;
          if ($debug) {printf("U");}
          if ($done) {printf("X\n");}
      } elsif (!$done && $line =~ /invalid instruction/) {
          calcFailDist($line, $injPoint, $execFile, $faultLoc);
          $results{$execFile}{$faultLoc}{"terminated"}++;
          if ($debug) {printf("T");}
          if ($done) {printf("X\n");}
      } elsif (!$done && $line =~ /Unknown ex/) {
          calcFailDist($line, $injPoint, $execFile, $faultLoc);
          $results{$execFile}{$faultLoc}{"terminated"}++;
          if ($debug) {printf("T");}
          if ($done) {printf("X\n");}
      } elsif (!$done && $line =~ /terminated/) {
          calcFailDist($line, $injPoint, $execFile, $faultLoc);
          $results{$execFile}{$faultLoc}{"terminated"}++;
          if ($debug) {printf("T");}
          if ($done) {printf("X\n");}
      } elsif (!$done && $line =~ /Timeout/) {
          $results{$execFile}{$faultLoc}{"Timeout"}++;
          if ($debug) {printf("TO");}
          if ($done) {printf("X\n");}
      } elsif ($line =~ /CORRECTED_MATH/) {
          @ws = split(/ /,$line);
          if ($ws[2] != 0) {
              # at least 1 math correction
              $results{$execFile}{$faultLoc}{"MthCs"}++;
              $results{$execFile}{$faultLoc}{"#MthC"} += $ws[2];
          }
      } elsif ($line =~ /WB_ERROR/) {
          @ws = split(/ /,$line);
          if ($ws[2] != 0) {
              # at least 1 WB error correction
              $results{$execFile}{$faultLoc}{"WBEs"}++;
              $results{$execFile}{$faultLoc}{"#WBE"} += $ws[2];
          }
      } elsif ($line =~ /-----------/) {
          # find logical masking (e.g. fault injected but not used)
          if ($injPoint != -2) { #ignore first run
              if ($injPoint == -1) {
                  # fault not injected into relevant part
                  $noInjPoint{$execFile}{$faultLoc}++;
              }
              $injRuns{$execFile}{$faultLoc}++;
          }


          $line =~ s/--*/-/g;
          @ws = split(/-/,$line);
          $faultLoc = $ws[1];
          if ($debug) {printf("\n $faultLoc ");}
          # next run
          $done = 0;
          $injPoint = -1;
          $numFound++;
          $tNumFound++;
      }

  }

  my $sumFound = 0;
  foreach my $r (@resCat) {
      foreach my $fl ("0x1", "0x2", "0x4", "0x8",
                      "0x10", "0x20", "0x40", "0x80",
                      "0x100", "0x200", "0x400", "0x800",
                      "0x1000", "0x2000") {
          if(defined($results{$execFile}{$fl}{$r})) {
              $sumFound += $results{$execFile}{$fl}{$r};
          }
      }
  }
  if ($verbose) {
      printf(" %d / %d (%d total)\n", $numFound, $sumFound, $tNumFound);
  }
  close $fh or die "can't read close '$fp': $OS_ERROR";
}

@statCat = ("MthCs", "#MthC", "WBEs", "#WBE");
@statPer = (1,0,1,0);  # print as % of runs
$printPercent = 1; # print any as percents

printf("                                ");
printf("%7s\t\t", "runs");
foreach $r (@resCat, @statCat) {
    printf("%7s\t", substr($r,0,6));
}
printf("\n");

#print main results
foreach $e (sort keys %results) {
    foreach $f (sort keys %{$results{$e}}) {

        $fStr = getFStr($f);


        $runs{$e} = 0;
        foreach $r (@resCat) {
            if (defined($results{$e}{$f}{$r})) {
                $runs{$e} += $results{$e}{$f}{$r};
            }
        }

        $nameStr = getNameStr($e);
        printf("%10s\t%10s\t%6d\t\t", substr($nameStr,0,10), $fStr, $runs{$e});

        foreach $r (@resCat) {
            if (defined($results{$e}{$f}{$r})) {
                if ($printPercent) {
                    printf("%6.2f%%\t", $results{$e}{$f}{$r}*100.0/$runs{$e});
                } else {
                    printf("%6.0f\t", $results{$e}{$f}{$r});
                }
            } else {
                if ($printPercent) {
                    printf("%6.2f%%\t", 0);
                } else {
                    printf("%6.0f\t", 0);
                }
            }
        }

        $i = 0;
        foreach $r (@statCat) {            
            if (defined($results{$e}{$f}{$r})) {
                if ($statPer[$i] & $printPercent) {
                    printf("%6.2f%%\t", $results{$e}{$f}{$r}*100.0/$runs{$e});
                } else {
                    printf("%6.3f\t", $results{$e}{$f}{$r}/$runs{$e});
                }
            } else {
                if ($statPer[$i] & $printPercent) {
                    printf("%6.2f%%\t", 0);
                } else {
                    printf("%6.0f\t", 0);
                }
            }
            $i++;
        }

        printf("\n");
    }
}

# print fail distance histograms
printf("\nHistograms of fault-to-fail distance:\n");
foreach $e (sort keys %failDists) {
    my $nmStr = getNameStr($e);

    foreach $f (sort keys %{$failDists{$e}}) {

        $fStr = getFStr($f);

        printf("%s %s avg %f\n", $nmStr, $fStr,
               $failSum{$e}{$f}/$failNum{$e}{$f});
        
        foreach $d (sort  {$a <=> $b}keys %{$failDists{$e}{$f}} ) {
            printf("%s %s 2^$d %d\n", $nmStr, $fStr, $failDists{$e}{$f}{$d});
        }
    }
}


#logical masking
printf("\ninjections into parts that are not used:\n");
foreach $e (sort keys %injRuns) {
    foreach $f (sort keys %{$injRuns{$e}}) {
        $fStr = getFStr($f);
        $nameStr = getNameStr($e);

        if (!defined($noInjPoint{$e}{$f})) {
            $noInjPoint{$e}{$f} = 0;
        }
        my $unusedPer = 100.0 * $noInjPoint{$e}{$f} / $injRuns{$e}{$f};
        
        printf("%10s\t%10s\t%6.2f%%\n", substr($nameStr,0,10), $fStr, $unusedPer);

    }
}
    
