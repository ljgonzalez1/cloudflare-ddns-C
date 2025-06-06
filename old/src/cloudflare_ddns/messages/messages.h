/**
 * @file messages.h
 * @brief Application Messages and User Interface Strings
 * @date 2025-06-01
 * 
 * Centralized message definitions for consistent user interface,
 * internationalization support, and maintainable string management.
 * 
 * Features:
 * - Categorized message types (info, warning, error, success)
 * - Unicode emoji support for modern terminals
 * - Consistent formatting and styling
 * - Easy internationalization preparation
 * - Debug and verbose message levels
 */

#pragma once

// ==============================================================================
// MESSAGE CATEGORIES
// ==============================================================================

/**
 * @brief Application lifecycle messages
 */
#define MSG_INFO_PROGRAM_START          "🚀 Starting Cloudflare DDNS Client..."
#define MSG_INFO_PROGRAM_END            "🏁 Cloudflare DDNS Client finished successfully"
#define MSG_INFO_SHUTDOWN               "🛑 Shutting down gracefully..."
#define MSG_INFO_RESTART                "🔄 Restarting application..."

/**
 * @brief Configuration and environment messages
 */
#define MSG_INFO_CONFIG_LOADING         "🔧 Loading configuration..."
#define MSG_INFO_CONFIG_LOADED          "✅ Configuration loaded successfully"
#define MSG_INFO_CONFIG_VALIDATING      "🔍 Validating configuration..."
#define MSG_INFO_CONFIG_VALID           "✅ Configuration validation passed"

#define MSG_INFO_ENV_LOADING            "🌍 Reading environment variables..."
#define MSG_INFO_ENV_LOADED             "✅ Environment variables loaded"
#define MSG_INFO_ENV_PARSING_DOMAINS    "📋 Parsing domain list..."
#define MSG_INFO_ENV_DOMAINS_PARSED     "✅ Successfully parsed %zu domain(s)"

/**
 * @brief Network and API operation messages
 */
#define MSG_INFO_NETWORK_CONNECTING     "🌐 Connecting to %s..."
#define MSG_INFO_NETWORK_CONNECTED      "✅ Connection established"
#define MSG_INFO_NETWORK_DISCONNECTING  "🔌 Closing connection..."
#define MSG_INFO_NETWORK_DISCONNECTED   "✅ Connection closed"

#define MSG_INFO_API_REQUEST_SENDING    "📤 Sending API request..."
#define MSG_INFO_API_REQUEST_SENT       "✅ API request sent successfully"
#define MSG_INFO_API_RESPONSE_RECEIVING "📥 Receiving API response..."
#define MSG_INFO_API_RESPONSE_RECEIVED  "✅ API response received"

/**
 * @brief DNS operation messages
 */
#define MSG_INFO_DNS_CHECKING_IP        "🌍 Checking current public IP address..."
#define MSG_INFO_DNS_IP_FOUND           "✅ Current IP: %s"
#define MSG_INFO_DNS_UPDATING_RECORD    "🔄 Updating DNS record for %s..."
#define MSG_INFO_DNS_RECORD_UPDATED     "✅ DNS record updated successfully"
#define MSG_INFO_DNS_VERIFYING          "🔍 Verifying DNS propagation..."
#define MSG_INFO_DNS_VERIFIED           "✅ DNS update verified"

// ==============================================================================
// WARNING MESSAGES
// ==============================================================================

/**
 * @brief Configuration warnings
 */
#define MSG_WARN_CONFIG_MISSING         "⚠️  Configuration incomplete"
#define MSG_WARN_CONFIG_DEFAULT         "⚠️  Using default value for %s"
#define MSG_WARN_CONFIG_DEPRECATED      "⚠️  Configuration option '%s' is deprecated"

/**
 * @brief Environment variable warnings
 */
#define MSG_WARN_ENV_VAR_NOT_FOUND      "⚠️  Environment variable '%s' not found"
#define MSG_WARN_ENV_VAR_EMPTY          "⚠️  Environment variable '%s' is empty"
#define MSG_WARN_ENV_VAR_INVALID        "⚠️  Environment variable '%s' has invalid value"
#define MSG_WARN_ENV_VAR_TOO_LONG       "⚠️  Environment variable '%s' exceeds maximum length"

/**
 * @brief Network and API warnings
 */
#define MSG_WARN_NETWORK_SLOW           "⚠️  Network response is slower than expected"
#define MSG_WARN_NETWORK_RETRY          "⚠️  Network error, retrying in %d seconds..."
#define MSG_WARN_API_RATE_LIMIT         "⚠️  API rate limit approaching"
#define MSG_WARN_API_DEPRECATED         "⚠️  Using deprecated API endpoint"

/**
 * @brief DNS operation warnings
 */
#define MSG_WARN_DNS_IP_UNCHANGED       "⚠️  IP address hasn't changed since last update"
#define MSG_WARN_DNS_PROPAGATION_SLOW   "⚠️  DNS propagation taking longer than expected"
#define MSG_WARN_DNS_RECORD_EXISTS      "⚠️  DNS record already exists with same value"

// ==============================================================================
// ERROR MESSAGES
// ==============================================================================

/**
 * @brief Critical application errors
 */
#define MSG_ERR_INITIALIZATION_FAILED   "❌ Application initialization failed"
#define MSG_ERR_MEMORY_ALLOCATION       "❌ Memory allocation failed"
#define MSG_ERR_INVALID_ARGUMENTS       "❌ Invalid command line arguments"
#define MSG_ERR_PERMISSION_DENIED       "❌ Permission denied"

/**
 * @brief Configuration errors
 */
#define MSG_ERR_CONFIG_NOT_FOUND        "❌ Configuration file not found"
#define MSG_ERR_CONFIG_PARSE_ERROR      "❌ Configuration parsing error"
#define MSG_ERR_CONFIG_VALIDATION       "❌ Configuration validation failed"
#define MSG_ERR_CONFIG_REQUIRED_MISSING "❌ Required configuration missing: %s"

/**
 * @brief Environment variable errors
 */
#define MSG_ERR_ENV_VAR_NOT_FOUND       "❌ Required environment variable not found: %s"
#define MSG_ERR_ENV_VAR_INVALID_FORMAT  "❌ Environment variable has invalid format: %s"
#define MSG_ERR_ENV_DOMAIN_PARSE_FAILED "❌ Failed to parse domain list"

/**
 * @brief Network and connectivity errors
 */
#define MSG_ERR_NETWORK_CONNECTION      "❌ Network connection failed"
#define MSG_ERR_NETWORK_TIMEOUT         "❌ Network operation timed out"
#define MSG_ERR_NETWORK_DNS_RESOLUTION  "❌ DNS resolution failed for %s"
#define MSG_ERR_NETWORK_UNREACHABLE     "❌ Network unreachable"

/**
 * @brief API and authentication errors
 */
#define MSG_ERR_API_AUTHENTICATION      "❌ API authentication failed"
#define MSG_ERR_API_AUTHORIZATION       "❌ API authorization failed - insufficient permissions"
#define MSG_ERR_API_INVALID_RESPONSE    "❌ Invalid API response received"
#define MSG_ERR_API_RATE_LIMITED        "❌ API rate limit exceeded"
#define MSG_ERR_API_SERVER_ERROR        "❌ API server error (HTTP %d)"

/**
 * @brief DNS operation errors
 */
#define MSG_ERR_DNS_ZONE_NOT_FOUND      "❌ DNS zone not found: %s"
#define MSG_ERR_DNS_RECORD_NOT_FOUND    "❌ DNS record not found: %s"
#define MSG_ERR_DNS_UPDATE_FAILED       "❌ DNS record update failed"
#define MSG_ERR_DNS_INVALID_IP          "❌ Invalid IP address format: %s"

// ==============================================================================
// SUCCESS MESSAGES
// ==============================================================================

/**
 * @brief Operation success messages
 */
#define MSG_SUCCESS_OPERATION_COMPLETE  "🎉 Operation completed successfully"
#define MSG_SUCCESS_CONFIG_SAVED        "💾 Configuration saved successfully"
#define MSG_SUCCESS_DNS_UPDATED         "🎯 DNS records updated successfully"
#define MSG_SUCCESS_ALL_DOMAINS         "🏆 All domains updated successfully"

/**
 * @brief Milestone messages
 */
#define MSG_SUCCESS_FIRST_RUN           "🌟 First run completed successfully"
#define MSG_SUCCESS_MONITORING_START    "👁️  Monitoring started successfully"
#define MSG_SUCCESS_BACKUP_CREATED      "💾 Backup created successfully"

// ==============================================================================
// DEBUG AND VERBOSE MESSAGES
// ==============================================================================

#ifdef DEBUG
/**
 * @brief Debug-only messages
 */
#define MSG_DEBUG_FUNCTION_ENTRY        "🔍 [DEBUG] Entering function: %s"
#define MSG_DEBUG_FUNCTION_EXIT         "🔍 [DEBUG] Exiting function: %s"
#define MSG_DEBUG_VARIABLE_VALUE        "🔍 [DEBUG] %s = %s"
#define MSG_DEBUG_MEMORY_ALLOCATION     "🔍 [DEBUG] Allocated %zu bytes at %p"
#define MSG_DEBUG_MEMORY_FREE           "🔍 [DEBUG] Freed memory at %p"
#define MSG_DEBUG_API_REQUEST_DETAILS   "🔍 [DEBUG] API Request: %s %s"
#define MSG_DEBUG_API_RESPONSE_DETAILS  "🔍 [DEBUG] API Response: %d bytes, status %d"
#else
#define MSG_DEBUG_FUNCTION_ENTRY        ""
#define MSG_DEBUG_FUNCTION_EXIT         ""
#define MSG_DEBUG_VARIABLE_VALUE        ""
#define MSG_DEBUG_MEMORY_ALLOCATION     ""
#define MSG_DEBUG_MEMORY_FREE           ""
#define MSG_DEBUG_API_REQUEST_DETAILS   ""
#define MSG_DEBUG_API_RESPONSE_DETAILS  ""
#endif

// ==============================================================================
// HELP AND USAGE MESSAGES
// ==============================================================================

/**
 * @brief Help and usage text
 */
#define MSG_HELP_USAGE                  "📋 Usage: %s [options]"
#define MSG_HELP_DESCRIPTION            "🌐 Cloudflare Dynamic DNS Client - Automatically update DNS records"
#define MSG_HELP_EXAMPLES               "💡 Examples:"
#define MSG_HELP_MORE_INFO              "📚 For more information, visit: https://github.com/user/cloudflare-ddns"

/**
 * @brief Environment setup help
 */
#define MSG_HELP_ENV_SETUP              "🔧 Environment Setup Required:"
#define MSG_HELP_ENV_API_KEY            "   export CLOUDFLARE_API_KEY=\"your_api_token\""
#define MSG_HELP_ENV_DOMAINS            "   export DOMAINS=\"example.com,subdomain.example.com\""
#define MSG_HELP_ENV_PROXIED            "   export PROXIED=\"true\"  # Optional, default: false"

/**
 * @brief Troubleshooting messages
 */
#define MSG_HELP_TROUBLESHOOT           "🔧 Troubleshooting:"
#define MSG_HELP_CHECK_NETWORK          "   • Check network connectivity"
#define MSG_HELP_CHECK_API_KEY          "   • Verify API key permissions"
#define MSG_HELP_CHECK_DOMAINS          "   • Ensure domains are in Cloudflare"
#define MSG_HELP_CHECK_LOGS             "   • Review application logs"

// ==============================================================================
// PROGRESS AND STATUS MESSAGES
// ==============================================================================

/**
 * @brief Progress indicators
 */
#define MSG_PROGRESS_STARTING           "⏳ Starting..."
#define MSG_PROGRESS_IN_PROGRESS        "⏳ In progress... (%d%%)"
#define MSG_PROGRESS_COMPLETING         "⏳ Completing..."
#define MSG_PROGRESS_DONE               "✅ Done"

/**
 * @brief Status indicators
 */
#define MSG_STATUS_IDLE                 "😴 Idle"
#define MSG_STATUS_WORKING              "⚡ Working..."
#define MSG_STATUS_WAITING              "⏰ Waiting..."
#define MSG_STATUS_ERROR                "💥 Error"
#define MSG_STATUS_SUCCESS              "🎉 Success"

// ==============================================================================
// FORMATTING HELPERS
// ==============================================================================

/**
 * @brief Message formatting constants
 */
#define MSG_SEPARATOR_MAJOR             "═══════════════════════════════════════════════════════════════════"
#define MSG_SEPARATOR_MINOR             "───────────────────────────────────────────────────────────────────"
#define MSG_INDENT                      "    "
#define MSG_BULLET                      "  • "

/**
 * @brief Color codes for terminal output (if supported)
 */
#ifdef ENABLE_COLOR_OUTPUT
#define COLOR_RED                       "\033[31m"
#define COLOR_GREEN                     "\033[32m"
#define COLOR_YELLOW                    "\033[33m"
#define COLOR_BLUE                      "\033[34m"
#define COLOR_MAGENTA                   "\033[35m"
#define COLOR_CYAN                      "\033[36m"
#define COLOR_WHITE                     "\033[37m"
#define COLOR_RESET                     "\033[0m"
#define COLOR_BOLD                      "\033[1m"
#define COLOR_DIM                       "\033[2m"
#else
#define COLOR_RED                       ""
#define COLOR_GREEN                     ""
#define COLOR_YELLOW                    ""
#define COLOR_BLUE                      ""
#define COLOR_MAGENTA                   ""
#define COLOR_CYAN                      ""
#define COLOR_WHITE                     ""
#define COLOR_RESET                     ""
#define COLOR_BOLD                      ""
#define COLOR_DIM                       ""
#endif

