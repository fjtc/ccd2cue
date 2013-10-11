/**
 * \file       cdt.h
 * \brief      _CD-Text_ format structure
 * \author     Bruno Félix Rezende Ribeiro (_oitofelix_)
 *             <oitofelix@gmail.com>
 * \date       2013
 * \version    0.2
 *
 * \copyright [GNU General Public License (version 3 or later)]
 *            (http://www.gnu.org/licenses/gpl.html)
 *
 * ~~~
 * This file is part of ccd2cue.
 *
 * ccd2cue is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ccd2cue is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ~~~
 *
 */


#ifndef CCD2CUE_CDT_H
#define CCD2CUE_CDT_H

#include <stddef.h>
#include <stdint.h>

/**
 * _CD-Text_ entry type
 *
 * Each CD-Text entry has a type according to the following
 * enumeration.  The field that holds that type is cdt_data.type,
 * however it is of type uint8_t rather enum cdt_entry_type for
 * portability reasons.  So we can say that effectively it has type
 * ((uint8_t) enum cdt_entry_type).
 *
 * This enumeration is here just for reference and completeness,
 * because it is not used in any way in the conversion routines.  This
 * info is already available in the input _CCD sheet_ and is copied
 * verbatim to the _CD-Text_ output file.
 *
 */

enum cdt_entry_type
  {
    CDT_TITLE = 0x80,       /**< Album name and Track titles. */
    CDT_PERFORMER = 0x81,   /**< Singer/player/conductor/orchestra. */
    CDT_SONGWRITER = 0x82,  /**< Name of the songwriter. */
    CDT_COMPOSER = 0x83,    /**< Name of the composer. */
    CDT_ARRANGER = 0x84,    /**< Name of the arranger. */
    CDT_MESSAGE = 0x85,	    /**< Message from content provider or artist. */
    CDT_DISK_ID = 0x86,	    /**< Disk identification information. */
    CDT_GENRE = 0x87,	    /**< Genre identification / information. */
    CDT_TOC = 0x88,	    /**< TOC information. */
    CDT_TOC2 = 0x89,	    /**< Second TOC. */
    CDT_RES_8A = 0x8A,	    /**< Reserved 8A. */
    CDT_RES_8B = 0x8B,	    /**< Reserved 8B. */
    CDT_RES_8C = 0x8C,	    /**< Reserved 8C. */
    CDT_CLOSED_INFO = 0x8D, /**< For internal use by content provider. */
    CDT_ISRC = 0x8E,	    /**< UPC/EAN code of album and ISRC for tracks. */
    CDT_SIZE = 0x8F	    /**< Size information of the block. */
  };

/**
 * _CD-Text_ data
 *
 * This is the actual _CD-Text_ data structure.  It has 4 header bytes
 * and then 12 bytes of real data.  This structure together with its
 * _CRC_ compose the called _CD-Text_ Entry (cdt_entry).
 *
 * This structure describes an individual and complete "Entry" entry
 * inside a "CDText" section found in _CCD sheets_.  That entry is a
 * sequence of 16 bytes represented as ascii in hexadecimal notation.
 *
 */

struct cdt_data
{
  uint8_t type;	        /**< Entry type ((uint8_t) enum cdt_entry_type). */
  uint8_t track;	/**< Track number (0..99). */
  uint8_t sequence;	/**< Sequence number. */
  uint8_t block;        /**< Block number / Char position. */
  uint8_t text[12];	/**< _CD-Text_ data. */
};

/**
 * _CD-Text_ entry
 *
 * This structure describes an individual and complete entry as it is
 * put inside a _CDT file_.  It is just the _CD-Text_ data, possibly found
 * inside a _CCD sheet_, plus its CRC calculated by ::crc16.
 *
 */

struct cdt_entry
{
  struct cdt_data data;  /**< _CD-Text_ data. */
  uint8_t crc[2];        /**< CRC 16. */
};

/**
 * _CD-Text_ format structure
 *
 * This structure is just the whole collection of _CD-Text_ entries as
 * defined in ::cdt_entry.  It is usually obtained from _CCD sheet_ by
 * means of its correspondent structure and ::ccd2cdt function.  It
 * represents all _CD-Text_ data possibly present in a _CCD sheet_,
 * with its CRC 16 checksum already calculated, and ready to be put in
 * a _CDT file_ by ::cdt2stream.
 *
 */

struct cdt
{
  struct cdt_entry *entry; 	/**< Array of _CDT_ entries.  */
  size_t entries;		/**< Number of _CDT_ entries. */
};

/**
 * Convert _CDT structure_ into a _CDT file_ stream.
 *
 * \param[in]   cdt     Pointer to the _CDT structure_;
 * \param[out]  stream  Stream;
 *
 * \note This function exits if any writing error occurs.
 *
 * \since 0.2
 *
 * This function converts a _CDT structure_ containing all _CD-Text_
 * data and its checksum into a binary _CDT file_ stream.  It is
 * normally used to generate the file with extension ".cdt" that
 * accompany the _CUE sheet_ that reference it with a "CDTEXTFILE"
 * entry.  The _CDT structure_ that this function receives is usually
 * generated by ::ccd2cdt.
 *
 * The resulting stream has a terminating NULL character.  So, the
 * _CDT file_ size has 18 bytes per CDT entry --- 4 for header, 12 for
 * data and 2 for CRC --- and an additional NULL character.  Thus, if
 * ENTRIES is the number of _CDT_ entries, the following formula gives
 * the _CDT file_ size:
 *
 * + size = (4+12+2) * ENTRIES + 1
 *
 * This is the last step in the _CD-Text_ data conversion process.
 *
 * \sa
 * - Previous step:
 *   + ::ccd2cue
 *   + ::ccd2cdt
 * - Parallel step:
 *   + ::cue2stream
 *
 */

void cdt2stream (const struct cdt *cdt, FILE *stream)
  __attribute__ ((nonnull));

#endif	/* CCD2CUE_CDT_H */
