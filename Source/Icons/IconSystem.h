#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wincodec.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

#pragma comment(lib, "windowscodecs.lib")

namespace HeroIcons {

    static constexpr UINT MAX_ICON_SLOTS = 64;

    struct IconTexture {
        ID3D12Resource* resource = nullptr;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
    };

    // Decoded pixel data waiting for GPU upload on the present thread.
    // Fill this from embedded/local image assets only.
    struct PendingIcon {
        std::string      heroName;
        std::vector<uint8_t> pixels;
        UINT             width  = 0;
        UINT             height = 0;
    };

    static bool                         bInitialized  = false;
    static std::unordered_map<std::string, IconTexture> icons;
    static std::vector<ID3D12Resource*> ownedResources;
    static std::mutex                   s_pendingMutex;
    static std::vector<PendingIcon>     s_pending;
    static UINT                         s_srvIndex = 1; // slot 0 = ImGui font atlas
    static ID3D12Device*                s_device   = nullptr;
    static ID3D12DescriptorHeap*        s_srvHeap  = nullptr;
    static ID3D12CommandQueue*          s_cmdQueue = nullptr;

    // NOTE: Hero icons were previously fetched at runtime from an external
    // file repository. That network layer has been removed. Icon textures now only appear if pixel data is supplied from
    // embedded or local-file assets: decode it with DecodeFromMemory() and
    // push a PendingIcon into s_pending; Tick() handles the rest. With no
    // data supplied, GetIcon() simply returns nullptr and every existing
    // call site already falls back to a text/dummy-widget path.

    // ── Decode PNG/WebP/JPEG bytes → RGBA pixels via WIC ─────────────
    static bool DecodeFromMemory(const std::vector<uint8_t>& data,
                                 std::vector<uint8_t>& pixels,
                                 UINT& outW, UINT& outH)
    {
        IWICImagingFactory* factory = nullptr;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) return false;

        IWICStream* stream = nullptr;
        HRESULT hr = factory->CreateStream(&stream);
        if (FAILED(hr)) { factory->Release(); return false; }

        hr = stream->InitializeFromMemory(
            const_cast<BYTE*>(data.data()), (DWORD)data.size());
        if (FAILED(hr)) { stream->Release(); factory->Release(); return false; }

        IWICBitmapDecoder* decoder = nullptr;
        hr = factory->CreateDecoderFromStream(stream, nullptr,
            WICDecodeMetadataCacheOnDemand, &decoder);
        stream->Release();
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

    // ── Init: call once from hkPresent ─────────────────────────────────
    static void Init(ID3D12Device* device,
                     ID3D12DescriptorHeap* srvHeap,
                     ID3D12CommandQueue* cmdQueue)
    {
        if (bInitialized || !device || !srvHeap || !cmdQueue) return;
        s_device   = device;
        s_srvHeap  = srvHeap;
        s_cmdQueue = cmdQueue;
        bInitialized = true;
    }

    // ── Tick: call every frame from hkPresent to upload any ready icons ─
    static void Tick()
    {
        if (!s_device || !s_srvHeap || !s_cmdQueue) return;

        std::vector<PendingIcon> batch;
        {
            std::lock_guard<std::mutex> lk(s_pendingMutex);
            if (s_pending.empty()) return;
            batch = std::move(s_pending);
        }

        ID3D12CommandAllocator* cmdAlloc = nullptr;
        if (FAILED(s_device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc)))) return;

        ID3D12GraphicsCommandList* cmdList = nullptr;
        if (FAILED(s_device->CreateCommandList(
                0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc,
                nullptr, IID_PPV_ARGS(&cmdList)))) {
            cmdAlloc->Release(); return;
        }

        UINT srvInc = s_device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        std::vector<ID3D12Resource*> uploadBuffers;

        for (auto& pi : batch) {
            if (s_srvIndex >= MAX_ICON_SLOTS) break;
            if (icons.count(pi.heroName)) continue;

            D3D12_RESOURCE_DESC texDesc{};
            texDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            texDesc.Width            = pi.width;
            texDesc.Height           = pi.height;
            texDesc.DepthOrArraySize = 1;
            texDesc.MipLevels        = 1;
            texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
            texDesc.SampleDesc.Count = 1;
            texDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            texDesc.Flags            = D3D12_RESOURCE_FLAG_NONE;

            D3D12_HEAP_PROPERTIES defHP{}; defHP.Type = D3D12_HEAP_TYPE_DEFAULT;
            ID3D12Resource* tex = nullptr;
            if (FAILED(s_device->CreateCommittedResource(
                    &defHP, D3D12_HEAP_FLAG_NONE, &texDesc,
                    D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                    IID_PPV_ARGS(&tex)))) continue;

            UINT64 uploadSize = 0;
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};
            s_device->GetCopyableFootprints(&texDesc, 0, 1, 0,
                &fp, nullptr, nullptr, &uploadSize);

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
            if (FAILED(s_device->CreateCommittedResource(
                    &upHP, D3D12_HEAP_FLAG_NONE, &bufDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                    IID_PPV_ARGS(&upload)))) {
                tex->Release(); continue;
            }

            void* mapped = nullptr;
            if (FAILED(upload->Map(0, nullptr, &mapped))) {
                upload->Release(); tex->Release(); continue;
            }
            uint8_t* dst = (uint8_t*)mapped + fp.Offset;
            for (UINT row = 0; row < pi.height; row++) {
                memcpy(dst + (size_t)row * fp.Footprint.RowPitch,
                       pi.pixels.data() + (size_t)row * pi.width * 4,
                       (size_t)pi.width * 4);
            }
            upload->Unmap(0, nullptr);

            D3D12_TEXTURE_COPY_LOCATION srcLoc{};
            srcLoc.pResource = upload; srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            srcLoc.PlacedFootprint = fp;
            D3D12_TEXTURE_COPY_LOCATION dstLoc{};
            dstLoc.pResource = tex; dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLoc.SubresourceIndex = 0;
            cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource   = tex;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmdList->ResourceBarrier(1, &barrier);

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels     = 1;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            D3D12_CPU_DESCRIPTOR_HANDLE cpuH = s_srvHeap->GetCPUDescriptorHandleForHeapStart();
            cpuH.ptr += (SIZE_T)s_srvIndex * srvInc;
            D3D12_GPU_DESCRIPTOR_HANDLE gpuH = s_srvHeap->GetGPUDescriptorHandleForHeapStart();
            gpuH.ptr += (UINT64)s_srvIndex * srvInc;
            s_device->CreateShaderResourceView(tex, &srvDesc, cpuH);

            IconTexture it; it.resource = tex; it.gpuHandle = gpuH;
            icons[pi.heroName] = it;
            ownedResources.push_back(tex);
            uploadBuffers.push_back(upload);
            s_srvIndex++;
        }

        cmdList->Close();
        if (!uploadBuffers.empty()) {
            ID3D12CommandList* lists[] = { cmdList };
            s_cmdQueue->ExecuteCommandLists(1, lists);

            ID3D12Fence* fence = nullptr;
            if (SUCCEEDED(s_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
                HANDLE evt = CreateEventW(nullptr, FALSE, FALSE, nullptr);
                s_cmdQueue->Signal(fence, 1);
                if (fence->GetCompletedValue() < 1) {
                    fence->SetEventOnCompletion(1, evt);
                    WaitForSingleObject(evt, 5000);
                }
                CloseHandle(evt);
                fence->Release();
            }
            for (auto* ub : uploadBuffers) ub->Release();
        }
        cmdList->Release();
        cmdAlloc->Release();
    }

    // ── Query icon for a given hero display name ──────────────────────
    static ImTextureID GetIcon(const std::string& heroName) {
        auto it = icons.find(heroName);
        if (it != icons.end()) return (ImTextureID)(it->second.gpuHandle.ptr);
        return nullptr;
    }

    // ── Draw icon on an ImDrawList (for ESP overlay) ──────────────────
    static void DrawIcon(ImDrawList* dl, const std::string& heroName,
                         ImVec2 pos, float size,
                         bool background = false, ImU32 bgCol = IM_COL32(0,0,0,150))
    {
        ImTextureID tex = GetIcon(heroName);
        if (tex) {
            if (background)
                dl->AddRectFilled(ImVec2(pos.x - 2, pos.y - 2),
                                  ImVec2(pos.x + size + 2, pos.y + size + 2), bgCol, 4.0f);
            dl->AddImage(tex, pos, ImVec2(pos.x + size, pos.y + size));
        }
    }

    // ── Draw icon via ImGui (for menus) ───────────────────────────────
    static bool MenuIcon(const std::string& heroName, float size = 20.0f) {
        ImTextureID tex = GetIcon(heroName);
        if (tex) { ImGui::Image(tex, ImVec2(size, size)); return true; }
        return false;
    }

    // ── Cleanup GPU resources ─────────────────────────────────────────
    static void Cleanup() {
        {
            std::lock_guard<std::mutex> lk(s_pendingMutex);
            s_pending.clear();
        }
        for (auto* r : ownedResources) if (r) r->Release();
        ownedResources.clear();
        icons.clear();
        bInitialized  = false;
        s_srvIndex = 1;
        s_device   = nullptr;
        s_srvHeap  = nullptr;
        s_cmdQueue = nullptr;
    }
}
