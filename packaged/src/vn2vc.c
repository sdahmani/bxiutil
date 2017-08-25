/* -*- coding: utf-8 -*-
###############################################################################
# Author:  Safae Dahmani <safae.dahmani@bull.net>
# Created on: Aug 21, 2017
# Contributors:
###############################################################################
# Copyright (C) 2017  Bull S. A. S.  -  All rights reserved
# Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
# This is not Free or Open Source software.
# Please contact Bull S. A. S. for details about its license.
###############################################################################
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "bxi/base/log.h"
#include "bxi/hwinterface/macro.h"
#include "bxi/base/mem.h"
#include "bxi/util/vn2vc.h"


// *********************************************************************************
// ********************************** Global Variables *****************************
// *********************************************************************************

SET_LOGGER(BXIVN2VC_LOGGER, BXILOG_LIB_PREFIX "bxiutil.vn2vc");

// *********************************************************************************
// ********************************** Implementation   *****************************
// *********************************************************************************

/*
 * Initialize the vn2vc tables with a RoundRobin algorithm
 */
 
int** init_vn2vc(int snid, int nid_nb, int* vc_nb) {
    int** buffer_out = bximem_calloc(BXIHWIF_VN_NB * sizeof(int*));
    for (int i = 0; i < BXIHWIF_VN_NB; i++) {
        buffer_out[i] = bximem_calloc((long unsigned)(nid_nb) * sizeof(int));
    }
    for (int vn = 0; vn < BXIHWIF_VN_NB; ++vn) {
        BXIASSERT(BXIVN2VC_LOGGER, vc_nb[vn] <= 8);
    	int vc_init = 0;
    	for (int dnid = 0; dnid < nid_nb; ++dnid) { 
            buffer_out[vn][dnid] = vc_init + snid % vc_nb[vn];
	    }
    	vc_init += vc_nb[vn];
	}
	return buffer_out;
}


/*
 * Compute vn2vc tables with depending on Src/Dest nodes
 */
int** compute_vn2vc(int snid, int nid_nb, int* vc_nb) {
    int** buffer_out = bximem_calloc(BXIHWIF_VN_NB * sizeof(int*));
    for (int i = 0; i < BXIHWIF_VN_NB; i++) {
        buffer_out[i] = bximem_calloc((long unsigned)(nid_nb) * sizeof(int));
    }
    for (int vn = 0; vn < BXIHWIF_VN_NB; ++vn) {
        BXIASSERT(BXIVN2VC_LOGGER, vc_nb[vn] <= 8);
    	int vc_init = 0;
    	for (int dnid = 0; dnid < nid_nb; ++dnid) { 
            buffer_out[vn][dnid] = vc_init + (snid + dnid + dnid/8) % vc_nb[vn];
	    }
    	vc_init += vc_nb[vn];
	}
	return buffer_out;
}
