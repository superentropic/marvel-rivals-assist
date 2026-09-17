#pragma once
#include <d3d12.h>
#include <string>
#include <vector>
#include <wincodec.h>
#include "../ThirdParty/ImGui/imgui.h"

#pragma comment(lib, "windowscodecs.lib")

namespace CharacterTexture {

    // SRV slot: after Logo (slot 64) → slot 65
    static constexpr UINT SRV_SLOT = 65;

    static ID3D12Resource*             s_resource = nullptr;
    static D3D12_GPU_DESCRIPTOR_HANDLE s_gpuHandle = {};
    static ImTextureID                 s_texID    = nullptr;
    static bool                        s_loaded   = false;
    static bool                        s_tried    = false;
    static UINT                        s_width    = 0;
    static UINT                        s_height   = 0;

    // Get the path to character.png next to the DLL
    static std::wstring GetImagePath()
    {
        wchar_t modulePath[MAX_PATH]{};
        HMODULE hMod = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)&GetImagePath, &hMod);
        GetModuleFileNameW(hMod, modulePath, MAX_PATH);
        std::wstring path(modulePath);
        size_t pos = path.find_last_of(L"\\/");
        if (pos != std::wstring::npos)
            path = path.substr(0, pos + 1);
        path += L"character.png";
        return path;
    }

    // Decode PNG from file → RGBA pixels via WIC
    static bool DecodeFromFile(const std::wstring& filePath,
                               std::vector<uint8_t>& pixels,
                               UINT& outW, UINT& outH)
    {
        IWICImagingFactory* factory = nullptr;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) return false;

        IWICBitmapDecoder* decoder = nullptr;
        HRESULT hr = factory->CreateDecoderFromFilename(filePath.c_str(),
            nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
        if (FAILED(hr)) { factory->Release(); return false; }

        IWICBitmapFrameDecode* frame = nullptr;
        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr)) { decoder->Release(); factory->Release(); return false; }

        IWICFormatConverter* conv = nullptr;
        hr = factory->CreateFormatConverter(&conv);
        if (FAILED(hr)) { frame->Release(); decoder->Release(); factory->Release(); return false; }

        hr = conv->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeMedianCut);
        if (FAILED(hr)) { conv->Release(); frame->Release(); decoder->Release(); factory->Release(); return false; }

        conv->GetSize(&outW, &outH);
        UINT stride = outW * 4;
        pixels.resize((size_t)stride * outH);
        hr = conv->CopyPixels(nullptr, stride, (UINT)pixels.size(), pixels.data());

        conv->Release(); frame->Release(); decoder->Release(); factory->Release();
        return SUCCEEDED(hr) && outW > 0;
    }

    // Upload RGBA pixels to D3D12 texture and create SRV
    static bool UploadToGPU(ID3D12Device* device,
                            ID3D12DescriptorHeap* srvHeap,
                            ID3D12CommandQueue* cmdQueue,
                            const std::vector<uint8_t>& pixels,
                            UINT width, UINT height)
    {
        D3D12_RESOURCE_DESC texDesc{};
        texDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width            = width;
        texDesc.Height           = height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels        = 1;
        texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        D3D12_HEAP_PROPERTIES defHP{}; defHP.Type = D3D12_HEAP_TYPE_DEFAULT;
        ID3D12Resource* tex = nullptr;
        if (FAILED(device->CreateCommittedResource(
                &defHP, D3D12_HEAP_FLAG_NONE, &texDesc,
                D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&tex)))) return false;

        UINT64 uploadSize = 0;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};
        device->GetCopyableFootprints(&texDesc, 0, 1, 0, &fp, nullptr, nullptr, &uploadSize);

        D3D12_RESOURCE_DESC bufDesc{};
        bufDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width            = uploadSize;
        bufDesc.Height           = 1;
        bufDesc.DepthOrArraySize = 1;
        bufDesc.MipLevels        = 1;
        bufDesc.Format           = DXGI_FORMAT_UNKNOWN;
        bufDesc.SampleDesc.Count = 1;
        bufDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12_HEAP_PROPERTIES upHP{}; upHP.Type = D3D12_HEAP_TYPE_UPLOAD;
        ID3D12Resource* upload = nullptr;
        if (FAILED(device->CreateCommittedResource(
                &upHP, D3D12_HEAP_FLAG_NONE, &bufDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload)))) {
            tex->Release(); return false;
        }

        void* mapped = nullptr;
        if (FAILED(upload->Map(0, nullptr, &mapped))) {
            upload->Release(); tex->Release(); return false;
        }
        uint8_t* dst = (uint8_t*)mapped + fp.Offset;
        for (UINT row = 0; row < height; row++)
            memcpy(dst + (size_t)row * fp.Footprint.RowPitch,
                   pixels.data() + (size_t)row * width * 4,
                   (size_t)width * 4);
        upload->Unmap(0, nullptr);

        ID3D12CommandAllocator*    cmdAlloc = nullptr;
        ID3D12GraphicsCommandList* cmdList  = nullptr;
        if (FAILED(device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc)))) {
            upload->Release(); tex->Release(); return false;
        }
        if (FAILED(device->CreateCommandList(
                0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc,
                nullptr, IID_PPV_ARGS(&cmdList)))) {
            cmdAlloc->Release(); upload->Release(); tex->Release(); return false;
        }

        D3D12_TEXTURE_COPY_LOCATION srcLoc{}, dstLoc{};
        srcLoc.pResource       = upload;
        srcLoc.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint = fp;
        dstLoc.pResource       = tex;
        dstLoc.Type            = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = 0;
        cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = tex;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);

        UINT srvInc = device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels     = 1;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuH = srvHeap->GetCPUDescriptorHandleForHeapStart();
        cpuH.ptr += (SIZE_T)SRV_SLOT * srvInc;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuH = srvHeap->GetGPUDescriptorHandleForHeapStart();
        gpuH.ptr += (UINT64)SRV_SLOT * srvInc;
        device->CreateShaderResourceView(tex, &srvDesc, cpuH);

        cmdList->Close();
        ID3D12CommandList* lists[] = { cmdList };
        cmdQueue->ExecuteCommandLists(1, lists);

        ID3D12Fence* fence = nullptr;
        if (SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
            HANDLE evt = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            cmdQueue->Signal(fence, 1);
            if (fence->GetCompletedValue() < 1) {
                fence->SetEventOnCompletion(1, evt);
                WaitForSingleObject(evt, 5000);
            }
            CloseHandle(evt);
            fence->Release();
        }
        upload->Release();
        cmdList->Release();
        cmdAlloc->Release();

        s_resource  = tex;
        s_gpuHandle = gpuH;
        s_texID     = (ImTextureID)(gpuH.ptr);
        s_width     = width;
        s_height    = height;
        s_loaded    = true;
        return true;
    }

    // Init: call once from hkPresent after ImGui is initialized
    static void Init(ID3D12Device* device,
                     ID3D12DescriptorHeap* srvHeap,
                     ID3D12CommandQueue* cmdQueue)
    {
        if (s_loaded || s_tried || !device || !srvHeap || !cmdQueue) return;
        s_tried = true;

        HRESULT comHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        bool needUninit = (comHr == S_OK || comHr == S_FALSE);

        std::wstring path = GetImagePath();
        std::vector<uint8_t> pixels;
        UINT w = 0, h = 0;
        if (DecodeFromFile(path, pixels, w, h)) {
            UploadToGPU(device, srvHeap, cmdQueue, pixels, w, h);
        }

        if (needUninit) CoUninitialize();
    }

    // Get the ImTextureID (nullptr if not loaded - will fall back to silhouette)
    static ImTextureID GetTexture() { return s_texID; }
    static bool        IsLoaded()   { return s_loaded; }
    static UINT        GetWidth()   { return s_width; }
    static UINT        GetHeight()  { return s_height; }

    // Cleanup
    static void Cleanup()
    {
        if (s_resource) { s_resource->Release(); s_resource = nullptr; }
        s_texID  = nullptr;
        s_loaded = false;
    }
}
