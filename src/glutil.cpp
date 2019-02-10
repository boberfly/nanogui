/*
    src/glutil.cpp -- Convenience classes for accessing OpenGL >= 3.x

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#if defined(NANOGUI_USE_GLWIDGETS)

#include <nanogui/glutil.h>
#include <iostream>
#include <fstream>
#include <sstream>

#if !defined(GL_RGBA8)
#  define GL_RGBA8            0x8058
#endif
#if !defined(GL_BGRA)
#  define GL_BGRA             0x80E1
#endif
#if !defined(GL_DEPTH24_STENCIL8)
#  define GL_DEPTH24_STENCIL8 0x8FF0
#endif

NAMESPACE_BEGIN(nanogui)

static GLuint create_shader_helper(GLint type, const std::string &name,
                                  const std::string &defines,
                                  std::string shader_string) {
    if (shader_string.empty())
        return (GLuint) 0;

    if (!defines.empty()) {
        if (shader_string.length() > 8 && shader_string.substr(0, 8) == "#version") {
            std::istringstream iss(shader_string);
            std::ostringstream oss;
            std::string line;
            std::getline(iss, line);
            oss << line << std::endl;
            oss << defines;
            while (std::getline(iss, line))
                oss << line << std::endl;
            shader_string = oss.str();
        } else {
            shader_string = defines + shader_string;
        }
    }

    GLuint id = glCreateShader(type);
    const char *shader_string_const = shader_string.c_str();
    glShaderSource(id, 1, &shader_string_const, nullptr);
    glCompileShader(id);

    GLint status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE) {
        char buffer[4096];
        std::cerr << "Error while compiling ";
        if (type == GL_VERTEX_SHADER)
            std::cerr << "vertex shader";
        else if (type == GL_FRAGMENT_SHADER)
            std::cerr << "fragment shader";
#if defined(NANOGUI_USE_OPENGL)
        else if (type == GL_GEOMETRY_SHADER)
            std::cerr << "geometry shader";
#endif
        std::cerr << " \"" << name << "\":" << std::endl;
        std::cerr << shader_string << std::endl << std::endl;
        glGetShaderInfoLog(id, sizeof(buffer), nullptr, buffer);
        std::cerr << "Error: " << std::endl << buffer << std::endl;
        throw std::runtime_error("Shader compilation failed!");
    }

    return id;
}

bool GLShader::init_from_files(
    const std::string &name,
    const std::string &vertex_fname,
    const std::string &fragment_fname,
    const std::string &geometry_fname) {
    auto file_to_string = [](const std::string &filename) -> std::string {
        if (filename.empty())
            return "";
        std::ifstream t(filename);
        return std::string((std::istreambuf_iterator<char>(t)),
                           std::istreambuf_iterator<char>());
    };

    return init(name,
                file_to_string(vertex_fname),
                file_to_string(fragment_fname),
                file_to_string(geometry_fname));
}

bool GLShader::init(const std::string &name,
                    const std::string &vertex_str,
                    const std::string &fragment_str,
                    const std::string &geometry_str) {
    std::string defines;
    for (auto def : m_definitions)
        defines += std::string("#define ") + def.first + std::string(" ") + def.second + "\n";

#if defined(NANOGUI_USE_OPENGL)
    glGenVertexArrays(1, &m_vertex_array_object);
#endif
    m_name = name;
    m_vertex_shader =
        create_shader_helper(GL_VERTEX_SHADER, name, defines, vertex_str);
#if defined(NANOGUI_USE_OPENGL)
    m_geometry_shader =
        create_shader_helper(GL_GEOMETRY_SHADER, name, defines, geometry_str);
#else
    if (!geometry_str.empty())
        throw std::runtime_error("Geometry shaders are not supported on GLES2!");
#endif
    m_fragment_shader =
        create_shader_helper(GL_FRAGMENT_SHADER, name, defines, fragment_str);

    if (!m_vertex_shader || !m_fragment_shader)
        return false;
#if defined(NANOGUI_USE_OPENGL)
    if (!geometry_str.empty() && !m_geometry_shader)
        return false;
#endif

    m_program_shader = glCreateProgram();

    glAttachShader(m_program_shader, m_vertex_shader);
    glAttachShader(m_program_shader, m_fragment_shader);

#if defined(NANOGUI_USE_OPENGL)
    if (m_geometry_shader)
        glAttachShader(m_program_shader, m_geometry_shader);
#endif

    glLinkProgram(m_program_shader);

    GLint status;
    glGetProgramiv(m_program_shader, GL_LINK_STATUS, &status);

    if (status != GL_TRUE) {
        char buffer[512];
        glGetProgramInfoLog(m_program_shader, 512, nullptr, buffer);
        std::cerr << "Linker error (" << m_name << "): " << std::endl << buffer << std::endl;
        m_program_shader = 0;
        throw std::runtime_error("Shader linking failed!");
    }

    return true;
}

void GLShader::bind() {
    glUseProgram(m_program_shader);
#if defined(NANOGUI_USE_OPENGL)
    glBindVertexArray(m_vertex_array_object);
#else
    for (const auto &it : m_buffer_objects) {
        const auto &buf = it.second;
        if (buf.attrib_id == -1) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.id);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, buf.id);
            glEnableVertexAttribArray(buf.attrib_id);
            glVertexAttribPointer(buf.attrib_id, (GLint) buf.dim, buf.gl_type, buf.integral, 0, 0);
        }
    }
#endif
}

GLint GLShader::attrib(const std::string &name, bool warn) const {
    GLint id = glGetAttribLocation(m_program_shader, name.c_str());
    if (id == -1 && warn)
        std::cerr << m_name << ": warning: did not find attrib " << name << std::endl;
    return id;
}

GLint GLShader::uniform(const std::string &name, bool warn) const {
    GLint id = glGetUniformLocation(m_program_shader, name.c_str());
    if (id == -1 && warn)
        std::cerr << m_name << ": warning: did not find uniform " << name << std::endl;
    return id;
}

void GLShader::upload_attrib(const std::string &name, size_t dim, size_t count,
                             size_t comp_size, GLuint gl_type, bool integral,
                             const void *data, int version) {
    GLint attrib_id = -1;
    if (name != "indices") {
        attrib_id = attrib(name);
        if (attrib_id < 0)
            return;
    }
    size_t size = count * dim * comp_size;

    GLuint buffer_id;
    auto it = m_buffer_objects.find(name);
    if (it != m_buffer_objects.end()) {
        Buffer &buffer = it->second;
        buffer_id = it->second.id;
        buffer.version = version;
        buffer.size = size;
        buffer.comp_size = comp_size;
        buffer.attrib_id = attrib_id;
        buffer.integral = integral;
    } else {
        glGenBuffers(1, &buffer_id);
        Buffer buffer;
        buffer.id = buffer_id;
        buffer.gl_type = gl_type;
        buffer.dim = dim;
        buffer.comp_size = comp_size;
        buffer.size = size;
        buffer.version = version;
        buffer.attrib_id = attrib_id;
        buffer.integral = integral;
        buffer.owned = true;
        m_buffer_objects[name] = buffer;
    }

    if (name == "indices") {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
        if (size == 0) {
            glDisableVertexAttribArray(attrib_id);
        } else {
            glEnableVertexAttribArray(attrib_id);
            glVertexAttribPointer(attrib_id, (GLint) dim, gl_type, integral, 0, 0);
        }
    }
}

size_t GLShader::attrib_size(const std::string &name) const {
    auto it = m_buffer_objects.find(name);
    if (it == m_buffer_objects.end())
        throw std::runtime_error("attrib_size(" + m_name + ", " + name + ") : buffer not found!");
    return it->second.size;
}

void GLShader::download_attrib(const std::string &name, void *data) {
#if defined(NANOGUI_USE_OPENGL)
    auto it = m_buffer_objects.find(name);
    if (it == m_buffer_objects.end())
        throw std::runtime_error("download_attrib(" + m_name + ", " + name + ") : buffer not found!");

    const Buffer &buf = it->second;
    if (name == "indices") {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.id);
        glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, buf.size, data);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, buf.id);
        glGetBufferSubData(GL_ARRAY_BUFFER, 0, buf.size, data);
    }
#else
    (void) name; (void) data;
    throw std::runtime_error("download_attrib(): unsupported on GLES2!");
#endif
}

void GLShader::share_attrib(const GLShader &other_shader, const std::string &name, const std::string &_as) {
    std::string as = _as.length() == 0 ? name : _as;
    auto it = other_shader.m_buffer_objects.find(name);
    if (it == other_shader.m_buffer_objects.end())
        throw std::runtime_error("share_attribute(" + other_shader.m_name + ", " + name + "): attribute not found!");
    Buffer buffer = it->second;
    buffer.owned = false;
    buffer.attrib_id = attrib(as);
    m_buffer_objects[name] = buffer;

    if (name != "indices") {
        if (buffer.attrib_id < 0)
            return;
        glEnableVertexAttribArray(buffer.attrib_id);
        glBindBuffer(GL_ARRAY_BUFFER, buffer.id);
        glVertexAttribPointer(buffer.attrib_id, (GLint) buffer.dim, buffer.gl_type,
                              buffer.comp_size == 1 ? GL_TRUE : GL_FALSE, 0, 0);
    } else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.id);
    }
}

void GLShader::invalidate_attribs() {
    for (auto &buffer : m_buffer_objects)
        buffer.second.version = -1;
}

void GLShader::free_attrib(const std::string &name) {
    auto it = m_buffer_objects.find(name);
    if (it != m_buffer_objects.end()) {
        if (it->second.owned)
            glDeleteBuffers(1, &it->second.id);
        m_buffer_objects.erase(it);
    }
}

void GLShader::draw_indexed(int type, uint32_t offset_, uint32_t count_) {
    if (count_ == 0)
        return;
    size_t offset = offset_;
    size_t count = count_;

    switch (type) {
        case GL_TRIANGLES: offset *= 3; count *= 3; break;
        case GL_LINES: offset *= 2; count *= 2; break;
    }

    glDrawElements(type, (GLsizei) count, GL_UNSIGNED_INT,
                   (const void *)(offset * sizeof(uint32_t)));
}

void GLShader::draw_array(int type, uint32_t offset, uint32_t count) {
    if (count == 0)
        return;

    glDrawArrays(type, offset, count);
}

void GLShader::free() {
    for (auto &buf: m_buffer_objects) {
        if (buf.second.owned)
            glDeleteBuffers(1, &buf.second.id);
    }
    m_buffer_objects.clear();

#if defined(NANOGUI_USE_OPENGL)
    if (m_vertex_array_object) {
        glDeleteVertexArrays(1, &m_vertex_array_object);
        m_vertex_array_object = 0;
    }
#endif

    if (m_program_shader) {
        glDeleteProgram(m_program_shader);
        m_program_shader = 0;
    }
    if (m_vertex_shader) {
        glDeleteShader(m_vertex_shader);
        m_vertex_shader = 0;
    }

    if (m_fragment_shader) {
        glDeleteShader(m_fragment_shader);
        m_fragment_shader = 0;
    }

#if defined(NANOGUI_USE_OPENGL)
if (m_geometry_shader) {
    glDeleteShader(m_geometry_shader);
    m_geometry_shader = 0;
}
#endif
}

const GLShader::Buffer &GLShader::attrib_buffer(const std::string &name) {
    for (auto &pair : m_buffer_objects) {
        if (pair.first == name)
            return pair.second;
    }

    throw std::runtime_error(m_name + ": attrib_buffer: " + name + " not found!");
}

//  ----------------------------------------------------

void GLFramebuffer::init(const Vector2i &size, int n_samples) {
    m_size = size;
    m_samples = n_samples;

    glGenRenderbuffers(1, &m_color);
    glBindRenderbuffer(GL_RENDERBUFFER, m_color);

#if defined(NANOGUI_USE_OPENGL)
    if (n_samples <= 1)
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, size.x(), size.y());
    else
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, n_samples, GL_RGBA8, size.x(), size.y());
#else
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, size.x(), size.y());
#endif

    glGenRenderbuffers(1, &m_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depth);

#if defined(NANOGUI_USE_OPENGL)
    if (n_samples <= 1)
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.x(), size.y());
    else
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, n_samples, GL_DEPTH24_STENCIL8, size.x(), size.y());
#else
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.x(), size.y());
#endif

    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_color);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depth);

#if defined(NANOGUI_USE_OPENGL)
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
#endif

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Could not create framebuffer object!");

    release();
}

void GLFramebuffer::free() {
    glDeleteRenderbuffers(1, &m_color);
    glDeleteRenderbuffers(1, &m_depth);
    m_color = m_depth = 0;
}

void GLFramebuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
#if defined(NANOGUI_USE_OPENGL)
    if (m_samples > 1)
        glEnable(GL_MULTISAMPLE);
#endif
}

void GLFramebuffer::release() {
#if defined(NANOGUI_USE_OPENGL)
    if (m_samples > 1)
        glDisable(GL_MULTISAMPLE);
#endif
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFramebuffer::blit() {
#if defined(NANOGUI_USE_OPENGL)
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);

    glBlitFramebuffer(0, 0, m_size.x(), m_size.y(), 0, 0, m_size.x(), m_size.y(),
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#else
    throw std::runtime_error("GLFramebuffer::blit(): Unimplemented for GLES2!");
#endif
}

void GLFramebuffer::download_tga(const std::string &filename) {
#if defined(NANOGUI_USE_OPENGL)
    uint8_t *temp = new uint8_t[hprod(m_size) * 4];

    std::cout << "Writing \"" << filename  << "\" (" << m_size.x() << "x" << m_size.y() << ") .. ";
    std::cout.flush();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glReadPixels(0, 0, m_size.x(), m_size.y(), GL_BGRA, GL_UNSIGNED_BYTE, temp);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    uint32_t row_size = m_size.x() * 4;
    uint32_t half_height = m_size.y() / 2;
    uint8_t *line = (uint8_t *) alloca(row_size);
    for (uint32_t i=0, j=m_size.y()-1; i<half_height; ++i) {
        memcpy(line, temp + i * row_size, row_size);
        memcpy(temp + i * row_size, temp + j * row_size, row_size);
        memcpy(temp + j * row_size, line, row_size);
        j--;
    }

    FILE *tga = fopen(filename.c_str(), "wb");
    if (tga == nullptr)
        throw std::runtime_error("GLFramebuffer::download_tga(): Could not open output file");
    fputc(0, tga); /* ID */
    fputc(0, tga); /* Color map */
    fputc(2, tga); /* Image type */
    fputc(0, tga); fputc(0, tga); /* First entry of color map (unused) */
    fputc(0, tga); fputc(0, tga); /* Length of color map (unused) */
    fputc(0, tga); /* Color map entry size (unused) */
    fputc(0, tga); fputc(0, tga);  /* X offset */
    fputc(0, tga); fputc(0, tga);  /* Y offset */
    fputc(m_size.x() % 256, tga); /* Width */
    fputc(m_size.x() / 256, tga); /* continued */
    fputc(m_size.y() % 256, tga); /* Height */
    fputc(m_size.y() / 256, tga); /* continued */
    fputc(32, tga);   /* Bits per pixel */
    fputc(0x20, tga); /* Scan from top left */
    fwrite(temp, hprod(m_size) * 4, 1, tga);
    fclose(tga);

    delete[] temp;
    std::cout << "done." << std::endl;
#else
    (void) filename;
    throw std::runtime_error("GLFramebuffer::download_tga(): Unimplemented for GLES2!");
#endif
}

NAMESPACE_END(nanogui)

#endif
