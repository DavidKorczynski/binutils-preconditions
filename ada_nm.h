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
fuzz_preconditions(char *filename) {
  return fuzz_display_file(filename);
}

