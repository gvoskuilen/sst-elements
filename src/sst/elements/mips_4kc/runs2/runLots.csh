#!/bin/tcsh

# ./runLots.csh <execBase>

set i = 0
set e = "../test/$1.out"  #executable file
while ($i < 7)
    date
    set ii = 0
    while ($ii < 12)  # run 8 in parallel
        set tempfoo = "out/sstOut-$1"
        set TMPFILE = `mktemp ${tempfoo}.XXXXX` || exit 1
#        set str = "sst ../test_by_time.py -- -e $e -f $f"
        echo $i $ii $e $TMPFILE
#        $str >> ${TMPFILE} &
        ./runSome.pl $e $TMPFILE &
        @ ii++
    end
    wait

    @ i++
end
