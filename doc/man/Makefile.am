##
## Makefile.am -- Process this file with automake to produce
##                Makefile.in.
##
#
# Copyright (C) 2013 Bruno Félix Rezende Ribeiro
# <oitofelix@riseup.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

dist_man_MANS = ccd2cue.1
MAINTAINERCLEANFILES = ccd2cue.1

ccd2cue.1: $(top_srcdir)/src/ccd2cue.c
	$(MAKE) --directory=$(top_srcdir)/src $(AM_MAKEFLAGS) ccd2cue
	$(HELP2MAN) --output=$@  $(top_srcdir)/src/ccd2cue