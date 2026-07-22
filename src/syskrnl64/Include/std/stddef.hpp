// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |--------------------------------------------------------------------| //
// | Minimal freestanding LIBSTDC Implementation for the OxizeOS kernel | //
// | STDDEF: extra standard types                                       | //
// |--------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

// -------- //
// OFFSETOF //
// -------- //

#ifndef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif