#pragma once

#include <skypch.h>

#include <yaml-cpp/yaml.h>
#include "renderer/environment.h"

namespace sky
{
class EnvironmentSerializer
{
  public:
    bool serialize(YAML::Emitter &out, const Environment &env);
    Environment deserialize(YAML::Node data);
};
}