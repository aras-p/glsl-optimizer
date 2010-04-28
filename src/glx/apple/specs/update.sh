for f in enum.spec enumext.spec gl.spec gl.tm glx.spec glxenum.spec glxenumext.spec glxext.spec ; do
	curl -LO http://www.opengl.org/registry/api/$f
done

