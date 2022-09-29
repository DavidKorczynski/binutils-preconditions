/* objdump.c -- Describe symbol table of a rel file.
   Copyright (C) 1991-2021 Free Software Foundation, Inc.
   This file is part of GNU Binutils.
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
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */


int fuzz_slurp_symtab(bfd *abfd) {
  asymbol **sy = NULL;
  long storage;

  if (!(bfd_get_file_flags (abfd) & HAS_SYMS))
    {
      symcount = 0;
      return NULL;
    }

  //printf("A1\n");
  storage = bfd_get_symtab_upper_bound (abfd);
  if (storage < 0)
    {
      non_fatal (_("failed to read symbol table from: %s"), bfd_get_filename (abfd));
      bfd_fatal (_("error message was"));
    }

  //printf("A2\n");
  if (storage)
    {
      off_t filesize = bfd_get_file_size (abfd);

      /* qv PR 24707.  */
      if (filesize > 0
	  && filesize < storage
	  /* The MMO file format supports its own special compression
	     technique, so its sections can be larger than the file size.  */
	  && bfd_get_flavour (abfd) != bfd_target_mmo_flavour)
	{
	  bfd_nonfatal_message (bfd_get_filename (abfd), abfd, NULL,
				_("error: symbol table size (%#lx) "
				  "is larger than filesize (%#lx)"),
				storage, (long) filesize);
	  exit_status = 1;
	  symcount = 0;
	  return NULL;
	}

      sy = (asymbol **) xmalloc (storage);
    }
  //printf("A3\n");

  symcount = bfd_canonicalize_symtab (abfd, sy);
  if (symcount < 0) {
    if (sy) {
      free(sy);
    }
    return 0;
    //bfd_fatal (bfd_get_filename (abfd));
  }
  //printf("A4\n");
  if (syms) {
    free(sy);
  }
  return 1;
  //return sy;
}

int fuzz_dump_bfd (bfd *abfd, bool is_mainfile) {
  return fuzz_slurp_symtab(abfd);
}

int
fuzz_display_object_bfd (bfd *abfd)
{
  char **matching;

  if (bfd_check_format_matches (abfd, bfd_object, &matching))
    {
      return fuzz_dump_bfd (abfd, true);
    }

  if (bfd_get_error () == bfd_error_file_ambiguously_recognized)
    {
      nonfatal (bfd_get_filename (abfd));
      list_matching_formats (matching);
      return 1;
    }

  if (bfd_get_error () != bfd_error_file_not_recognized)
    {
      nonfatal (bfd_get_filename (abfd));
      return 1;
    }

  if (bfd_check_format_matches (abfd, bfd_core, &matching))
    {
      return fuzz_dump_bfd (abfd, true);
      //return 1;
    }

  nonfatal (bfd_get_filename (abfd));

  if (bfd_get_error () == bfd_error_file_ambiguously_recognized)
    list_matching_formats (matching);
  return 1;
}


int
fuzz_display_any_bfd (bfd *file, int level)
{
  int retval = 1;
  /* Decompress sections unless dumping the section contents.  */
  if (!dump_section_contents)
    file->flags |= BFD_DECOMPRESS;

  /* If the file is an archive, process all of its elements.  */
  if (bfd_check_format (file, bfd_archive))
    {
      bfd *arfile = NULL;
      bfd *last_arfile = NULL;

      if (level == 0)
        printf (_("In archive %s:\n"), sanitize_string (bfd_get_filename (file)));
      else if (level > 100)
	{
	  /* Prevent corrupted files from spinning us into an
	     infinite loop.  100 is an arbitrary heuristic.  */
	  //fatal (_("Archive nesting is too deep"));
	  return 0;
	}
  //    else
  //      printf (_("In nested archive %s:\n"),
	//	sanitize_string (bfd_get_filename (file)));

      for (;;)
	{
	  bfd_set_error (bfd_error_no_error);

	  arfile = bfd_openr_next_archived_file (file, arfile);
	  if (arfile == NULL)
	    {
	      if (bfd_get_error () != bfd_error_no_more_archived_files)
		nonfatal (bfd_get_filename (file));
	      break;
	    }

	  retval = fuzz_display_any_bfd (arfile, level + 1);

	  if (last_arfile != NULL)
	    {
	      bfd_close (last_arfile);
	      /* PR 17512: file: ac585d01.  */
	      if (arfile == last_arfile)
		{
		  last_arfile = NULL;
		  break;
		}
	    }
	  last_arfile = arfile;
	}

      if (last_arfile != NULL)
	bfd_close (last_arfile);
    }
  else
    retval = fuzz_display_object_bfd (file);

  return retval;
}


int
fuzz_display_file (char *filename, char *target, bool last_file)
{
  int retval = 1;
  bfd *file;

  if (get_file_size (filename) < 1)
    {
      return 0;
    }

  file = bfd_openr (filename, target);
  if (file == NULL)
    {
      return 0;
    }

  retval = fuzz_display_any_bfd (file, 0);

  /* This is an optimization to improve the speed of objdump, especially when
     dumping a file with lots of associated debug informatiom.  Calling
     bfd_close on such a file can take a non-trivial amount of time as there
     are lots of lists to walk and buffers to free.  This is only really
     necessary however if we are about to load another file and we need the
     memory back.  Otherwise, if we are about to exit, then we can save (a lot
     of) time by only doing a quick close, and allowing the OS to reclaim the
     memory for us.  */
  if (! last_file)
    bfd_close (file);
  else
    bfd_close_all_done (file);

  return retval;
}

int
fuzz_preconditions(char *filename) {
  int retval;

  retval = fuzz_display_file(filename, NULL, true);


  // Ensure globals are set
  //filename_per_symbol = 1;

  return retval;
}

