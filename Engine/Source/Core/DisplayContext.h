#pragma once

class DisplayContext
{
    DECLARE_SINGLE(DisplayContext)

public:
    void SetSize(UINT width, UINT height)
    {
        assert(width  > 0 && "DisplayContext: width must be > 0");
        assert(height > 0 && "DisplayContext: height must be > 0");
        _width  = width;
        _height = height;
    }

    UINT  GetWidth()   const { return _width;  }
    UINT  GetHeight()  const { return _height; }
    float GetWidthF()  const { return static_cast<float>(_width);  }
    float GetHeightF() const { return static_cast<float>(_height); }
    float GetAspect()  const { return static_cast<float>(_width) / static_cast<float>(_height); }

private:
    UINT _width  = 0;
    UINT _height = 0;
};
