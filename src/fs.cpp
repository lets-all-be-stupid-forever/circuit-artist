#include "fs.h"

extern "C" {
#include "common.h"
#include "utils.h"
};

#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#if 0
struct LevelDef {
  std::string description;
  std::vector<std::string> kernels;
  std::vector<sprite_t> sprites;
  sprite_t icon_;
};
#endif

namespace fs = std::filesystem;

bool is_subpath(const fs::path& path, const fs::path& base) {
  // Get canonical (absolute, resolved) paths
  fs::path canonical_path = fs::weakly_canonical(path);
  fs::path canonical_base = fs::weakly_canonical(base);

  // Check if path starts with base
  auto path_it = canonical_path.begin();
  auto base_it = canonical_base.begin();

  while (base_it != canonical_base.end()) {
    if (path_it == canonical_path.end() || *path_it != *base_it) {
      return false;
    }
    ++path_it;
    ++base_it;
  }
  return true;
}

char* checkmodpath(const char* root, const char* rpath) {
  fs::path r = fs::path(root);
  fs::path p = r / rpath;
  //   std::cout << "r=" << r << std::endl;
  //   std::cout << "p=" << p << std::endl;
  //   std::cout << "rc=" << fs::canonical(r) << std::endl;
  //   std::cout << "pc=" << fs::canonical(p) << std::endl;
  if (!is_subpath(p, r)) {
    return NULL;
  }
  fs::path can = fs::weakly_canonical(p);
  return clone_string(can.string().c_str());
}

char* os_path_basename(const char* fname) {
  if (!fname) return nullptr;

  try {
    std::filesystem::path p(fname);
    std::string filename = p.filename().string();

    char* out = (char*)std::malloc(filename.size() + 1);
    if (!out) return nullptr;

    std::memcpy(out, filename.c_str(), filename.size() + 1);
    return out;
  } catch (...) {
    return nullptr;
  }
}

char* os_path_join_impl(const char* first, ...) {
  if (!first) return nullptr;

  try {
    std::filesystem::path result(first);

    va_list args;
    va_start(args, first);

    const char* part;
    while ((part = va_arg(args, const char*)) != nullptr) {
      result /= part;
    }

    va_end(args);

    result = result.lexically_normal();

    std::string s = result.string();

    char* out = (char*)std::malloc(s.size() + 1);
    if (!out) return nullptr;

    std::memcpy(out, s.c_str(), s.size() + 1);
    return out;

  } catch (...) {
    return nullptr;
  }
}

char* abs_path(const char* path) {
  if (!path) return nullptr;

  try {
    std::filesystem::path p(path);

    // closest to Python os.path.abspath
    auto abs = std::filesystem::absolute(p).lexically_normal();

    std::string s = abs.string();

    char* out = (char*)std::malloc(s.size() + 1);
    if (!out) return nullptr;

    std::memcpy(out, s.c_str(), s.size() + 1);
    return out;
  } catch (...) {
    return nullptr;
  }
}

bool os_path_exists(const char* path) {
  if (!path) return false;
  return std::filesystem::exists(path);
}

void ensure_folder_exists(const char* path) {
  if (!path) return;
  std::filesystem::create_directories(path);
}
