//
// Created by Android on 2017/5/3.
//

#ifndef NATIVEDISPLAYIMAGE_TEXTURE_H
#define NATIVEDISPLAYIMAGE_TEXTURE_H

#include <GLES2/gl2.h>

GLuint load_texture(
        const GLsizei width, const GLsizei height,
        const GLenum type, const GLvoid* pixels);


#endif //NATIVEDISPLAYIMAGE_TEXTURE_H
