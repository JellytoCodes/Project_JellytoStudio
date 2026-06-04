#pragma once

struct RenderDebugOptions
{
    bool bEnableFrustumCulling = true;
    bool bEnableFaceOcclusionCulling = true;
    bool bEnableSmartRebuild = true;

    static RenderDebugOptions& Get()
    {
        static RenderDebugOptions options;
        return options;
    }
};
