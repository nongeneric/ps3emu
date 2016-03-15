#pragma once

#define glcall(a) { a; glcheck(__LINE__, #a); }
void glcheck(int line, const char* call);