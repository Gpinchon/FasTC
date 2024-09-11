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

#include "FasTC/CompressedImage.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FasTC/Pixel.h"

#include "FasTC/ASTCCompressor.h"
#include "FasTC/BPTCCompressor.h"
#include "FasTC/DXTCompressor.h"
#include "FasTC/ETCCompressor.h"
#include "FasTC/PVRTCCompressor.h"
#include "FasTC/TexCompTypes.h"

using FasTC::CompressionJob;
using FasTC::DecompressionJob;
using FasTC::ECompressionFormat;

CompressedImage::CompressedImage(const CompressedImage& other)
    : UncompressedImage(other)
    , m_Format(other.m_Format)
    , m_CompressedData(other.m_CompressedData)
{
}

CompressedImage::CompressedImage(
    const unsigned int width,
    const unsigned int height,
    const ECompressionFormat format,
    const unsigned char* data)
    : UncompressedImage(width, height, reinterpret_cast<uint32*>(NULL))
    , m_Format(format)
    , m_CompressedData(data, data + GetCompressedSize())
{
}

CompressedImage& CompressedImage::operator=(const CompressedImage& other)
{
    UncompressedImage::operator=(other);
    m_Format = other.m_Format;
    m_CompressedData = other.m_CompressedData;
    return *this;
}

CompressedImage::~CompressedImage()
{
}

bool CompressedImage::DecompressImage(unsigned char* outBuf, unsigned int outBufSz) const
{

    assert(outBufSz == GetUncompressedSize());

    DecompressionJob dj(m_Format, m_CompressedData.data(), outBuf, GetWidth(), GetHeight());
    if (m_Format == FasTC::eCompressionFormat_DXT1) {
        DXTC::DecompressDXT1(dj);
    } else if (m_Format == FasTC::eCompressionFormat_DXT5) {
        DXTC::DecompressDXT5(dj);
    } else if (m_Format == FasTC::eCompressionFormat_ETC1) {
        ETCC::Decompress(dj);
    } else if (FasTC::COMPRESSION_FORMAT_PVRTC_BEGIN <= m_Format && FasTC::COMPRESSION_FORMAT_PVRTC_END >= m_Format) {
#ifndef NDEBUG
        PVRTCC::Decompress(dj, PVRTCC::eWrapMode_Wrap, true);
#else
        PVRTCC::Decompress(dj);
#endif
    } else if (m_Format == FasTC::eCompressionFormat_BPTC) {
        BPTCC::Decompress(dj);
    } else if (FasTC::COMPRESSION_FORMAT_ASTC_BEGIN <= m_Format && FasTC::COMPRESSION_FORMAT_ASTC_END >= m_Format) {
        ASTCC::Decompress(dj);
    } else {
        const char* errStr = "Have not implemented decompression method.";
        fprintf(stderr, "%s\n", errStr);
        assert(!errStr);
        return false;
    }

    return true;
}

void CompressedImage::ComputePixels()
{

    uint32 unCompSz = GetWidth() * GetHeight() * 4;
    uint8* unCompBuf = new uint8[unCompSz];
    DecompressImage(unCompBuf, unCompSz);

    uint32* newPixelBuf = reinterpret_cast<uint32*>(unCompBuf);

    FasTC::Pixel* newPixels = new FasTC::Pixel[GetWidth() * GetHeight()];
    for (uint32 i = 0; i < GetWidth() * GetHeight(); i++) {
        newPixels[i].Unpack(newPixelBuf[i]);
    }

    SetImageData(GetWidth(), GetHeight(), newPixels);
}

uint32 CompressedImage::GetCompressedSize(uint32 width, uint32 height, ECompressionFormat format)
{
    // The compressed size is the block size times the number of blocks
    uint32 blockDim[2];
    GetBlockDimensions(format, blockDim);

    const uint32 blocksWide = (width + blockDim[0] - 1) / blockDim[0];
    const uint32 blocksHigh = (height + blockDim[1] - 1) / blockDim[1];
    const uint32 uncompBlockSize = blockDim[0] * blockDim[1] * sizeof(uint32);

    const uint32 nBlocks = blocksWide * blocksHigh;
    const uint32 blockSz = GetBlockSize(format);

    return nBlocks * blockSz;
}
