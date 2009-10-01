
#ifndef SKYBOX_H
#define SKYBOX_H


extern GLuint
LoadSkyBoxCubeTexture(const char *filePosX,
                      const char *fileNegX,
                      const char *filePosY,
                      const char *fileNegY,
                      const char *filePosZ,
                      const char *fileNegZ);

extern void
DrawSkyBoxCubeTexture(GLuint tex);


#endif /* SKYBOX_H */
