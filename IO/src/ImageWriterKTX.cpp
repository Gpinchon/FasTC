// Copyright 2016 The University of North Carolina at Chapel Hill
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Please send all BUG REPORTS to <pavel@cs.unc.edu>.
// <http://gamma.cs.unc.edu/FasTC/>

#include "ImageWriterKTX.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "FasTC/Image.h"
#include "FasTC/Pixel.h"

#include "FasTC/CompressedImage.h"

#include "GLDefines.h"

ImageWriterKTX::ImageWriterKTX(FasTC::Image<>& im)
    : ImageWriter(im.GetWidth(), im.GetHeight(), NULL)
    , m_Image(im)
{
}

class ByteWriter {
private:
    std::vector<uint8> m_Buffer;

public:
    ByteWriter(const std::vector<uint8>& dst)
        : m_Buffer(dst)
    {
    }

    std::vector<uint8>& GetBytes() { return m_Buffer; }
    uint32 GetBytesWritten() const { return m_Buffer.size(); }

    void Write(const void* src, const uint32 nBytes)
    {
        m_Buffer.reserve(m_Buffer.size() + nBytes);
        for (uint32 i = 0; i < nBytes; i++)
            m_Buffer.emplace_back(*((uint8*)src + i));
    }

    void Write(const uint32 v)
    {
        Write(&v, 4);
    }
};

bool ImageWriterKTX::WriteImage()
{
    ByteWriter wtr(m_RawFileData);

    const uint8 kIdentifier[12] = {
        0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
    };
    wtr.Write(kIdentifier, 12);
    wtr.Write(0x04030201);

    const char* orientationKey = "KTXorientation";
    uint32 oKeyLen = static_cast<uint32>(strlen(orientationKey));

    const char* orientationValue = "S=r,T=d";
    uint32 oValLen = static_cast<uint32>(strlen(orientationValue));

    const uint32 kvSz = oKeyLen + 1 + oValLen + 1;
    uint32 tkvSz = kvSz + 4; // total kv size
    tkvSz = (tkvSz + 3) & ~0x3; // 4-byte aligned

    CompressedImage* ci = dynamic_cast<CompressedImage*>(&m_Image);
    if (ci) {
        wtr.Write(0); // glType
        wtr.Write(1); // glTypeSize
        wtr.Write(0); // glFormat must be zero for compressed images...
        switch (ci->GetFormat()) {
        case FasTC::eCompressionFormat_BPTC:
            wtr.Write(GL_COMPRESSED_RGBA_BPTC_UNORM); // glInternalFormat
            wtr.Write(GL_RGBA); // glBaseFormat
            break;

        case FasTC::eCompressionFormat_PVRTC4:
            wtr.Write(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG); // glInternalFormat
            wtr.Write(GL_RGBA); // glBaseFormat
            break;

        case FasTC::eCompressionFormat_DXT1:
            wtr.Write(GL_COMPRESSED_RGB_S3TC_DXT1_EXT); // glInternalFormat
            wtr.Write(GL_RGB); // glBaseFormat
            break;

        case FasTC::eCompressionFormat_DXT5:
            wtr.Write(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT); // glInternalFormat
            wtr.Write(GL_RGBA); // glBaseFormat
            break;

        default:
            fprintf(stderr, "Unsupported KTX compressed format: %d\n", ci->GetFormat());
            m_RawFileData = wtr.GetBytes();
            return false;
        }
    } else {
        wtr.Write(GL_BYTE); // glType
        wtr.Write(1); // glTypeSize
        wtr.Write(GL_RGBA); // glFormat
        wtr.Write(GL_RGBA8); // glInternalFormat
        wtr.Write(GL_RGBA); // glBaseFormat
    }

    wtr.Write(m_Width); // pixelWidth
    wtr.Write(m_Height); // pixelHeight
    wtr.Write(0); // pixelDepth
    wtr.Write(0); // numberOfArrayElements
    wtr.Write(1); // numberOfFaces
    wtr.Write(1); // numberOfMipmapLevels
    wtr.Write(tkvSz); // total key value size
    wtr.Write(kvSz); // key value size
    wtr.Write(orientationKey, oKeyLen + 1); // key
    wtr.Write(orientationValue, oValLen + 1); // value
    wtr.Write(orientationKey, tkvSz - kvSz - 4); // padding

    if (ci && (ci->GetFormat() == FasTC::eCompressionFormat_BPTC || ci->GetFormat() == FasTC::eCompressionFormat_DXT5)) {
        static const uint32 kImageSize = m_Width * m_Height;
        wtr.Write(kImageSize); // imageSize
        wtr.Write(ci->GetCompressedData(), kImageSize); // imagedata...
    } else if (ci && (ci->GetFormat() == FasTC::eCompressionFormat_PVRTC4 || ci->GetFormat() == FasTC::eCompressionFormat_DXT1)) {
        static const uint32 kImageSize = m_Width * m_Height >> 1;
        wtr.Write(kImageSize); // imageSize
        wtr.Write(ci->GetCompressedData(), kImageSize); // imagedata...
    } else {
        static const uint32 kImageSize = m_Width * m_Height * 4;
        wtr.Write(kImageSize); // imageSize
        wtr.Write(m_Image.GetPixels(), kImageSize); // imagedata...
    }

    m_RawFileData = wtr.GetBytes();
    return true;
}
