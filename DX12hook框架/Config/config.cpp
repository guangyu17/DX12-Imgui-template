#include "config.h"

namespace Config
{
	bool IsDebug = 1;//是否启用调试
	bool IsImgui = 1;//是否启用Imgui

	bool isOpenPrint = 0;
	bool isOpenDebugDraw = 0;
	
	bool isOpenDrawtest = 0;
	bool GetIsDebug()
	{
		return IsDebug;
	};
	bool GetIsImgui()
	{
		return IsImgui;
	};
};