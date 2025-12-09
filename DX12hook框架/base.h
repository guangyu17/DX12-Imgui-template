#pragma once
#include <iostream>
#include <Windows.h>
#include "DX12Hook/Hook/Hook.h"
#include"Config/config.h"
#include<d3d12.h>
#include "DX12Hook/Imgui/imgui.h"
#include "DX12Hook/Imgui/imgui_impl_dx12.h"
#include "DX12Hook/Imgui/imgui_impl_win32.h"
#include"DX12Hook/Draw/draw.h"
#include <dxgi1_4.h>
#pragma comment(lib,"d3d12.lib")


int  STdebug_printf(
    _In_z_ _Printf_format_string_ char const* const _Format,
    ...);