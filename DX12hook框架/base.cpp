#include"base.h"
int  STdebug_printf(
    _In_z_ _Printf_format_string_ char const* const _Format,
    ...) {
#if _DEBUG
    return printf(_Format);
#endif // _DEBUG
    return 0;
};