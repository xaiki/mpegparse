/*
 * flv.h
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

#ifndef __XA1_FLV_H__
#define __XA1_FLV_H__

#include <arpa/inet.h>
#include "mpegparse.h"

#ifndef NULL
#define NULL 0
#endif

#define xa1_debug_flv_type(data)	(data)[4]
#define xa1_debug_flv_prev(data)	ntohl(*((uint32_t*)(data)))
#define xa1_debug_flv_datasize(data)    ntohs(*((uint16_t*)((data) + 6)))
#define xa1_debug_flv_timestamp(data)   ntohs(*((uint16_t*)((data) + 9)))

enum {
	FLV_H_SIGNATURE = 0,
	FLV_H_VERSION,
	FLV_H_TYPEFLAGSRESERVED0,
	FLV_H_TYPEFLAGSAUDIO,
	FLV_H_TYPEFLAGSRESERVED1,
	FLV_H_TYPEFLAGSVIDEO,
	FLV_H_DATAOFFSET,
};

struct parseme flv_header[] =
	{
		{"FLV Header"},
		{"Signature", 24, UINT24STR('F', 'L', 'V')},
		{"Version", 8, 0x01},
		{"TypeFlagsReserved0", 5, 0, _check_zero},
		{"TypeFlagsAudio", 1},
		{"TypeFlagsReserved1", 1, 0, _check_zero},
		{"TypeFlagsVideo", 1},
		{"DataOffset", 32},
		{NULL}
	};

#define FLV_TAG_SIZE 15

enum {
	FLV_T_PREVIOUSTAGSIZE = 0,
	FLV_T_TYPE,
	FLV_T_DATASIZE,
	FLV_T_TIMESTAMP,
	FLV_T_TIMESTAMPEXTENDED,
	FLV_T_STREAMID,
	FLV_T_EMBEDDED,
	FLV_T_DATA,
};

struct parseme flv_minfo[] =
	{
		{"FLV Metadata Info"},
		{"Not Implemented", 0, 0, 0},
		{NULL}
	};

struct parseme flv_ainfo[] =
	{
		{"FLV Audio Info"},
		{"ACodec", 4, PM_RANGE(0,15)},
		{"SamplingRate", 2, PM_RANGE(0,3)},
		{"Is16bits", 1, 0},
		{"IsStereo", 1, 0},
		{NULL}
	};

enum {
    FLV_T_V_FT_KEYFRAME = 1,
    FLV_T_V_FT_INTERFRAME = 2,
    FLV_T_V_FT_DISP_INTERFRAME = 3,
    FLV_T_V_FT_GEN_KEYFRAME = 4,
    FLV_T_V_FT_VINFO_RESERVED = 5,
};

struct parseme flv_vinfo[] =
	{
		{"FLV Video Info"},
		{"FrameType", 4, PM_RANGE(1,5)},
		{"VCodec", 4, PM_RANGE(1,7)},
		{NULL}
	};

enum {
	FLV_T_TYPE_AUDIO = 8,
	FLV_T_TYPE_VIDEO = 9,
	FLV_T_TYPE_METADATA = 18,
};

int check_type (struct parseme p[], int field, char *buf, size_t buflen) {
	char v = *buf;
	switch (v) {
	case FLV_T_TYPE_AUDIO:
		dprintf("type is audio\n");
		_PM_EMBEDDED_EMBED (p[FLV_T_EMBEDDED], flv_ainfo);
		break;
	case FLV_T_TYPE_VIDEO:
		dprintf("type is video\n");
		_PM_EMBEDDED_EMBED (p[FLV_T_EMBEDDED], flv_vinfo);
		break;
	case FLV_T_TYPE_METADATA:
		dprintf("type is metadata\n");
		//		p[FLV_T_EMBEDDED].check = flv_minfo;
		break;
	default:
		dprintf("wrong type: %d\n", v);
		PM_CHECK_FAIL;
		return 0;
	}
	return 1;
}

int update_data_size (struct parseme p[], int field, char *buf, size_t buflen) {
	dprintf("setting %s to %u from %s.\n", p[FLV_T_DATA].name, p[field].data, p[field].name);

	p[FLV_T_DATA].size = p[field].data*8;
	return 1;
}

struct parseme flv_tag[] =
	{
		{"FLV Tag"},
		{"PreviousTagSize", 32, 0},
		{"Type", 8, 0, check_type},
		{"DataSize", 24, 0, update_data_size},
		{"Timestamp", 24, 0},
		{"TimestampExtended", 8, 0},
		{"StreamID", 24, 0, _check_zero},
		{_PM_EMBEDDED},
		{"Data", 0},
		{NULL}
	};

#endif /* __XA1_FLV_H__ */
