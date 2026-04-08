



/* This file is part of GLD, the Gnu Linker.
   Copyright (C) 1999-2024 Free Software Foundation, Inc.

   This file is part of the GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

/* For TI COFF */
/* Need to determine load and run pages for output sections */

#define TARGET_IS_tic3xcoff_onchip

#include "sysdep.h"
#include "libiberty.h"
#include "bfd.h"
#include "bfdlink.h"
#include "ctf-api.h"
#include "getopt.h"

#include "ld.h"
#include "ldmain.h"
#include "ldmisc.h"
#include "ldexp.h"
#include "ldlang.h"
#include "ldfile.h"
#include "ldemul.h"

static int coff_version;

/* TI COFF extra command line options */
#define OPTION_COFF_FORMAT		(300 + 1)

static void
gldtic3xcoff_onchip_add_options
  (int ns ATTRIBUTE_UNUSED, char **shortopts ATTRIBUTE_UNUSED, int nl,
   struct option **longopts, int nrl ATTRIBUTE_UNUSED,
   struct option **really_longopts ATTRIBUTE_UNUSED)
{
  static const struct option xtra_long[] = {
    /* TI COFF options */
    {"format", required_argument, NULL, OPTION_COFF_FORMAT },
    {NULL, no_argument, NULL, 0}
  };

  *longopts = (struct option *)
    xrealloc (*longopts, nl * sizeof (struct option) + sizeof (xtra_long));
  memcpy (*longopts + nl, &xtra_long, sizeof (xtra_long));
}

static void
gldtic3xcoff_onchip_list_options (FILE * file)
{
  fprintf (file, _("  --format 0|1|2              Specify which COFF version to use\n"));
}

static bool
gldtic3xcoff_onchip_handle_option (int optc)
{
  switch (optc)
    {
    default:
      return false;

    case OPTION_COFF_FORMAT:
      if ((*optarg == '0' || *optarg == '1' || *optarg == '2')
	  && optarg[1] == '\0')
	{
	  static char buf[] = "coffX-tic4x";
	  coff_version = *optarg - '0';
	  buf[4] = *optarg;
	  lang_add_output_format (buf, NULL, NULL, 0);
	}
      else
	{
	  einfo (_("%F%P: invalid COFF format version %s\n"), optarg);
	}
      break;
    }
  return false;
}

static void
gldtic3xcoff_onchip_before_parse(void)
{
#ifndef TARGET_			/* I.e., if not generic.  */
  ldfile_set_output_arch ("tic3x", bfd_arch_unknown);
#endif /* not TARGET_ */
}

static char *
gldtic3xcoff_onchip_get_script (int *isfile)
{
  *isfile = 1;

  if (bfd_link_relocatable (&link_info) && config.build_constructors)
    return "ldscripts/tic3xcoff_onchip.xu";
  else if (bfd_link_relocatable (&link_info))
    return "ldscripts/tic3xcoff_onchip.xr";
  else if (!config.text_read_only)
    return "ldscripts/tic3xcoff_onchip.xbn";
  else if (!config.magic_demand_paged)
    return "ldscripts/tic3xcoff_onchip.xn";
  else
    return "ldscripts/tic3xcoff_onchip.x";
}

struct ld_emulation_xfer_struct ld_tic3xcoff_onchip_emulation =
{
  gldtic3xcoff_onchip_before_parse,
  syslib_default,
  hll_default,
  after_parse_default,
  NULL,
  after_open_default,
  after_check_relocs_default,
  before_place_orphans_default,
  after_allocation_default,
  set_output_arch_default,
  ldemul_default_target,
  before_allocation_default,
  gldtic3xcoff_onchip_get_script,
  "tic3xcoff_onchip",
  "coff2-tic4x",
  finish_default,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  gldtic3xcoff_onchip_add_options,
  gldtic3xcoff_onchip_handle_option,
  NULL,
  gldtic3xcoff_onchip_list_options,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
