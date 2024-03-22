/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2019 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 2.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 2.0 of the Apache
 * License as published by the Apache Software Foundation. See the LICENSE files
 * for more details.
 *
 * RELIC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the LICENSE files for more details.
 *
 * You should have received a copy of the GNU Lesser General Public or the
 * Apache License along with RELIC. If not, see <https://www.gnu.org/licenses/>
 * or <https://www.apache.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of the low-level extension field addition functions.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fp_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp3_addn_low(fp3_t c, fp3_t a, fp3_t b) {
	fp_addn_low(c[0], a[0], b[0]);
	fp_addn_low(c[1], a[1], b[1]);
	fp_addn_low(c[2], a[2], b[2]);
}

void fp3_addm_low(fp3_t c, fp3_t a, fp3_t b) {
	fp_addm_low(c[0], a[0], b[0]);
	fp_addm_low(c[1], a[1], b[1]);
	fp_addm_low(c[2], a[2], b[2]);
}

void fp3_subn_low(fp3_t c, fp3_t a, fp3_t b) {
	fp_subn_low(c[0], a[0], b[0]);
	fp_subn_low(c[1], a[1], b[1]);
	fp_subn_low(c[2], a[2], b[2]);
}

void fp3_subm_low(fp3_t c, fp3_t a, fp3_t b) {
	fp_subm_low(c[0], a[0], b[0]);
	fp_subm_low(c[1], a[1], b[1]);
	fp_subm_low(c[2], a[2], b[2]);
}

void fp3_dbln_low(fp3_t c, fp3_t a) {
	fp_dbln_low(c[0], a[0]);
	fp_dbln_low(c[1], a[1]);
	fp_dbln_low(c[2], a[2]);
}

void fp3_subc_low(dv3_t c, dv3_t a, dv3_t b) {
	fp_subc_low(c[0], a[0], b[0]);
	fp_subc_low(c[1], a[1], b[1]);
	fp_subc_low(c[2], a[2], b[2]);
}

void fp3_dblm_low(fp3_t c, fp3_t a) {
	fp_dblm_low(c[0], a[0]);
	fp_dblm_low(c[1], a[1]);
	fp_dblm_low(c[2], a[2]);
}
