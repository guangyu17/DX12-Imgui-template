#pragma once


#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx12.h"
#include <d3d11.h>
#pragma comment(lib,"d3d11.lib")
//全局变量

//定义
namespace ImguiDrawFunc
{
	void Setstyle(ImGuiStyle* style__dst = NULL);
	void DrawRect(ImDrawList* Drawlist, ImVec2 start, ImVec2 end, ImColor color, float width);
	void Setfonts(ImGuiIO& io, const char* path, float width);
	void WINAPI rainbow(ImColor& color);
}




