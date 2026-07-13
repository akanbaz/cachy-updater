#pragma once

#include "Pkg.h"

// Severity classification and change-summary building.
namespace cachy::classifier {

bool isKernel(const QString &name, Source source);
Severity classify(const Pkg &pkg);
QString buildSummary(const Pkg &pkg);

// "major" | "minor" | "patch" | "unknown"
QString versionBump(const QString &oldVer, const QString &newVer);

} // namespace cachy::classifier
