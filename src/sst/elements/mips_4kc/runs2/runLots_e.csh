#!/bin/tcsh

# ./runLots.csh <execBase> <fault>

set i = 0
set e = "../test/$1.out"  #executable file
set f = $2
while ($i < 1)
    date
    set ii = 0
    while ($ii < 12)  # run x in parallel
        set tempfoo = "out_e/sstOut-$1"
        set TMPFILE = `mktemp ${tempfoo}.XXXXX` || exit 1
#        set str = "sst ../test_by_time.py -- -e $e -f $f"
        echo $i $ii $e $TMPFILE
#        $str >> ${TMPFILE} &
        ./runSome_e.pl $e $f $TMPFILE &
        @ ii++
    end
    wait

    @ i++
end
