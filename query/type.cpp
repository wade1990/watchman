/* Copyright 2013-present Facebook, Inc.
 * Licensed under the Apache License, Version 2.0 */

#include "watchman.h"

#include <memory>

using watchman::DType;

class TypeExpr : public QueryExpr {
  char arg;

 public:
  explicit TypeExpr(char arg) : arg(arg) {}

  EvaluateResult evaluate(struct w_query_ctx*, FileResult* file) override {
    auto optionalDtype = file->dtype();
    if (!optionalDtype.has_value()) {
      return folly::none;
    }
    auto dtype = *optionalDtype;
    if (dtype != DType::Unknown) {
      switch (arg) {
        case 'b':
          return dtype == DType::Block;
        case 'c':
          return dtype == DType::Char;
        case 'p':
          return dtype == DType::Fifo;
        case 's':
          return dtype == DType::Socket;
        case 'd':
          return dtype == DType::Dir;
        case 'f':
          return dtype == DType::Regular;
        case 'l':
          return dtype == DType::Symlink;
      }
    }

    auto stat = file->stat();
    if (!stat.has_value()) {
      return folly::none;
    }

    switch (arg) {
#ifndef _WIN32
      case 'b':
        return S_ISBLK(stat->mode);
      case 'c':
        return S_ISCHR(stat->mode);
      case 'p':
        return S_ISFIFO(stat->mode);
      case 's':
        return S_ISSOCK(stat->mode);
#endif
      case 'd':
        return stat->isDir();
      case 'f':
        return stat->isFile();
      case 'l':
        return stat->isSymlink();
#ifdef S_ISDOOR
      case 'D':
        return S_ISDOOR(stat->mode);
#endif
      default:
        return false;
    }
  }

  static std::unique_ptr<QueryExpr> parse(w_query*, const json_ref& term) {
    const char *ignore, *typestr, *found;
    char arg;

    if (json_unpack(term, "[s,u]", &ignore, &typestr) != 0) {
      throw QueryParseError("must use [\"type\", \"typestr\"]");
    }

    found = strpbrk(typestr, "bcdfplsD");
    if (!found || strlen(typestr) > 1) {
      throw QueryParseError("invalid type string '", typestr, "'");
    }

    arg = *found;

    return std::make_unique<TypeExpr>(arg);
  }
};
W_TERM_PARSER("type", TypeExpr::parse)

/* vim:ts=2:sw=2:et:
 */
