#pragma once

class FrameBuffer
{
public:
    FrameBuffer(int screenW, int screenH);
    ~FrameBuffer();

    unsigned int getFrameBufferID() const { return m_fboID; }
    unsigned int getColorTextureID() const { return m_colorTexID; }
    unsigned int getDepthTextureID() const { return m_depthTexID; }

private:
    unsigned int TEXTURE_SIZE;

    unsigned int m_colorTexID;
    unsigned int m_depthTexID;
    unsigned int m_fboID;
};