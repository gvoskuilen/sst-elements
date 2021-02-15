#!/bin/tcsh

foreach o ('' 'O3')
    foreach e ('matmat' 'tmr_matmat' 'dmr_matmat' 'rd_matmat' 'qsort' 'rd_qsort')
        set use_e = $e${o}
        ./runLots.csh $use_e
    end
end
