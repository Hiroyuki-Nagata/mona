/*************************************************************
 * Copyright (c) 2006 Shotaro Tsuji
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is     * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *************************************************************/

/* Please send bug reports to
    Shotaro Tsuji
    4-1010,
    Sakasedai 1-chome,
    Takaraduka-si,
    Hyogo-ken,
    665-0024
    Japan
    negi4d41@yahoo.co.jp
*/

#include "stdio_p.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <monapi/syscall.h>
#include <monapi/messages.h>

size_t __nida_read_console(void *ptr, size_t size, FILE *stream)
{
//    syscall_print("hoge");
    size = (size_t)readStream(stream->_stream, ptr, (uint32_t)size);
    return size;
}

size_t __nida_nonebuf_fread(void *buf, size_t size, FILE *stream)
{
    size_t readsize = 0;

    readsize = stream->_read(stream, buf, size);
    if( readsize == -1 )
    {
        stream->_flags |= __SERR;
        return 0;
    }
    if( readsize < size )
    {
        stream->_flags |= __SEOF;
    }

    stream->_extra->offset += readsize;
//  fseek(stream, stream->_extra->offset, SEEK_SET);
    return readsize;
}

size_t __nida_fullybuf_fread(void *buf, size_t size, FILE *stream)
{
  size_t readsize = 0;
  size_t retsize = 0;
  if( stream->_bf._range == 0 )
  {
    if(size > stream->_bf._size)
    {
      /* do not cache this case for simplicity. */
      return stream->_read(stream, buf,
                           size);
    }

    retsize = stream->_read(stream, stream->_bf._base,
                            stream->_bf._size);
    if( retsize == -1 )
    {
      stream->_flags |= __SERR;
      return 0;
    }
    if( retsize < size )
    {
      stream->_flags |= __SEOF;
    }
    readsize = retsize > size ? size : retsize;

    memcpy(buf, stream->_bf._base, readsize);
    stream->_bf._offset = stream->_extra->offset;
    stream->_bf._range = retsize;
  }
  else
  {
#if 0
    static int i = 0;
    if (i++ % 1000 == 0) {
    _logprintf("stream->_bf._offset = %d\n", stream->_bf._offset);
    _logprintf("stream->_extra->offset = %d\n", stream->_extra->offset);
    _logprintf("stream->_bf._size = %d\n", stream->_bf._size);
    _logprintf("stream->_bf._range = %d\n", stream->_bf._range);
    _logprintf("size = %d\n", size);
    }
#endif
    if( stream->_bf._offset == stream->_extra->offset )
    {
      if( size <= stream->_bf._range )
      {
        memcpy(buf, stream->_bf._base, size);
        //memcpy(buf, stream->_bf._base, stream->_bf._size);

        readsize = size;
      }
    }
    else if( stream->_bf._offset < stream->_extra->offset &&
             stream->_bf._offset+stream->_bf._range > stream->_extra->offset+size )
    {
      memcpy(buf,
             stream->_bf._base+(stream->_extra->offset-stream->_bf._offset),
             size);
      readsize = size;
    }
    else
    {
      stream->_seek(stream, stream->_extra->offset, SEEK_SET);
      readsize = stream->_read(stream,
                               stream->_bf._base,
                               stream->_bf._size);
      if( readsize == -1 )
      {
        stream->_flags |= __SERR;
        return 0;
      }
      if( readsize < size )
      {
        stream->_flags |= __SEOF;
        memcpy(buf, stream->_bf._base, readsize);
      }
      else
      {
        memcpy(buf, stream->_bf._base, size);
        stream->_bf._offset = stream->_extra->offset;
        stream->_bf._range = readsize;
        if( size > stream->_bf._size )
        {
          retsize = stream->_read(stream, (uint8_t*)buf+readsize, size-readsize);
          readsize += retsize;
        }
        else
        {
          readsize = size;
        }
      }
    }
  }

  stream->_extra->offset += readsize;

  return readsize;
}

size_t fread(void *buf, size_t size, size_t nmemb, FILE *stream)
{
/*
  if (stream->_extra == NULL)
    {
      syscall_print("panic");
    }
  else {
    //    syscall_print("not panic");
  }
*/
//_printf("filesize = %d, offset = %d\n", monapi_file_get_file_size(stream->_file), stream->_extra->offset);
//_printf("fread\n");
//_printf("buf = %x, size = %d, nmemb = %d\n", buf, size, nmemb);
//_printf("_read = %x\n", stream->_read);
/*
    if( monapi_file_get_file_size(stream->_file) <= stream->_extra->offset )
    {
        return 0;
    }
    */
    if( !(stream->_flags & __SRD) )
    {
        errno = EBADF;
        return 0;
    }
    if( stream->_extra->stds == __STDIN )
    {
        return __nida_read_console(buf, size*nmemb, stream);
    }
    /*
    if( stream->_extra->offset == monapi_file_get_file_size(stream->_file) )
    {
        return 0;
    }
    */
    else if( stream->_ungetcbuf != EOF )
    {
        {
            unsigned char *p = (unsigned char*)buf;
            p[0] = (unsigned char)stream->_ungetcbuf;
            stream->_ungetcbuf = EOF;
            return (size_t)1;
        }
    }
    else if( stream->_flags & __SNBF )
    {
        return __nida_nonebuf_fread(buf, size*nmemb, stream);
    }
    else if( stream->_flags & __SFBF )
    {
        return __nida_fullybuf_fread(buf, size*nmemb, stream);
    }
    else
    {
        return __nida_nonebuf_fread(buf, size*nmemb, stream);
    }
}
