#!/bin/tcsh

mkdir out_e

foreach f ('0x1' '0x4' '0x8' '0x10' '0x20' '0x40' '0x80' '0x200' '0x400' '0x800' '0x1000' '0x2000')
    foreach o ('' 'O3')
        foreach e ('matmat' 'tmr_matmat' 'dmr_matmat' 'rd_matmat' 'qsort' 'rd_qsort')
            set use_e = $e${o}
            ./runLots_e.csh $use_e $f
        end
    end
end
