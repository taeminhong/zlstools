// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include <shlwapi.h>
#include <Wincrypt.h>   // For CryptStringToBinary.
#include <thumbcache.h> // For IThumbnailProvider.
#include <wincodec.h>   // Windows Imaging Codecs
#include <msxml6.h>
#include <tchar.h>
#include <new>
#include <zls/zls.hpp>
#include <zls/image.hpp>
#include <zls/draping.hpp>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "msxml6.lib")

class ZlsStream : public zls::File
{
public:
    ZlsStream(IStream* pStream)
        : _pStream(pStream)
    {
    }
    uint_type read(void* buf, uint_type size)
    {
        ULONG nRead = 0;
        _pStream->Read(buf, size, &nRead);
        return nRead;
    }
    int_type tell()
    {
        ULARGE_INTEGER pos;
        LARGE_INTEGER move;
        move.QuadPart = 0;
        HRESULT hr = _pStream->Seek(move, STREAM_SEEK_CUR, &pos);
        if (FAILED(hr))
            return -1;
        return pos.QuadPart;
    }
    bool seek(int_type offset, int_type origin)
    {
        DWORD dwOrigin;
        switch (origin)
        {
        case Set:
            dwOrigin = STREAM_SEEK_SET;
            break;
        case Cur:
            dwOrigin = STREAM_SEEK_CUR;
            break;
        case End:
            dwOrigin = STREAM_SEEK_END;
            break;
        default:
            return false;
        }
        ULARGE_INTEGER pos;
        LARGE_INTEGER move;
        move.QuadPart = offset;
        HRESULT hr = _pStream->Seek(move, dwOrigin, &pos);
        return SUCCEEDED(hr);
    }
private:
    IStream* _pStream;
};

// this thumbnail provider implements IInitializeWithStream to enable being hosted
// in an isolated process for robustness

class CRecipeThumbProvider : public IInitializeWithStream,
    public IThumbnailProvider
{
public:
    CRecipeThumbProvider() : _cRef(1), _pStream(NULL)
    {
    }

    virtual ~CRecipeThumbProvider()
    {
        if (_pStream)
        {
            _pStream->Release();
        }
    }

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        static const QITAB qit[] =
        {
            QITABENT(CRecipeThumbProvider, IInitializeWithStream),
            QITABENT(CRecipeThumbProvider, IThumbnailProvider),
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        ULONG cRef = InterlockedDecrement(&_cRef);
        if (!cRef)
        {
            delete this;
        }
        return cRef;
    }

    // IInitializeWithStream
    IFACEMETHODIMP Initialize(IStream* pStream, DWORD grfMode);

    // IThumbnailProvider
    IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP* phbmp, WTS_ALPHATYPE* pdwAlpha);

private:
    long _cRef;
    IStream* _pStream;     // provided during initialization.
};

HRESULT CRecipeThumbProvider_CreateInstance(REFIID riid, void** ppv)
{
    CRecipeThumbProvider* pNew = new (std::nothrow) CRecipeThumbProvider();
    HRESULT hr = pNew ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        hr = pNew->QueryInterface(riid, ppv);
        pNew->Release();
    }
    return hr;
}

// IInitializeWithStream
IFACEMETHODIMP CRecipeThumbProvider::Initialize(IStream* pStream, DWORD)
{
    HRESULT hr = E_UNEXPECTED;  // can only be inited once
    if (_pStream != NULL)
        return E_UNEXPECTED;

        // take a reference to the stream if we have not been inited yet
    hr = pStream->QueryInterface(&_pStream);
    if (FAILED(hr))
        return hr;
    

    return hr;
}

void rgba2bgra(uint8_t* pixel)
{
    std::swap(pixel[0], pixel[2]);
}

HBITMAP create_thumbnail(const zls::Image* image)
{
    if (!image || image->empty())
        return NULL;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = image->width();
    bmi.bmiHeader.biHeight = -static_cast<LONG>(image->height());
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    BYTE* pBits;
    HBITMAP hbmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBits), NULL, 0);
    HRESULT hr = hbmp ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        for (int y = 0; y < image->height(); ++y) {
            memcpy(pBits, image->scanline(y), image->stride());
            for (int x = 0; x < image->width(); ++x)
                rgba2bgra(pBits + x * 4);
            pBits += image->stride();
        }
        return hbmp;
    }
    else
    {
        DeleteObject(hbmp);
        return NULL;
    }
}

// IThumbnailProvider
IFACEMETHODIMP CRecipeThumbProvider::GetThumbnail(UINT cx, HBITMAP* phbmp, WTS_ALPHATYPE* pdwAlpha)
{
    try {
        ZlsStream stream(_pStream);
        zls::Archive zls;
        if (!zls.open(&stream, 0))
            return E_FAIL;

        zls::Uuid thumbnail_uuid;
        zls::Image thumbnail;
        if (!zls::read_thumbnail_image(zls, &thumbnail, &thumbnail_uuid)) {
            return E_FAIL;
        }

        zls::Image small_image = thumbnail.resize(256, 256);

        *phbmp = create_thumbnail(&small_image);
        if (*phbmp) {
            *pdwAlpha = WTSAT_ARGB;
            return S_OK;
        }
        else 
        {
            return E_FAIL;
        }
    }
    catch (...) {
        return E_FAIL;
    }
}
