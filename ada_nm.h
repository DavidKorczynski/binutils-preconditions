/* nm.c -- Describe symbol table of a rel file.
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
int
fuzz_display_archive (bfd *file)
{
  bfd *arfile = NULL;
  bfd *last_arfile = NULL;

  while (true) {
    arfile = bfd_openr_next_archived_file (file, arfile);
    if (arfile == NULL) {
      if (bfd_get_error () != bfd_error_no_more_archived_files) {
        return 0;
      }
      break;
    }
    if (last_arfile != NULL) {
      bfd_close (last_arfile);
      if (arfile == last_arfile)
        return 1;
    }
    last_arfile = arfile;
  }

  if (arfile) {
    bfd_close (last_arfile);
  }
  return 1;
}

int fuzz_display_file(char *filename) {
  bfd *file;
  file = bfd_openr (filename, NULL);
  if (file == NULL) {
    return 1;
  }

  if (line_numbers) {
    file->flags |= BFD_DECOMPRESS;
  }

  int retval = 1;
  if (bfd_check_format (file, bfd_archive)) {
      retval = fuzz_display_archive (file);
  }
  if (file) {
    bfd_close(file);
  }
  return retval;
}

int
fuzz2_display_rel_file (bfd *abfd, bfd *archive_bfd)
{
  long symcount;
  void *minisyms;
  unsigned int size;
  struct size_sym *symsizes;
  asymbol *synthsyms = NULL;

  if (! dynamic)
    {
      if (!(bfd_get_file_flags (abfd) & HAS_SYMS))
	{
	  if (!quiet)
	    non_fatal (_("%s: no symbols"), bfd_get_filename (abfd));
	  return 1;
	}
    }

  symcount = bfd_read_minisymbols (abfd, dynamic, &minisyms, &size);
  if (symcount < 0)
    {
      if (dynamic && bfd_get_error () == bfd_error_no_symbols)
	{
	  if (!quiet)
	    non_fatal (_("%s: no symbols"), bfd_get_filename (abfd));
	  return 1;
	}

      //bfd_fatal (bfd_get_filename (abfd));  
      return 0;
    }

  if (symcount == 0)
    {
      if (!quiet)
	non_fatal (_("%s: no symbols"), bfd_get_filename (abfd));
      return 1;
    }

  if (show_synthetic && size == sizeof (asymbol *))
    {
      asymbol **static_syms = NULL;
      asymbol **dyn_syms = NULL;
      long static_count = 0;
      long dyn_count = 0;
      long synth_count;

      if (dynamic)
	{
	  dyn_count = symcount;
	  dyn_syms = (asymbol **) minisyms;
	}
      else
	{
	  long storage = bfd_get_dynamic_symtab_upper_bound (abfd);

	  static_count = symcount;
	  static_syms = (asymbol **) minisyms;

	  if (storage > 0)
	    {
	      dyn_syms = (asymbol **) xmalloc (storage);
	      dyn_count = bfd_canonicalize_dynamic_symtab (abfd, dyn_syms);
	      if (dyn_count < 0) {
		//bfd_fatal (bfd_get_filename (abfd));
    return 0;
        }
	    }
	}

      synth_count = bfd_get_synthetic_symtab (abfd, static_count, static_syms,
					      dyn_count, dyn_syms, &synthsyms);
      if (synth_count > 0)
	{
	  asymbol **symp;
	  long i;

	  minisyms = xrealloc (minisyms,
			       (symcount + synth_count + 1) * sizeof (*symp));
	  symp = (asymbol **) minisyms + symcount;
	  for (i = 0; i < synth_count; i++)
	    *symp++ = synthsyms + i;
	  *symp = 0;
	  symcount += synth_count;
	}
      if (!dynamic && dyn_syms != NULL)
	free (dyn_syms);
    }

  /* lto_slim_object is set to false when a bfd is loaded with a compiler
     LTO plugin.  */
  if (abfd->lto_slim_object)
    {
      report_plugin_err = false;
      non_fatal (_("%s: plugin needed to handle lto object"),
		 bfd_get_filename (abfd));
    }

  /* Discard the symbols we don't want to print.
     It's OK to do this in place; we'll free the storage anyway
     (after printing).  */

  symcount = filter_symbols (abfd, dynamic, minisyms, symcount, size);

  symsizes = NULL;
  if (! no_sort)
    {
      sort_bfd = abfd;
      sort_dynamic = dynamic;
      sort_x = bfd_make_empty_symbol (abfd);
      sort_y = bfd_make_empty_symbol (abfd);
      if (sort_x == NULL || sort_y == NULL)
      {
	//bfd_fatal (bfd_get_filename (abfd));
  return 0;
      }

      if (! sort_by_size)
	qsort (minisyms, symcount, size,
	       sorters[sort_numerically][reverse_sort]);
      else
	symcount = sort_symbols_by_size (abfd, dynamic, minisyms, symcount,
					 size, &symsizes);
    }

 // if (! sort_by_size)
 //   print_symbols (abfd, dynamic, minisyms, symcount, size, archive_bfd);
 // else
 //   print_size_symbols (abfd, dynamic, symsizes, symcount, archive_bfd);

  if (synthsyms)
    free (synthsyms);
  free (minisyms);
  free (symsizes);
  return 1;
}


int
fuzz_display_file2 (char *filename)
{
  bool retval = true;
  bfd *file;
  char **matching;

  if (get_file_size (filename) < 1)
    return false;

  file = bfd_openr (filename, target ? target : plugin_target);
  if (file == NULL)
    {
      bfd_nonfatal (filename);
      return false;
    }

  /* If printing line numbers, decompress the debug sections.  */
  if (line_numbers)
    file->flags |= BFD_DECOMPRESS;

  if (bfd_check_format (file, bfd_archive))
    {
      display_archive (file);
    }
  else if (bfd_check_format_matches (file, bfd_object, &matching))
    {
      set_print_width (file);
      format->print_object_filename (filename);
      retval = fuzz2_display_rel_file (file, NULL);
    }
  else
    {
      bfd_nonfatal (filename);
      if (bfd_get_error () == bfd_error_file_ambiguously_recognized)
	list_matching_formats (matching);
      retval = false;
    }

  if (!bfd_close (file))
    bfd_fatal (filename);

  lineno_cache_bfd = NULL;
  lineno_cache_rel_bfd = NULL;

  return retval;
}


int
fuzz_preconditions(char *filename) {
  int retval;
  retval = fuzz_display_file(filename);
  if (retval == 0) {
    return 0;
  }

  // Ensure globals are set
  line_numbers = 1;
  no_sort = 0;
  sort_numerically = 1;
  sort_by_size = 0;
  filename_per_symbol = 1;

  retval = fuzz_display_file2(filename);

  line_numbers = 1;
  no_sort = 0;
  sort_numerically = 1;
  sort_by_size = 0;
  filename_per_symbol = 1;

  return retval;
}

