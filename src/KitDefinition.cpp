#include "KitDefinition.h"

namespace kit
{
std::vector<std::string> layerFiles (const ArticulationInfo& info)
{
    const std::string base (info.sample);

    switch (info.naming)
    {
        case LayerNaming::graded:
            return { base + "-Softest.wav", base + "-Soft.wav", base + "-Med.wav",
                     base + "-Hard.wav", base + "-Hardest.wav" };

        case LayerNaming::numbered:
            return { base + "-1.wav", base + "-2.wav", base + "-3.wav",
                     base + "-4.wav", base + "-5.wav" };

        case LayerNaming::single:
            return { base + ".wav" };
    }

    return {};
}

std::string paramId (const char* prefix, int piece)
{
    return std::string (prefix) + "_" + pieces[(size_t) piece].id;
}
} // namespace kit
