/*
 *
 */
#include "vk_print.h"

namespace Json
{
    JsonCout cout;
}

__attribute__((constructor))
static void initialize_JsonCout()
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    Json::StreamWriter *writer = builder.newStreamWriter();
    assert( writer );
    Json::cout = Json::JsonCout(writer);
}

#define CASE(enum_val)\
    case enum_val:\
        return #enum_val
#define UNKNOWN()\
    return "UNKNOWN"

const char *vk_enum_to_string_VkPhysicalDeviceType(VkPhysicalDeviceType val)
{
    switch (val)
    {
        CASE(VK_PHYSICAL_DEVICE_TYPE_OTHER);
        CASE(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
        CASE(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
        CASE(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU);
        CASE(VK_PHYSICAL_DEVICE_TYPE_CPU);
        CASE(VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM);
    }
    UNKNOWN();
}


