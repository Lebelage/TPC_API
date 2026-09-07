module;
#include <string>
export module tpc.core.definitions.client_definitions;
export namespace tpc::core::definitions {

inline constexpr std::string_view CLIENT_ERROR__ = "Client error";
inline constexpr std::string_view CLIENT_WARNING__ = "Client warning";
inline constexpr std::string_view CLIENT_INFO__ = "Client info";
inline constexpr std::string_view CLIENT_SUCCESS__ = "Client success";



}