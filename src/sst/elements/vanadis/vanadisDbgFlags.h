// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_DBG_FLAGS
#define _H_VANADIS_DBG_FLAGS

// Masks allow categorizing output by type (32b mask)

// Levels define how much information
// Level 1: High level information that confirms simulation parameters, config, etc.
// Level 2: Warnings that are likely not errors but could be
// Level 3:
// Per-cycle basic status per thread
#define VANADIS_VERB_STATUS CALL_INFO, 4, 0
// Reports pipeline actions taken
#define VANADIS_VERB_PIPELINE    CALL_INFO, 5, 0
// Reports pipeline actions detail
#define VANADIS_VERB_PIPELINE_DETAIL CALL_INFO, 6, 0
// Add pretty-print separators
#define VANADIS_VERB_PRETTY CALL_INFO, 10, 0
// Level 7:
// Level 8:
// Level 9:
// Level 10:
// Level 11:
// Level 12:
// Level 13:
// Level 14:
// Level 15:
// Level 16:
//

#define VANADIS_DBG_INS_LDR_FLG  (1<<0)
#define VANADIS_DBG_DECODER_FLG  (1<<1)
#define VANADIS_DBG_ISSUE_FLG  (1<<2)
#define VANADIS_DBG_CYCLE_FLG  (1<<3)
#define VANADIS_DBG_RETIRE_FLG  (1<<4)
// Change 'checkpoint' to 'snapshot' to
// indicate this is not the same as SST's checkpoint
#define VANADIS_DBG_CHECKPOINT  (1<<5)
#define VANADIS_DBG_SNAPSHOT  (1<<5)

#endif
