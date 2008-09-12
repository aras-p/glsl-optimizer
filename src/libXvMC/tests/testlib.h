#ifndef testlib_h
#define testlib_h

/*
#define TEST(pred, doc)	test(pred, #pred, doc, __FILE__, __LINE__)

void test(int pred, const char *pred_string, const char *doc_string, const char *file, unsigned int line);
*/

#include <X11/Xlib.h>
#include <X11/extensions/XvMClib.h>

/*
 * display: IN			A valid X display
 * width, height: IN		Surface size that the port must display
 * chroma_format: IN		Chroma format that the port must display
 * mc_types, num_mc_types: IN	List of MC types that the port must support, first port that matches the first mc_type will be returned
 * port_id: OUT			Your port's ID
 * surface_type_id: OUT		Your port's surface ID
 * is_overlay: OUT		If 1, port uses overlay surfaces, you need to set a colorkey
 * intra_unsigned: OUT		If 1, port uses unsigned values for intra-coded blocks
 */
int GetPort
(
	Display *display,
	unsigned int width,
	unsigned int height,
	unsigned int chroma_format,
	const unsigned int *mc_types,
	unsigned int num_mc_types,
	XvPortID *port_id,
	int *surface_type_id,
	unsigned int *is_overlay,
	unsigned int *intra_unsigned
);

#endif

