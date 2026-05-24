#include "render/ui/radial_menu.h"

#include "core/common.h"
#include "render/ui/radial_menu_draw.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <string>

namespace radial_menu_mod::radial_menu {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kInputDirectionDeadzone = 0.01f;
constexpr float kVirtualCursorInputStep = 52.0f;
constexpr float kVirtualCursorSelectionDeadzone = 42.0f;
constexpr float kVirtualCursorMaxRadius = 210.0f;

bool g_is_open = false;
int g_selected_slot = -1;
ImVec2 g_virtual_cursor_offset = {0.0f, 0.0f};
float g_cursor_sensitivity = 1.0f;
IconTextureInfo(*g_icon_texture_resolver)(std::uint32_t icon_id) = nullptr;
std::vector<std::uint32_t> g_cached_icon_ids;
std::vector<IconTextureInfo> g_cached_icon_textures;

void ClearIconCache()
{
    g_cached_icon_ids.clear();
    g_cached_icon_textures.clear();
}

const std::vector<IconTextureInfo>& ResolveIconTextures(const std::vector<RadialSlot>& slots)
{
    if (g_icon_texture_resolver == nullptr) {
        ClearIconCache();
        return g_cached_icon_textures;
    }

    bool cache_matches_slots = g_cached_icon_ids.size() == slots.size() &&
        g_cached_icon_textures.size() == slots.size();
    for (std::size_t i = 0; cache_matches_slots && i < slots.size(); ++i) {
        cache_matches_slots = g_cached_icon_ids[i] == slots[i].icon_id;
    }

    if (!cache_matches_slots) {
        g_cached_icon_ids.clear();
        g_cached_icon_textures.clear();
        g_cached_icon_ids.reserve(slots.size());
        g_cached_icon_textures.reserve(slots.size());
        for (const RadialSlot& slot : slots) {
            g_cached_icon_ids.push_back(slot.icon_id);
            g_cached_icon_textures.push_back(slot.icon_id ? g_icon_texture_resolver(slot.icon_id) : IconTextureInfo{});
        }
        return g_cached_icon_textures;
    }

    for (std::size_t i = 0; i < slots.size(); ++i) {
        if (slots[i].icon_id != 0 && g_cached_icon_textures[i].texture == ImTextureID{}) {
            g_cached_icon_textures[i] = g_icon_texture_resolver(slots[i].icon_id);
        }
    }
    return g_cached_icon_textures;
}

std::wstring GetConfigPath()
{
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&g_is_open),
            &module)) {
        return L"config.ini";
    }

    wchar_t path[MAX_PATH] = {};
    if (GetModuleFileNameW(module, path, MAX_PATH) == 0) return L"config.ini";

    wchar_t* last_slash = std::wcsrchr(path, L'\\');
    if (!last_slash) return L"config.ini";

    *(last_slash + 1) = L'\0';
    return std::wstring(path) + L"config.ini";
}

void LoadConfig()
{
    const std::wstring config_path = GetConfigPath();
    wchar_t value[32] = {};
    GetPrivateProfileStringW(L"Input", L"CursorSensitivity", L"1.0", value,
        static_cast<DWORD>(ARRAYSIZE(value)), config_path.c_str());

    wchar_t* end = nullptr;
    const float parsed = std::wcstof(value, &end);
    if (end != value && std::isfinite(parsed)) {
        g_cursor_sensitivity = std::clamp(parsed, 0.25f, 3.0f);
    } else {
        g_cursor_sensitivity = 1.0f;
    }
}

}  // namespace

void SetIconTextureResolver(IconTextureInfo(*resolver)(std::uint32_t icon_id))
{
    g_icon_texture_resolver = resolver;
}

void Open(int initial_selection)
{
    LoadConfig();
    g_is_open = true;
    g_selected_slot = initial_selection >= 0 ? initial_selection : 0;
    g_virtual_cursor_offset = {0.0f, 0.0f};
    InvalidateDrawCache();
}

void Close()
{
    g_is_open = false;
    g_selected_slot = -1;
    g_virtual_cursor_offset = {0.0f, 0.0f};
    InvalidateDrawCache();
}

void InvalidateDrawCache()
{
    ClearIconCache();
    InvalidateMenuDrawCache();
}

bool IsOpen()
{
    return g_is_open;
}

int GetSelectedSlot()
{
    return g_selected_slot;
}

void UpdateSelectionFromDirection(float selection_x, float selection_y, std::size_t slot_count)
{
    if (!g_is_open || slot_count == 0) return;

    const float magnitude = std::sqrt((selection_x * selection_x) + (selection_y * selection_y));
    if (magnitude < kInputDirectionDeadzone) return;

    const float input_step = kVirtualCursorInputStep * g_cursor_sensitivity;
    g_virtual_cursor_offset.x += selection_x * input_step;
    g_virtual_cursor_offset.y += selection_y * input_step;

    const float cursor_magnitude = std::sqrt(
        (g_virtual_cursor_offset.x * g_virtual_cursor_offset.x) +
        (g_virtual_cursor_offset.y * g_virtual_cursor_offset.y));
    if (cursor_magnitude > kVirtualCursorMaxRadius) {
        const float scale = kVirtualCursorMaxRadius / cursor_magnitude;
        g_virtual_cursor_offset.x *= scale;
        g_virtual_cursor_offset.y *= scale;
    }
    if (cursor_magnitude < kVirtualCursorSelectionDeadzone) return;

    float angle = std::atan2(-g_virtual_cursor_offset.y, g_virtual_cursor_offset.x) + (kPi * 0.5f);
    if (angle < 0.0f) angle += 2.0f * kPi;

    const float segment_size = (2.0f * kPi) / static_cast<float>(slot_count);
    g_selected_slot = static_cast<int>(std::floor(angle / segment_size)) % static_cast<int>(slot_count);
}

void Draw(const std::vector<RadialSlot>& slots, const char* title, const char* controls)
{
    if (!g_is_open) return;
    DrawMenuContents(slots, title, controls, g_selected_slot, g_virtual_cursor_offset,
        ResolveIconTextures(slots));
}

}  // namespace radial_menu_mod::radial_menu
