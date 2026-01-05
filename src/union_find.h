#ifndef CA_UNION_FIND_H
#define CA_UNION_FIND_H

// Union-find Find
static inline int uf_find(int* c, int a) {
  int root = a;
  while (c[root] != root) {
    root = c[root];
  }
  while (c[a] != root) {
    int par = c[a];
    c[a] = root;
    a = par;
  }
  return root;
}

// Union-find Union
static inline void uf_union(int* c, int* r, int a, int b) {
  int ca = uf_find(c, a);
  int cb = uf_find(c, b);
  if (ca == cb) {
    return;
  }
  if (r[ca] < r[cb]) {
    c[ca] = cb;
  } else {
    c[cb] = ca;
    if (r[ca] == r[cb]) {
      r[ca] += 1;
    }
  }
}

#endif
