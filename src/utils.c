/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * utils.c  This file is part of Woofer GTK
 * Copyright (C) 2021, 2022  Quico Augustijn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed "as is" in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  If your
 * computer no longer boots, divides by 0 or explodes, you are the only
 * one responsible.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <https://www.gnu.org/licenses/gpl-3.0.html>.
 */

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>
#include <math.h>

// Woofer core includes
/*< none >*/

// Module includes
#include "utils.h"

// Dependency includes
/*< none >*/

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This provides some general purpose utility used by the rest of the software.
 *
 * Since this module only contains utilities for other modules, all of
 * these "utilities" are part of the normal module functions and
 * constructors, destructors, etc are left out.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */
/* DEFINES END */

/* CUSTOM TYPES BEGIN */
/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */
/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */
/* GLOBAL VARIABLES END */

/* MODULE FUNCTIONS BEGIN */

// Function to round a double and convert it into a char array to be used in a string or printf
gchar *
interface_utils_round_double_two_decimals_to_str(gdouble value)
{
	gint whole_numbers, decimal_numbers, result = 0;
	gint multiply = 100; // two decimals, multiply and divide by 10^2
	gchar *str;

	// Multiply (e.g. value was == 2.346, value now == 234.6)
	value *= multiply;

	// Round properly instead of always down
	value += 0.5;

	// Only keep the value we want (e.g. value == 235.1, result == 235)
	result = floor(value);

	// Split whole numbers and decimal numbers
	whole_numbers = result / multiply; // e.g. 2 = 235 / 100 (int automatically rounds down)
	decimal_numbers = result - (whole_numbers * multiply); // e.g. 35 = 235 - (2 * 100)

	// Dump values into string
	str = g_strdup_printf("%d.%d", whole_numbers, decimal_numbers); // e.g. 2.35

	return str;
}

/* MODULE FUNCTIONS END */

/* END OF FILE */
