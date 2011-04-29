/*
 * ogg.h
 * This file is part of mpegparse.
 *
 * Copyright (C) 2011 - Niv Sardi <xaiki@debian.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation.
 * version 2.0 of the License (strictly).
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __XA1_OGG_H__
#define __XA1_OGG_H__
struct parseme ogg_page[] =
	{
		{"Capture Patern", 32, UINT32STR('O', 'g', 'g', 'S')},
		{"Version", 8, 0, _check_zero},
		{"Header Type", 8},
		{"Granule Position", 64},
		{"Bitstream Serial Number", 32},
		{"Page Sequence Number", 32},
		{"Checksum", 32},
		{"Page Segment", 8},
		{"Segment Table", 8},
		{NULL}
	};

#endif /* __XA1_OGG_H__ */
