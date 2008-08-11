# src/glu/sgi/Makefile

.SUFFIXES : .cc

TOP = ../../..

include $(TOP)/configs/current

GLU_MAJOR = 1
GLU_MINOR = 3
GLU_TINY = 0$(MESA_MAJOR)0$(MESA_MINOR)0$(MESA_TINY)

INCDIRS = -I$(TOP)/include -Iinclude -Iinternals -Ilibnurbs/internals -Ilibnurbs/interface -Ilibnurbs/nurbtess

C_SOURCES = \
	libutil/error.c		\
	libutil/glue.c		\
	libutil/mipmap.c	\
	libutil/project.c	\
	libutil/quad.c		\
	libutil/registry.c	\
	libtess/dict.c		\
	libtess/geom.c		\
	libtess/memalloc.c	\
	libtess/mesh.c		\
	libtess/normal.c	\
	libtess/priorityq.c	\
	libtess/render.c	\
	libtess/sweep.c		\
	libtess/tess.c		\
	libtess/tessmono.c

CC_SOURCES = \
	libnurbs/interface/bezierEval.cc		\
	libnurbs/interface/bezierPatch.cc		\
	libnurbs/interface/bezierPatchMesh.cc		\
	libnurbs/interface/glcurveval.cc		\
	libnurbs/interface/glinterface.cc		\
	libnurbs/interface/glrenderer.cc		\
	libnurbs/interface/glsurfeval.cc		\
	libnurbs/interface/incurveeval.cc		\
	libnurbs/interface/insurfeval.cc		\
	libnurbs/internals/arc.cc			\
	libnurbs/internals/arcsorter.cc			\
	libnurbs/internals/arctess.cc			\
	libnurbs/internals/backend.cc			\
	libnurbs/internals/basiccrveval.cc		\
	libnurbs/internals/basicsurfeval.cc		\
	libnurbs/internals/bin.cc			\
	libnurbs/internals/bufpool.cc			\
	libnurbs/internals/cachingeval.cc		\
	libnurbs/internals/ccw.cc			\
	libnurbs/internals/coveandtiler.cc		\
	libnurbs/internals/curve.cc			\
	libnurbs/internals/curvelist.cc			\
	libnurbs/internals/curvesub.cc			\
	libnurbs/internals/dataTransform.cc		\
	libnurbs/internals/displaylist.cc		\
	libnurbs/internals/flist.cc			\
	libnurbs/internals/flistsorter.cc		\
	libnurbs/internals/hull.cc			\
	libnurbs/internals/intersect.cc			\
	libnurbs/internals/knotvector.cc		\
	libnurbs/internals/mapdesc.cc			\
	libnurbs/internals/mapdescv.cc			\
	libnurbs/internals/maplist.cc			\
	libnurbs/internals/mesher.cc			\
	libnurbs/internals/monoTriangulationBackend.cc	\
	libnurbs/internals/monotonizer.cc		\
	libnurbs/internals/mycode.cc			\
	libnurbs/internals/nurbsinterfac.cc		\
	libnurbs/internals/nurbstess.cc			\
	libnurbs/internals/patch.cc			\
	libnurbs/internals/patchlist.cc			\
	libnurbs/internals/quilt.cc			\
	libnurbs/internals/reader.cc			\
	libnurbs/internals/renderhints.cc		\
	libnurbs/internals/slicer.cc			\
	libnurbs/internals/sorter.cc			\
	libnurbs/internals/splitarcs.cc			\
	libnurbs/internals/subdivider.cc		\
	libnurbs/internals/tobezier.cc			\
	libnurbs/internals/trimline.cc			\
	libnurbs/internals/trimregion.cc		\
	libnurbs/internals/trimvertpool.cc		\
	libnurbs/internals/uarray.cc			\
	libnurbs/internals/varray.cc			\
	libnurbs/nurbtess/directedLine.cc		\
	libnurbs/nurbtess/gridWrap.cc			\
	libnurbs/nurbtess/monoChain.cc			\
	libnurbs/nurbtess/monoPolyPart.cc		\
	libnurbs/nurbtess/monoTriangulation.cc		\
	libnurbs/nurbtess/partitionX.cc			\
	libnurbs/nurbtess/partitionY.cc			\
	libnurbs/nurbtess/polyDBG.cc			\
	libnurbs/nurbtess/polyUtil.cc			\
	libnurbs/nurbtess/primitiveStream.cc		\
	libnurbs/nurbtess/quicksort.cc			\
	libnurbs/nurbtess/rectBlock.cc			\
	libnurbs/nurbtess/sampleComp.cc			\
	libnurbs/nurbtess/sampleCompBot.cc		\
	libnurbs/nurbtess/sampleCompRight.cc		\
	libnurbs/nurbtess/sampleCompTop.cc		\
	libnurbs/nurbtess/sampleMonoPoly.cc		\
	libnurbs/nurbtess/sampledLine.cc		\
	libnurbs/nurbtess/searchTree.cc

SOURCES = $(C_SOURCES) $(CC_SOURCES)

C_OBJECTS = $(C_SOURCES:.c=.o)
CC_OBJECTS = $(CC_SOURCES:.cc=.o)
OBJECTS = $(C_OBJECTS) $(CC_OBJECTS)


##### RULES #####

.c.o:
	$(CC) -c $(INCDIRS) $(CFLAGS) -DNDEBUG -DLIBRARYBUILD $< -o $@

.cc.o:
	$(CXX) -c $(INCDIRS) $(CXXFLAGS) -DNDEBUG -DLIBRARYBUILD $< -o $@


##### TARGETS #####

default:
	@if [ "${CONFIG_NAME}" = "beos" ] ; then \
		echo "$(GLU_LIB_NAME) not build under BeOS, but integrated into ${GL_LIB_NAME}." ; \
		exit 0 ; \
	else \
		$(MAKE) $(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME) || exit 1 ; \
	fi

$(TOP)/$(LIB_DIR):
	-mkdir $(TOP)/$(LIB_DIR)

# Make the library:
$(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME): $(OBJECTS)
	$(MKLIB) -o $(GLU_LIB) -linker '$(CXX)' -ldflags '$(LDFLAGS)' \
		-major $(GLU_MAJOR) -minor $(GLU_MINOR) -patch $(GLU_TINY) \
		-cplusplus $(MKLIB_OPTIONS) -install $(TOP)/$(LIB_DIR) \
		-exports glu.exports -id $(INSTALL_LIB_DIR)/lib$(GLU_LIB).$(GLU_MAJOR).dylib \
		$(GLU_LIB_DEPS) $(OBJECTS)


clean:
	-rm -f *.o */*.o */*/*.o
	-rm -f *.lo */*.lo */*/*.lo
	-rm -f *.la */*.la */*/*.la
