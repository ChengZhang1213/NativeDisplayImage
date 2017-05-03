//
// Created by Android on 2017/5/3.
//

#ifndef NATIVEDISPLAYIMAGE_BUFFER_H
#define NATIVEDISPLAYIMAGE_BUFFER_H

#include <GLES2/gl2.h>

#define BUFFER_OFFSET(i) ((void*)(i))

GLuint create_vbo(const GLsizeiptr size, const GLvoid* data, const GLenum usage);



#endif //NATIVEDISPLAYIMAGE_BUFFER_H
