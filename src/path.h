#pragma once

// Ensure the directory exists (No-op if it does)
int mkdir_safe(const char *path);
// Join two paths together. The result must be freed.
char *pathJoin(const char *path1, const char *path2);
