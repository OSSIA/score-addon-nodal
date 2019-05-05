#pragma once
#include <Process/ProcessMetadata.hpp>

namespace Nodal
{
class Model;
}

PROCESS_METADATA(
    , Nodal::Model, "f5678806-c431-45c5-ae3a-fae5183380fb",
    "Nodal",                                   // Internal name
    "Nodal",                                   // Pretty name
    Process::ProcessCategory::Other,              // Category
    "Other",                                      // Category
    "Description",                                // Description
    "Author",                                     // Author
    (QStringList{"Put", "Your", "Tags", "Here"}), // Tags
    {},                                           // Inputs
    {},                                           // Outputs
    Process::ProcessFlags::SupportsAll            // Flags
)
