/*
 *   jack-rack
 *    
 *   Copyright (C) Robert Ham 2002 (node@users.sourceforge.net)
 *    
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __JR_OBJECT_FILE_H__
#define __JR_OBJECT_FILE_H__

typedef struct _object_file object_file_t;

struct _object_file
{
  char *         filename;
  void *         dl_handle;
  unsigned long  instance_count;
};

object_file_t * object_file_new     (const char * filename);
void            object_file_destroy (object_file_t * object_file);

const char *  object_file_get_filename       (const object_file_t * object_file);
unsigned long object_file_get_instance_count (const object_file_t * object_file);

plugin_t * object_file_create_instance (object_file_t * object_file, unsigned long index);



#endif /* __JR_OBJECT_FILE_H__ */
