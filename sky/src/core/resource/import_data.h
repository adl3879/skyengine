#pragma once

#include <skypch.h>
#include "core/uuid.h"
#include "core/filesystem.h"
#include "asset_management/asset.h"

namespace sky
{
struct ImportData
{
	UUID		id;
	AssetType	type;
	fs::path	source;
	fs::path	destination;
	std::string version;
};

class ImportDataSerializer
{
  public:
    ImportDataSerializer(ImportData &importData) : m_data(importData) {}

    bool serialize(const std::filesystem::path &path);
    bool deserialize(const std::filesystem::path &path);

  private:
    ImportData &m_data;
};
}