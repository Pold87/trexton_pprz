/*
 * Copyright (C) Volker Strobel
 *
 * This file is part of paparazzi
 *
 * paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
/**
 * @file "modules/trexton/trexton.regression.h"
 * @author Volker Strobel
 * treXton regression localization
 */

#ifndef TREXTON_REG_H
#define TREXTON_REG_H

#include <stdio.h>
#include "texton_settings.h"
#include "lib/vision/image.h"

static int histograms[NUM_HISTOGRAMS][NUM_TEXTONS];

//static int targets[1000]; /* targets for classifier (machine learning)*/
static char training_data_path[] = "training_data/";

uint8_t predict_class(int *texton_hist);

extern void trexton_init(void);
extern void trexton_periodic(void);


#endif
