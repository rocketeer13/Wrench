/*
 *  Texture.cpp
 *
 *  Created by Nikolaus Karpinsky on 9/20/10.
 *  Copyright 2010 ISU. All rights reserved.
 */

#include "Texture.h"

wrench::gl::Texture::Texture()
{
  m_width = 0;
  m_height = 0;
  m_fbo = nullptr;
}

wrench::gl::Texture::~Texture()
{
  //	If we have a width and height we assume to have a texture
  if(m_width != 0 && m_height != 0)
  {
	glDeleteTextures(1, &m_textureId);
	glDeleteBuffers(1, &m_PBOId);
  }

  if(nullptr != m_fbo)
  {
	delete m_fbo;
  }
}

bool wrench::gl::Texture::init(const GLuint width, const GLuint height, const GLint internalFormat, const GLenum format, const GLenum dataType)
{
  m_width = width;
  m_height = height;
  m_internalFormat = internalFormat;
  m_format = format;
  m_dataType = dataType;

  m_dataSize = Converter::typeToSize(m_dataType);

  //	Initialize the PBO
  glGenBuffers(1, &m_PBOId);

  //	Initialize the Texture
  //	Generate a texture and bind it
  glGenTextures(1, &m_textureId);
  glBindTexture(GL_TEXTURE_2D, m_textureId);

  //	Turn off filtering and wrapping
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  //	Allocate memory for the texture
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, 0);

  OGLStatus::logOGLErrors("Texture - init()");
  return true;
}

bool wrench::gl::Texture::reinit(const GLuint width, const GLuint height, const GLint internalFormat, const GLenum format, const GLenum dataType)
{
  //	If we have a width and height we assume to have a texture
  if(m_width != 0 && m_height != 0)
  {
	glDeleteTextures(1, &m_textureId);
	glDeleteBuffers(1, &m_PBOId);
  }

  return init(width, height, internalFormat, format, dataType);
}

void wrench::gl::Texture::bind()
{
  glBindTexture(GL_TEXTURE_2D, m_textureId);
}

void wrench::gl::Texture::bind(GLenum texture)
{
  glActiveTexture(texture);
  glBindTexture(GL_TEXTURE_2D, m_textureId);
}

void wrench::gl::Texture::unbind()
{
  glBindTexture(GL_TEXTURE_2D, 0);
}

const GLuint wrench::gl::Texture::getTextureId(void) const
{
  return m_textureId;
}

const GLenum wrench::gl::Texture::getTextureTarget(void) const
{
  return GL_TEXTURE_2D;
}

const int wrench::gl::Texture::getChannelCount(void) const
{
  int channelCount = 1;

  if(m_format == GL_RGB || m_format == GL_BGR)
  {
	channelCount = 3;
  }
  else if(m_format == GL_RGBA || m_format == GL_BGRA)
  {
	channelCount = 4;
  }

  return channelCount;
}

const GLuint wrench::gl::Texture::getWidth(void) const
{
  return m_width;
}

const GLuint wrench::gl::Texture::getHeight(void) const
{
  return m_height;
}

const GLuint wrench::gl::Texture::getFormat(void) const
{
  return m_format;
}

const GLuint wrench::gl::Texture::getDataType(void) const
{
  return m_dataType;
}

bool wrench::gl::Texture::transferFromTexture(IplImage* image)
{
  //  TODO: Not sure if this needs to be bound here
  bind(GL_TEXTURE5);

  if(nullptr == m_fbo)
  {
	m_fbo = new FBO();
	m_fbo->init(m_width, m_height);
	m_fbo->setTextureAttachPoint(*this, GL_COLOR_ATTACHMENT0);
  }

  bool compatible = _checkImageCompatibility(image);

  if(compatible)
  {
	const int channelCount = getChannelCount();

	m_fbo->bind();	
	m_fbo->bindDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBOId);
	glBufferData(GL_PIXEL_PACK_BUFFER, m_width * m_height * channelCount * m_dataSize, nullptr, GL_STREAM_READ);
	glReadPixels(0, 0, m_width, m_height, m_format, m_dataType, 0);

	if(GL_FLOAT == getDataType())
	{
	  float* gpuMem = (float*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	  //  Actual data transfer
	  for (unsigned int i = 0; i < m_height; i++)
	  {
		  //  OpenCV does not guarentee continous memory blocks so it has to be copied row by row
		  memcpy(image->imageData + (i * image->widthStep), gpuMem + (i * m_width * 3), m_width * channelCount * m_dataSize);
	  }
	}
	else
	{
	  char* gpuMem = (char*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	  //  Actual data transfer
	  for (unsigned int i = 0; i < m_height; i++)
	  {
		  //  OpenCV does not guarentee continous memory blocks so it has to be copied row by row
		  memcpy(image->imageData + (i * image->widthStep), gpuMem + (i * m_width * 3), m_width * channelCount * m_dataSize);
	  }
	}

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER); // release pointer to mapping buffer
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  }
  return compatible;
}

bool wrench::gl::Texture::transferToTexture(const IplImage* image)
{
  bool compatible = _checkImageCompatibility(image);

  if(compatible)
  {
	const int channelCount = getChannelCount();

	bind();
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_PBOId);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, m_width * m_height * channelCount * m_dataSize, nullptr, GL_STREAM_DRAW);
	char* gpuMem = (char*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

	//  Actual data transfer
	for (unsigned int i = 0; i < m_height; i++)
	{
		//  OpenCV does not guarentee continous memory blocks so it has to be copied row by row
		memcpy(gpuMem + (i * m_width * 3), image->imageData + (i * image->widthStep), m_width * channelCount * m_dataSize);
	}

	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release pointer to mapping buffer
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, m_format, m_dataType, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  }
  return compatible;
}

//  Check for reactor. If its present build with functionality otherwise dont
//  so that we can limit dependencies
#ifdef USE_REACTOR
bool wrench::gl::Texture::transferToTexture(reactor::MediaFrame& frame)
{
 bool compatible = _checkImageCompatibility(frame);

  if(compatible)
  {
	const int channelCount = getChannelCount();

	bind();
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_PBOId);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, m_width * m_height * channelCount * m_dataSize, nullptr, GL_STREAM_DRAW);
	char* gpuMem = (char*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

	//  Actual data transfer
	for (unsigned int i = 0; i < m_height; i++)
	{
		//  OpenCV does not guarentee continous memory blocks so it has to be copied row by row
		memcpy(gpuMem + (i * m_width * 3), (frame.getBuffer())[0] + (i * frame.getFrame()->linesize[0]), m_width * channelCount * m_dataSize);
	}

	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release pointer to mapping buffer
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, m_format, m_dataType, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  }
  return compatible;
}
#endif

bool wrench::gl::Texture::_checkImageCompatibility(const IplImage* image) const
{
  bool compatible = false;

  //    Number of channels must be the same so that memcpy can be used. This is for
  //    the shear speed of memcpy
  if(image->width	== (GLint)m_width &&
	  image->height	== (GLint)m_height &&
	  image->nChannels  == getChannelCount())
  {
	compatible = true;
  }

  return compatible;
}

#ifdef USE_REACTOR
bool wrench::gl::Texture::_checkImageCompatibility(const reactor::MediaFrame& frame) const
{
  bool compatible = false;

  //    Number of channels must be the same so that memcpy can be used. This is for
  //    the shear speed of memcpy
  if(frame.getWidth()	== (GLint)m_width &&
	 frame.getHeight()	== (GLint)m_height &&
	  3  == getChannelCount())
  {
	compatible = true;
  }

  return compatible;
}
#endif
