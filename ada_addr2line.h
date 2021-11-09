/*
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
  long storage;
  long symcount;
  bool dynamic = false;

  if ((bfd_get_file_flags (abfd) & HAS_SYMS) == 0) {
    return 1;
  }  
  storage = bfd_get_symtab_upper_bound (abfd);
  if (storage == 0) {
    storage = bfd_get_dynamic_symtab_upper_bound (abfd);
    dynamic = true;
  }
  if (storage < 0) {
    return 0;
  }

  syms = (asymbol **) xmalloc (storage);
  if (dynamic) {
    symcount = bfd_canonicalize_dynamic_symtab (abfd, syms);
  }
  else {
    symcount = bfd_canonicalize_symtab (abfd, syms);
  }

  if (symcount < 0) {
    free(syms);
    syms = NULL;
    return 0;
  }
  free(syms);
  syms = NULL;
  return 1;
}

int fuzz_preconditions_check(const char *file_name, const char *target) {
  bfd *abfd;
  char **matching;

  if (get_file_size (file_name) < 1) {
    return 0;
  }

  abfd = bfd_openr (file_name, target);
  if (abfd == NULL) {
    /* In this specific case just exit the fuzzer */
    bfd_fatal (file_name);
  }

  abfd->flags |= BFD_DECOMPRESS;
  if (bfd_check_format (abfd, bfd_archive)) {
    bfd_close(abfd);
    return 0;
  }

  if (! bfd_check_format_matches (abfd, bfd_object, &matching)) {
    bfd_close(abfd);
    return 0;
  }

  /* Check if slurp_symtab will exit */
  int retval = fuzz_slurp_symtab(abfd);
  bfd_close(abfd);
  return retval;
}

