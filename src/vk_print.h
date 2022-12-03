#ifndef VK_PRINT_H
#define VK_PRINT_H
/* vk_print.h
 *
 * Helper functions for printing vulkan information.
 */

#include <vulkan/vulkan.h>
#include <json/json.h>

#include <iostream>
#include <stdio.h>
#include <assert.h>

namespace Json
{
    /*
     * Global std::cout-like json writer.
     */
    class JsonCout
    {
    public:
        std::ostream &operator<<(Json::Value &value)
        {
            assert( m_writer );
            m_writer->write(value, &std::cout);
            return std::cout;
        }
        JsonCout(Json::StreamWriter *writer) :
            m_writer{writer}
        {
        }
        JsonCout()
        {
            // NOTE: Weirdly, if m_writer is set to nullptr in the initializer list, or in
            // the body of the constructor, the pointer is nulled after static initialization
            // of the global instance...
        }
        Json::StreamWriter *m_writer;
    private:
    };
    extern JsonCout cout;
}

const char *vk_enum_to_string_VkPhysicalDeviceType(VkPhysicalDeviceType val);

#endif // VK_PRINT_H
