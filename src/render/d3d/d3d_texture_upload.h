#pragma once

#include <d3d12.h>

#include <cstdint>
#include <vector>

namespace radial_menu_mod::d3d_texture_upload {

struct UploadedTexture {
    ID3D12Resource* resource = nullptr;
    float width = 1.0f;
    float height = 1.0f;
};

struct PendingUpload {
    ID3D12Resource* upload_buffer = nullptr;
    ID3D12Fence* fence = nullptr;
    ID3D12CommandAllocator* allocator = nullptr;
    ID3D12GraphicsCommandList* cmd_list = nullptr;
    UINT64 fence_value = 0;
    UploadedTexture texture;
};

bool StartBc7TextureUpload(
    ID3D12Device* device,
    ID3D12CommandQueue* queue,
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_srv,
    const std::vector<std::uint8_t>& dds,
    PendingUpload& pending);

bool CompletePendingUpload(PendingUpload& pending, UploadedTexture& texture);
void ReleasePendingUpload(PendingUpload& pending);

// Keep old function for compatibility during transition
bool UploadBc7Texture(
    ID3D12Device* device,
    ID3D12CommandQueue* queue,
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_srv,
    const std::vector<std::uint8_t>& dds,
    UploadedTexture& texture);

}  // namespace radial_menu_mod::d3d_texture_upload