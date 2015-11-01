#pragma once

class IRenderer
{
public:
    virtual void Render(HDC hDC, const RECT& rect) = 0;
};
    
class CProgressRenderer : public IRenderer
{
public:
    CProgressRenderer();
    ~CProgressRenderer();

    bool SetPos(int nPos, int nCount);

    void Render(HDC hDC, const RECT& rect);

private:
    CComPtr<IPicture> m_spPicture;
    int m_nWidth, m_nHeight;
    HBITMAP m_hbmDst;
    HDC m_hdcSrc, m_hdcDst;
    int m_nPos;
};
