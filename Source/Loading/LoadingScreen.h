#pragma once
#include <d3d12.h>
#include <string>
#include <vector>
#include <mutex>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "../../ThirdParty/ImGui/imgui.h"
#include "../Icons/IconSystem.h"

namespace LoadingScreen {

    // NOTE: The loading-screen logo was previously fetched at runtime from an
    // external URL. That network acquisition has been removed. The screen now
    // always uses the procedural drawn logo (pulsing circle below) and
    // auto-dismisses after TIMEOUT_SEC. If an embedded/local logo image is
    // ever wanted, decode it with HeroIcons::DecodeFromMemory(), fill the
    // s_pending struct (pixels/width/height/ready) under s_mtx, and Tick()
    // will upload it to GPU slot LOGO_SRV_SLOT as before.

    static constexpr UINT LOGO_SRV_SLOT = HeroIcons::MAX_ICON_SLOTS; // slot 64

    static constexpr float POST_LOAD_HOLD = 2.5f;
    static constexpr float TIMEOUT_SEC   = 9.0f;

    static bool  bShowing    = true;
    static bool  bLogoReady  = false;
    static float s_elapsed   = 0.0f;
    static float s_total     = 0.0f;

    static float s_dotTimer = 0.0f;
    static int   s_dotCount = 1;

    static ID3D12Resource*             s_logoRes = nullptr;
    static D3D12_GPU_DESCRIPTOR_HANDLE s_logoGpu = {};
    static ImTextureID                 s_logoTex = nullptr;

    struct PendingLogo {
        std::vector<uint8_t> pixels;
        UINT width  = 0;
        UINT height = 0;
        bool ready  = false;
    };
    static PendingLogo         s_pending;
    static std::mutex          s_mtx;

    static void Init()
    {
        // Nothing to fetch — the drawn fallback logo is used.
    }

    static void Tick(ID3D12Device*         device,
                     ID3D12DescriptorHeap* srvHeap,
                     ID3D12CommandQueue*   cmdQueue)
    {
        if (bLogoReady || !device || !srvHeap || !cmdQueue) return;

        PendingLogo pl;
        {
            std::lock_guard<std::mutex> lk(s_mtx);
            if (!s_pending.ready) return;
            pl = std::move(s_pending);
            s_pending.ready = false;
        }

        D3D12_RESOURCE_DESC texDesc{};
        texDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width            = pl.width;
        texDesc.Height           = pl.height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels        = 1;
        texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        D3D12_HEAP_PROPERTIES defHP{}; defHP.Type = D3D12_HEAP_TYPE_DEFAULT;
        ID3D12Resource* tex = nullptr;
        if (FAILED(device->CreateCommittedResource(
                &defHP, D3D12_HEAP_FLAG_NONE, &texDesc,
                D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&tex)))) return;

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
            tex->Release(); return;
        }

        void* mapped = nullptr;
        if (FAILED(upload->Map(0, nullptr, &mapped))) {
            upload->Release(); tex->Release(); return;
        }
        uint8_t* dst = (uint8_t*)mapped + fp.Offset;
        for (UINT row = 0; row < pl.height; row++)
            memcpy(dst + (size_t)row * fp.Footprint.RowPitch,
                   pl.pixels.data() + (size_t)row * pl.width * 4,
                   (size_t)pl.width * 4);
        upload->Unmap(0, nullptr);

        ID3D12CommandAllocator*    cmdAlloc = nullptr;
        ID3D12GraphicsCommandList* cmdList  = nullptr;
        if (FAILED(device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc)))) {
            upload->Release(); tex->Release(); return;
        }
        if (FAILED(device->CreateCommandList(
                0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc,
                nullptr, IID_PPV_ARGS(&cmdList)))) {
            cmdAlloc->Release(); upload->Release(); tex->Release(); return;
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
        cpuH.ptr += (SIZE_T)LOGO_SRV_SLOT * srvInc;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuH = srvHeap->GetGPUDescriptorHandleForHeapStart();
        gpuH.ptr += (UINT64)LOGO_SRV_SLOT * srvInc;
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

        s_logoRes  = tex;
        s_logoGpu  = gpuH;
        s_logoTex  = (ImTextureID)(gpuH.ptr);
        bLogoReady = true;
    }

    struct Particle {
        float x, y, vx, vy, life, maxLife, size;
    };
    static constexpr int MAX_PARTICLES = 40;
    static Particle s_particles[MAX_PARTICLES];
    static bool     s_particlesInit = false;
    static float    s_fadeAlpha     = 0.0f;

    static void InitParticle(Particle& p, float scrW, float scrH, float cx, float cy) {
        float angle = ((float)(rand() % 3600)) / 10.0f * 3.14159f / 180.0f;
        float dist  = 100.0f + (float)(rand() % 200);
        p.x       = cx + cosf(angle) * dist;
        p.y       = cy + sinf(angle) * dist;
        p.vx      = ((float)(rand() % 100) - 50.0f) * 0.15f;
        p.vy      = -((float)(rand() % 60) + 10.0f) * 0.2f;
        p.maxLife = 2.0f + (float)(rand() % 200) / 100.0f;
        p.life    = p.maxLife;
        p.size    = 1.0f + (float)(rand() % 20) / 10.0f;
    }

    static void Draw()
    {
        if (!bShowing) return;

        ImGuiIO& io = ImGui::GetIO();
        float    dt = io.DeltaTime;
        ImVec2   scr = io.DisplaySize;
        if (scr.x <= 0.0f || scr.y <= 0.0f) return;
        float time = (float)ImGui::GetTime();

        s_total += dt;
        bool dismissing = false;
        if (bLogoReady) {
            s_elapsed += dt;
            if (s_elapsed >= POST_LOAD_HOLD) dismissing = true;
        } else if (s_total >= TIMEOUT_SEC) {
            dismissing = true;
        }

        if (dismissing) {
            s_fadeAlpha -= dt * 2.0f;
            if (s_fadeAlpha <= 0.0f) { s_fadeAlpha = 0.0f; bShowing = false; return; }
        } else {
            if (s_fadeAlpha < 1.0f) s_fadeAlpha += dt * 1.8f;
            if (s_fadeAlpha > 1.0f) s_fadeAlpha = 1.0f;
        }

        float alpha = s_fadeAlpha;
        int   a255  = (int)(alpha * 255.0f);

        s_dotTimer += dt;
        if (s_dotTimer >= 0.35f) {
            s_dotTimer = 0.0f;
            s_dotCount = (s_dotCount % 4) + 1;
        }

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        const float cx = scr.x * 0.5f;
        const float cy = scr.y * 0.45f;

        dl->AddRectFilled({ 0, 0 }, scr, IM_COL32(2, 2, 8, (int)(235 * alpha)));
        dl->AddRectFilled({ 0, 0 }, scr, IM_COL32(6, 6, 18, (int)(30 * alpha)));
        dl->AddRectFilledMultiColor(
            { 0, 0 }, { scr.x, scr.y * 0.35f },
            IM_COL32(0, 0, 0, (int)(180 * alpha)), IM_COL32(0, 0, 0, (int)(180 * alpha)),
            IM_COL32(0, 0, 0, 0),                  IM_COL32(0, 0, 0, 0));
        dl->AddRectFilledMultiColor(
            { 0, scr.y * 0.65f }, scr,
            IM_COL32(0, 0, 0, 0),                  IM_COL32(0, 0, 0, 0),
            IM_COL32(0, 0, 0, (int)(200 * alpha)), IM_COL32(0, 0, 0, (int)(200 * alpha)));

        {
            float pulse = (sinf(time * 1.2f) + 1.0f) * 0.5f;
            int glowBase = (int)((25.0f + pulse * 15.0f) * alpha);
            for (int i = 5; i >= 0; i--) {
                float r = 80.0f + (float)i * 40.0f;
                int   a = (int)(glowBase * (1.0f - (float)i / 6.0f));
                dl->AddCircleFilled({ cx, cy }, r,
                    IM_COL32(50, 180, 130, a), 64);
            }
        }

        if (!s_particlesInit) {
            s_particlesInit = true;
            for (int i = 0; i < MAX_PARTICLES; i++)
                InitParticle(s_particles[i], scr.x, scr.y, cx, cy);
        }
        for (int i = 0; i < MAX_PARTICLES; i++) {
            Particle& p = s_particles[i];
            p.x    += p.vx * dt;
            p.y    += p.vy * dt;
            p.life -= dt;
            if (p.life <= 0.0f)
                InitParticle(p, scr.x, scr.y, cx, cy);

            float t = p.life / p.maxLife;
            float pAlpha = sinf(t * 3.14159f) * alpha;
            int   pa = (int)(pAlpha * 180.0f);
            if (pa > 0) {
                dl->AddCircleFilled({ p.x, p.y }, p.size,
                    IM_COL32(101, 255, 170, pa), 8);
            }
        }

        float ringRadius = 140.0f;
        float rotSpeed   = time * 0.6f;
        int   segments   = 80;
        for (int i = 0; i < segments; i++) {
            float a1 = rotSpeed + (float)i / (float)segments * 6.2832f;
            float a2 = rotSpeed + (float)(i + 1) / (float)segments * 6.2832f;
            float segProgress = (float)i / (float)segments;
            float segAlpha = segProgress * alpha;
            int   sa = (int)(segAlpha * 160.0f);
            if (sa < 1) continue;
            ImVec2 p1 = { cx + cosf(a1) * ringRadius, cy + sinf(a1) * ringRadius };
            ImVec2 p2 = { cx + cosf(a2) * ringRadius, cy + sinf(a2) * ringRadius };
            dl->AddLine(p1, p2, IM_COL32(101, 255, 170, sa), 2.0f);
        }
        float headAngle = rotSpeed + 6.2832f;
        ImVec2 headPos = { cx + cosf(headAngle) * ringRadius, cy + sinf(headAngle) * ringRadius };
        dl->AddCircleFilled(headPos, 4.0f, IM_COL32(101, 255, 170, (int)(220 * alpha)), 12);
        dl->AddCircleFilled(headPos, 8.0f, IM_COL32(101, 255, 170, (int)(50 * alpha)), 12);

        float ringRadius2 = 155.0f;
        float rotSpeed2   = -time * 0.35f;
        int   segments2   = 60;
        for (int i = 0; i < segments2; i++) {
            float a1 = rotSpeed2 + (float)i / (float)segments2 * 6.2832f;
            float a2 = rotSpeed2 + (float)(i + 1) / (float)segments2 * 6.2832f;
            float segAlpha = ((float)i / (float)segments2) * alpha;
            int   sa = (int)(segAlpha * 80.0f);
            if (sa < 1) continue;
            ImVec2 p1 = { cx + cosf(a1) * ringRadius2, cy + sinf(a1) * ringRadius2 };
            ImVec2 p2 = { cx + cosf(a2) * ringRadius2, cy + sinf(a2) * ringRadius2 };
            dl->AddLine(p1, p2, IM_COL32(60, 160, 220, sa), 1.2f);
        }

        const float logoSize = 200.0f;
        ImVec2 logoTL = { cx - logoSize * 0.5f, cy - logoSize * 0.5f };
        ImVec2 logoBR = { cx + logoSize * 0.5f, cy + logoSize * 0.5f };

        if (s_logoTex) {
            dl->AddCircleFilled({ cx, cy }, logoSize * 0.48f,
                IM_COL32(30, 120, 90, (int)(50 * alpha)), 64);
            dl->AddImage(s_logoTex, logoTL, logoBR,
                { 0, 0 }, { 1, 1 }, IM_COL32(255, 255, 255, a255));
        } else {
            float pulse = (sinf(time * 2.5f) + 1.0f) * 0.5f;
            dl->AddCircleFilled({ cx, cy }, logoSize * 0.42f,
                IM_COL32(20, 50, 40, (int)((90 + pulse * 40) * alpha)), 64);
            dl->AddCircle({ cx, cy }, logoSize * 0.42f,
                IM_COL32(101, 255, 170, (int)((100 + pulse * 80) * alpha)), 64, 2.0f);
        }

        ImFont* font = ImGui::GetFont();
        const char* title = "Marvel Abyss";
        float titleSize = 30.0f;
        ImVec2 tsz = font->CalcTextSizeA(titleSize, FLT_MAX, 0.0f, title);
        float tx = cx - tsz.x * 0.5f;
        float ty = logoBR.y + 20.0f;
        dl->AddText(font, titleSize, { tx + 1, ty + 1 },
            IM_COL32(0, 0, 0, (int)(180 * alpha)), title);
        dl->AddText(font, titleSize, { tx, ty },
            IM_COL32(101, 255, 170, a255), title);

        float lineW = tsz.x * 0.6f;
        float lineY = ty + tsz.y + 8.0f;
        dl->AddRectFilled(
            { cx - lineW * 0.5f, lineY },
            { cx + lineW * 0.5f, lineY + 1.5f },
            IM_COL32(101, 255, 170, (int)(100 * alpha)));

        float barW = 260.0f;
        float barH = 3.0f;
        float barY = logoBR.y + 72.0f;
        float barX = cx - barW * 0.5f;

        dl->AddRectFilled(
            { barX, barY }, { barX + barW, barY + barH },
            IM_COL32(30, 30, 40, (int)(160 * alpha)), 1.5f);

        float progress;
        if (bLogoReady) {
            progress = 1.0f;
        } else {
            float sweep = fmodf(time * 0.5f, 1.0f);
            float segStart = sweep;
            float segEnd   = sweep + 0.3f;
            if (segEnd > 1.0f) segEnd = 1.0f;

            dl->AddRectFilled(
                { barX + barW * segStart, barY },
                { barX + barW * segEnd, barY + barH },
                IM_COL32(101, 255, 170, (int)(200 * alpha)), 1.5f);
            progress = -1.0f;
        }

        if (progress >= 0.0f) {
            float fillW = barW * progress;
            dl->AddRectFilled(
                { barX, barY }, { barX + fillW, barY + barH },
                IM_COL32(101, 255, 170, (int)(220 * alpha)), 1.5f);
            if (fillW > 2.0f) {
                dl->AddCircleFilled(
                    { barX + fillW, barY + barH * 0.5f }, 4.0f,
                    IM_COL32(101, 255, 170, (int)(80 * alpha)), 8);
            }
        }

        const char* dotStr[4] = { ".", "..", "...", "...." };
        char statusTxt[64];
        if (bLogoReady)
            snprintf(statusTxt, sizeof(statusTxt),
                "Ready");
        else
            snprintf(statusTxt, sizeof(statusTxt),
                "Loading%s",
                dotStr[s_dotCount - 1]);

        float stSize = 16.0f;
        ImVec2 ssz = font->CalcTextSizeA(stSize, FLT_MAX, 0.0f, statusTxt);
        float sx = cx - ssz.x * 0.5f;
        float sy = logoBR.y + 84.0f;

        dl->AddText(font, stSize, { sx + 1, sy + 1 },
            IM_COL32(0, 0, 0, (int)(120 * alpha)), statusTxt);
        dl->AddText(font, stSize, { sx, sy },
            IM_COL32(180, 180, 190, (int)(alpha * 220)), statusTxt);

        const char* watermark = "Abyss v1.1";
        float wmSize = 13.0f;
        ImVec2 wsz = font->CalcTextSizeA(wmSize, FLT_MAX, 0.0f, watermark);
        float wx = scr.x - wsz.x - 16.0f;
        float wy = scr.y - wsz.y - 12.0f;
        dl->AddText(font, wmSize, { wx, wy },
            IM_COL32(60, 60, 70, (int)(120 * alpha)), watermark);
    }

    static void Cleanup()
    {
        if (s_logoRes) { s_logoRes->Release(); s_logoRes = nullptr; }
        s_logoTex  = nullptr;
        bShowing   = false;
        bLogoReady = false;
    }
}
