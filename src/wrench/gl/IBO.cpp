#include "IBO.h"

wrench::gl::IBO::IBO(void)
{
  m_iboID = 0;
}

wrench::gl::IBO::~IBO()
{
  if(0 != m_iboID)
  {
    glDeleteFramebuffersEXT(1, &m_iboID);
  }
}

bool wrench::gl::IBO::init(GLint componentSize, GLenum componentType)
{
  m_componentSize = componentSize;  //  Set the component size that was passed in
  m_componentType = componentType;  //  Set the component type that was passed in

  glGenBuffers(1, &m_iboID);        //  Create a buffer on the GPU for the indicies

  OGLStatus::logOGLErrors("wrench::gl::VBO - init()");  // Log any OpenGL errors
  return (0 != m_iboID);
}

void wrench::gl::IBO::bufferData(GLsizei size, const GLvoid* data, GLenum usage)
{
  bind();
  m_componentCount = size;

  glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_componentCount * m_componentSize * Converter::typeToSize(m_componentType), data, usage);
  unbind();

  OGLStatus::logOGLErrors("wrench::gl::VBO - bufferData()");  // Log any OpenGL errors
}

GLsizei wrench::gl::IBO::getComponentCount(void)
{
  return m_componentCount;
}

GLint wrench::gl::IBO::getComponentSize(void)
{
  return m_componentSize;
}

GLenum wrench::gl::IBO::getComponentType(void)
{
  return m_componentType;
}

void wrench::gl::IBO::bind()
{
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iboID); // Bind the IBO as the current GL_ELEMENT_ARRAY_BUFFER
  OGLStatus::logOGLErrors("wrench::gl::VBO - _bind()");
}

void wrench::gl::IBO::unbind()
{
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);       // Unbind the IBO, setting 0 as the current GL_ELEMENT_ARRAY_BUFFER
  OGLStatus::logOGLErrors("wrench::gl::VBO - _unbind()");
}
