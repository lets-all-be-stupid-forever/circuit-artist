#include "fs.h"

extern "C" {
#include "common.h"
#include "utils.h"
};

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

char* checkmodpath2(const char* root, const char* caller, const char* rpath) {
  // TODO
  // There are 2 possible candidates for the file path:
  // 1. Relative to root
  // 2. Relative to the folder of caller file (must still be within root)
  return NULL;
}
