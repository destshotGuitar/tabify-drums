#include "KitDefinition.h"

namespace kit
{
std::vector<std::string> layerFiles (const ArticulationInfo& info)
{
    const std::string base (info.sample);

    switch (info.naming)
    {
        case LayerNaming::graded:
            return { base + "-Softest", base + "-Soft", base + "-Med", base + "-Hard", base + "-Hardest" };

        case LayerNaming::numbered:
            return { base + "-1", base + "-2", base + "-3", base + "-4", base + "-5" };

        case LayerNaming::single:
            return { base };
    }

    return {};
}

std::string paramId (const char* prefix, int piece)
{
    return std::string (prefix) + "_" + pieces[(size_t) piece].id;
}
} // namespace kit
