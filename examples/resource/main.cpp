#include <stdio.h>
#include "jpeglib.h" //  sudo apt install libjpeg-dev
#include <setjmp.h> 
#include <string.h> 

#include <thread>
#include <unistd.h>

#include "framebuffer.h"
#include "resource.h"

/******************** JPEG DECOMPRESSION SAMPLE INTERFACE *******************/
 
 /* This half of the example shows how to read data from the JPEG decompressor.
  * It's a bit more refined than the above, in that we show:
  *   (a) how to modify the JPEG library's standard error-reporting behavior;
  *   (b) how to allocate workspace using the library's memory manager.
  *
  * Just to make this example a little different from the first one, we'll
  * assume that we do not intend to put the whole image into an in-memory
  * buffer, but to send it line-by-line someplace else.  We need a one-
  * scanline-high JSAMPLE array as a work buffer, and we will let the JPEG
  * memory manager allocate it for us.  This approach is actually quite useful
  * because we don't need to remember to deallocate the buffer separately: it
  * will go away automatically when the JPEG object is cleaned up.
  */
 
 
 /*
  * ERROR HANDLING:
  *
  * The JPEG library's standard error handler (jerror.c) is divided into
  * several "methods" which you can override individually.  This lets you
  * adjust the behavior without duplicating a lot of code, which you might
  * have to update with each future release.
  *
  * Our example here shows how to override the "error_exit" method so that
  * control is returned to the library's caller when a fatal error occurs,
  * rather than calling exit() as the standard error_exit method does.
  *
  * We use C's setjmp/longjmp facility to return control.  This means that the
  * routine which calls the JPEG library must first execute a setjmp() call to
  * establish the return point.  We want the replacement error_exit to do a
  * longjmp().  But we need to make the setjmp buffer accessible to the
  * error_exit routine.  To do this, we make a private extension of the
  * standard JPEG error handler object.  (If we were using C++, we'd say we
  * were making a subclass of the regular error handler.)
  *
  * Here's the extended error handler struct:
  */

/*
 * SOME FINE POINTS:
 *
 * In the above code, we ignored the return value of jpeg_read_scanlines,
 * which is the number of scanlines actually read.  We could get away with
 * this because we asked for only one line at a time and we weren't using
 * a suspending data source.  See libjpeg.txt for more info.
 *
 * We cheated a bit by calling alloc_sarray() after jpeg_start_decompress();
 * we should have done it beforehand to ensure that the space would be
 * counted against the JPEG max_memory setting.  In some systems the above
 * code would risk an out-of-memory error.  However, in general we don't
 * know the output image dimensions before jpeg_start_decompress(), unless we
 * call jpeg_calc_output_dimensions().  See libjpeg.txt for more about this.
 *
 * Scanlines are returned in the same order as they appear in the JPEG file,
 * which is standardly top-to-bottom.  If you must emit data bottom-to-top,
 * you can use one of the virtual arrays provided by the JPEG memory manager
 * to invert the data.  See wrbmp.c for an example.
 *
 * As with compression, some operating modes may require temporary files.
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.txt.
 */

struct my_error_mgr
{
	struct jpeg_error_mgr pub;    /* "public" fields */
	jmp_buf setjmp_buffer;        /* for return to caller */
};
 
typedef struct my_error_mgr *my_error_ptr;
 
/*
* Here's the routine that will replace the standard error_exit method:
*/

METHODDEF(void) my_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}


/*
* Sample routine for JPEG decompression.  We assume that the source file name
* is passed in.  We want to return 1 on success, 0 on error.
*/
GLOBAL(int) read_JPEG_file (uint8_t *MyBuffer,int MyBufferSize,FBIO::FrameBuffer* FB)
{	
	/* This struct contains the JPEG decompression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	*/
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	struct my_error_mgr jerr;
	/* More stuff */
	JSAMPARRAY buffer;            /* Output row buffer */
	int row_stride;               /* physical row width in output buffer */

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer))
	{
		/* If we get here, the JPEG code has signaled an error.
		* We need to clean up the JPEG object, close the input file, and return.
		*/
		jpeg_destroy_decompress(&cinfo);
		return 0;
	}
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

//	jpeg_stdio_src(&cinfo, infile);
	jpeg_mem_src(&cinfo,MyBuffer,MyBufferSize);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void) jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	*   (a) suspension is not possible with the stdio data source, and
	*   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	* See libjpeg.txt for more info.
	*/

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	* jpeg_read_header(), so we do nothing here.
	*/

	/* Step 5: Start decompressor */

	(void) jpeg_start_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/
	printf("File: %dx%d Channels %d\n",cinfo.output_width,cinfo.output_height,cinfo.num_components);
	
	/* We may need to do some setup of our own at this point before reading
	* the data.  After jpeg_start_decompress() we have the correct scaled
	* output image dimensions available, as well as the output colormap
	* if we asked for color quantization.
	* In this example, we need to make an output work buffer of the right size.
	*/
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
	((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	*/
	uint8_t* image = new uint8_t[cinfo.output_height * cinfo.output_width * 3];

	uint8_t* dst = image;
	for( JDIMENSION y = 0 ; y < cinfo.output_height ; y++, dst += (cinfo.output_width * 3) )
	{
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);
		memcpy(dst,buffer[0],cinfo.output_width * 3);
	}

	FB->BlitRGB24(image,(FB->GetWidth()/2) - (cinfo.output_width/2),(FB->GetHeight()/2) - (cinfo.output_height/2),cinfo.output_width,cinfo.output_height);
	
	delete []image;	
	
	/* Step 7: Finish decompression */

	(void) jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* At this point you may want to check to see whether any corrupt-data
	* warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	*/

	/* And we're done! */
	return 1;
}

int main(int argc, char *argv[])
{
	FBIO::FrameBuffer* FB = FBIO::FrameBuffer::Open(true);
	if( FB )
	{
		int buffersize;
		uint8_t* buffer = AppBuildResource::CopyResourceToMemory("./data/rose.jpg",buffersize);
		if( buffer && buffersize > 0 )
		{
			FB->ClearScreen(255,255,255);
			read_JPEG_file(buffer,buffersize,FB);
			sleep(3);
		}
		else
		{
			printf("Failed to get resource \"./data/rose.jpg\"\n");
		}
		delete []buffer;

		buffer = AppBuildResource::CopyResourceToMemory("./data/moon.jpg",buffersize);
		if( buffer && buffersize > 0 )
		{
			FB->ClearScreen(0,0,0);
			read_JPEG_file(buffer,buffersize,FB);
			sleep(3);
		}
		else
		{
			printf("Failed to get resource \"./data/moon.jpg\"\n");
		}
		delete []buffer;

		FB->ClearScreen(0,0,0);
		delete FB;
		FB = NULL;
	}

	return 0;
}
