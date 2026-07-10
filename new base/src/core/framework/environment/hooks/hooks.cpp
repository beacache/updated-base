#include <hooks/hooks.h>
#include <http/http.h>
#include <miscellaneous/miscellaneous.h>
#include <unordered_set>

lua_CFunction original_index = nullptr;
lua_CFunction original_namecall = nullptr;

static std::unordered_set<std::string> blocked_funcs = {
    "Load", "OpenScreenshotsFolder", "OpenVideosFolder", "AddCoreScriptLocal",
    "SaveScriptProfilingData", "GetRobuxBalance", "PerformPurchase", "PerformPurchaseV2",
    "PerformBulkPurchase", "PerformSubscriptionPurchase", "PerformSubscriptionPurchaseV2",
    "PerformCancelSubscription", "PromptPurchase", "PromptProductPurchase",
    "PromptGamePassPurchase", "PromptBundlePurchase", "PromptBulkPurchase",
    "PromptRobloxPurchase", "PromptThirdPartyPurchase", "PromptPremiumPurchase",
    "PromptSubscriptionPurchase", "PromptCancelSubscription", "PromptNativePurchase",
    "PromptNativePurchaseWithLocalPlayer", "PromptCollectiblesPurchase",
    "PrepareCollectiblesPurchase", "GetUserSubscriptionPaymentHistoryAsync",
    "GetUserSubscriptionDetailsInternalAsync", "GetUserSubscriptionStatusAsync",
    "ReportAbuse", "ReportAbuseV3", "TakeScreenshot", "ToggleRecording",
    "OpenBrowserWindow", "OpenNativeOverlay", "ExecuteJavaScript", "ReturnToJavaScript",
    "SendCommand", "EmitHybridEvent", "OpenWeChatAuthWindow", "BroadcastNotification",
    "OpenUrl", "DetectUrl", "RegisterLuaUrl", "StartLuaUrlDelivery", "StopLuaUrlDelivery",
    "SwitchToSettingsApp", "SupportsSwitchToSettingsApp", "GetLastLuaUrl",
    "GetAndClearLastPendingUrl", "IsUrlRegistered", "RequestInternal", "requestInternal",
    "RequestAsync", "GetAsync", "PostAsync", "GetAsyncFullUrl", "PostAsyncFullUrl",
    "RequestLimitedAsync", "HttpRequestAsync", "GetApiV1", "InvokeAsync",
    "RegisterOpenCloud", "CaptureScreenshot", "CreatePostAsync", "DeleteCapture",
    "DeleteCapturesAsync", "GetCaptureFilePathAsync", "GetCaptureUploadDataAsync",
    "GetCaptureSizeAsync", "GetCaptureStorageSizeAsync", "RetrieveCaptures",
    "SaveCaptureToExternalStorage", "SaveCapturesToExternalStorageAsync",
    "SaveScreenshotCapture", "PromptSaveCapturesToGallery", "PromptShareCapture",
    "PostToFeedAsync", "Call", "GetLast", "GetMessageId",
    "GetProtocolMethodRequestMessageId", "GetProtocolMethodResponseMessageId",
    "MakeRequest", "Publish", "PublishProtocolMethodRequest",
    "PublishProtocolMethodResponse", "Subscribe", "SubscribeToProtocolMethodRequest",
    "SubscribeToProtocolMethodResponse", "GetCredentialsHeaders", "GetDeviceAccessToken",
    "GetDeviceIntegrityToken", "GetDeviceIntegrityTokenYield", "NoPromptCreateOutfit",
    "NoPromptDeleteOutfit", "NoPromptRenameOutfit", "NoPromptSaveAvatar",
    "NoPromptSaveAvatarThumbnailCustomization", "NoPromptSetFavorite",
    "NoPromptUpdateOutfit", "PerformCreateOutfitWithDescription", "PerformDeleteOutfit",
    "PerformRenameOutfit", "PerformSaveAvatarWithDescription", "PerformSetFavorite",
    "PerformUpdateOutfit", "PromptCreateOutfit", "PromptDeleteOutfit",
    "PromptRenameOutfit", "PromptSaveAvatar", "PromptSetFavorite", "PromptUpdateOutfit",
    "SetAllowInventoryReadAccess", "PromptAllowInventoryReadAccess",
    "SignalCreateOutfitFailed", "SignalCreateOutfitPermissionDenied",
    "SignalDeleteOutfitFailed", "SignalDeleteOutfitPermissionDenied",
    "SignalRenameOutfitFailed", "SignalRenameOutfitPermissionDenied",
    "SignalSaveAvatarPermissionDenied", "SignalSetFavoriteFailed",
    "SignalSetFavoritePermissionDenied", "SignalUpdateOutfitFailed",
    "SignalUpdateOutfitPermissionDenied", "GetLocalFileContents", "SetBaseUrl",
    "FetchAssetWithFormat", "RegisterUGCValidationFunction", "PerformManagedUpdate",
    "SetPurchasePromptIsShown", "DumpToFileAsync", "Run", "OpenInBrowser_DONOTUSE",
    "TryInstallPlugin", "PromptImportFile", "PromptImportFiles", "UninstallPlugin",
};

static std::unordered_set<std::string> blocked_services = {
    "BrowserService", "ScriptContext", "ScriptProfilerService", "MicroProfilerService",
    "TestService", "StudioService", "AccountService", "MessageBusService",
    "OpenCloudService", "HttpRbxApiService", "AvatarEditorService", "CaptureService",
    "LinkingService",
};

static std::string sanitize_key(const char* str, size_t len)
{
    std::string key;
    key.reserve(len);
    for (size_t i = 0; i < len; i++)
    {
        if (str[i] != '\0' && str[i] != '\1')
            key += str[i];
    }
    return key;
}

static bool is_executor_state(lua_State* L)
{
    if (!L || !L->userdata)
        return false;

    auto* data = static_cast<RobloxExtraSpace*>(L->userdata);

    if (data->capabilities == module::rbx::globals.max_capabilities)
        return true;

    if (data->source.expired())
        return true;

    return false;
}

int index_hook(lua_State* L)
{
    std::string key;
    if (lua_isstring(L, 2))
    {
        size_t len = 0;
        const char* str = lua_tolstring(L, 2, &len);
        if (str)
            key = sanitize_key(str, len);
    }

    if (key.empty())
        return original_index(L);

    if (blocked_funcs.count(key) > 0)
    {
        luaL_error(L, "attempt to access blocked function '%s'", key.c_str());
        return 0;
    }

    if (is_executor_state(L))
    {
        if (key == "HttpGet" || key == "HttpGetAsync")
        {
            lua_pushcclosure(L, module::core::environment::http.http_get, nullptr, NULL);
            return 1;
        }
        else if (key == "GetObjects" || key == "GetObjectsAsync")
        {
            lua_pushcclosure(L, module::core::environment::miscellaneous.get_objects, nullptr, NULL);
            return 1;
        }
    }

    return original_index(L);
}

int namecall_hook(lua_State* L)
{
    std::string key;
    if (L->namecall)
    {
        const char* str = L->namecall->data;
        size_t len = L->namecall->len;
        if (str)
            key = sanitize_key(str, len);
    }

    if (key.empty())
        return original_namecall(L);

    if (blocked_services.count(key) > 0)
    {
        luaL_error(L, "attempt to access blocked service '%s'", key.c_str());
        return 0;
    }

    if (blocked_funcs.count(key) > 0)
    {
        luaL_error(L, "attempt to access blocked function '%s'", key.c_str());
        return 0;
    }

    if (is_executor_state(L))
    {
        if (key == "HttpGet" || key == "HttpGetAsync")
            return module::core::environment::http.http_get(L);
        else if (key == "GetObjects" || key == "GetObjectsAsync")
            return module::core::environment::miscellaneous.get_objects(L);
    }

    return original_namecall(L);
}

void initialize_hooks(lua_State* L)
{
    int original_top = lua_gettop(L);

    lua_getglobal(L, "game");

    luaL_getmetafield(L, -1, "__index");
    if (lua_type(L, -1) == LUA_TFUNCTION)
    {
        Closure* index_closure = clvalue(luaA_toobject(L, -1));
        if (original_index == nullptr)
            original_index = index_closure->c.f;
        index_closure->c.f = index_hook;
    }
    lua_pop(L, 1);

    luaL_getmetafield(L, -1, "__namecall");
    if (lua_type(L, -1) == LUA_TFUNCTION)
    {
        Closure* namecall_closure = clvalue(luaA_toobject(L, -1));
        if (original_namecall == nullptr)
            original_namecall = namecall_closure->c.f;
        namecall_closure->c.f = namecall_hook;
    }
    lua_pop(L, 1);

    lua_settop(L, original_top);
}
