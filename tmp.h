/*
 * This file is the header for HelloWorldPluginTmp.cxx
 * Auto-generated header file
 */

#ifndef HELLO_WORLD_PLUGIN_TMP_H
#define HELLO_WORLD_PLUGIN_TMP_H

#include <map>
#include <string>
#include <functional>
#include <memory>
#include <type_traits>
#include <cstdint>


// Function map class for managing type information and functions
class FunctionMap {
public:
    // Functions structure for set, get, and getRaw operations
    struct Functions {
        std::function<void(std::string, void*)> set;
        std::function<std::string(void*)> get;
        std::function<void*()> getRaw;
    };
    
    // Type entry structure to store functions
    struct TypeEntry {
        Functions functions;
    };

    // Register a new type with the function map
    template<typename T>
    static void registerType(const std::string& typeName);

    // Get functions for a specific type
    static Functions& getFunctions(const std::string& typeName);

private:
    // Static map to store type information
    static std::map<std::string, TypeEntry> typeMap;
};

// Function to get data for a specific type
FunctionMap::Functions GetDataTmp(std::string name);

// Function to register all supported data types
void RegisterDataTmp();

#endif // HELLO_WORLD_PLUGIN_TMP_H