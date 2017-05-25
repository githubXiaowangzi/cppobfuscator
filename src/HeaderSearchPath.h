#pragma once

#include "clang/Basic/Version.h"
#include <string>
using std::string;

//#define LLVM_PREFIX "/usr/local"
const string HS_PATHS[] = {
	LLVM_PREFIX "/lib/clang/" CLANG_VERSION_STRING "/include"
};