#pragma once

// Define DLL export that only targets Windows
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif
