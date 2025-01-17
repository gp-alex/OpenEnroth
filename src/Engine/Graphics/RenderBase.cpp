#include "Engine/Graphics/RenderBase.h"

#include <cassert>

#include "Engine/Engine.h"
#include "Engine/MM7.h"
#include "Engine/SpellFxRenderer.h"

#include "Engine/Objects/Actor.h"
#include "Engine/Objects/SpriteObject.h"

#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Camera.h"
#include "Engine/Graphics/LightmapBuilder.h"
#include "Engine/Graphics/LightsStack.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/BspRenderer.h"
#include "Engine/Graphics/Sprites.h"
#include "Engine/Graphics/Viewport.h"
#include "Engine/Graphics/Vis.h"

#include "Utility/Math/TrigLut.h"



bool RenderBase::Initialize() {
    window->Resize({config->window.Width.Get(), config->window.Height.Get()});

    if (!pD3DBitmaps.Open(MakeDataPath("data", "d3dbitmap.hwl"))) {
        return false;
    }
    if (!pD3DSprites.Open(MakeDataPath("data", "d3dsprite.hwl"))) {
        return false;
    }

    return true;
}

void RenderBase::PostInitialization() {
    // TODO: We aren't using that routine for OpenGL renderer and it should be removed together with DirectDraw.
    __debugbreak();

/*
    if (!config->window.Fullscreen.Get()) {
        // window->SetWindowedMode(game_width, game_height);
        SwitchToWindow();
    } else {
        InitializeFullscreen();
        window->SetFullscreen(true);
    }
*/
}

unsigned int RenderBase::Billboard_ProbablyAddToListAndSortByZOrder(float z) {
    if (uNumBillboardsToDraw >= 999) {
        return 0;
    }

    if (!uNumBillboardsToDraw) {
        uNumBillboardsToDraw = 1;
        return 0;
    }

    unsigned int v7 = 0;
    for (int left = 0, right = uNumBillboardsToDraw;
         left < right;) {  // binsearch
        v7 = left + (right - left) / 2;
        if (z <= render->pBillboardRenderListD3D[v7].z_order)
            right = v7;
        else
            left = v7 + 1;
    }

    if (z > render->pBillboardRenderListD3D[v7].z_order) {
        if (v7 == render->uNumBillboardsToDraw - 1) {
            v7 = render->uNumBillboardsToDraw;
        } else {
            if (render->uNumBillboardsToDraw > v7) {
                for (unsigned int i = 0; i < render->uNumBillboardsToDraw - v7;
                     i++) {
                    memcpy(&render->pBillboardRenderListD3D
                                [render->uNumBillboardsToDraw - i],
                           &render->pBillboardRenderListD3D
                                [render->uNumBillboardsToDraw - (i + 1)],
                           sizeof(render->pBillboardRenderListD3D
                                      [render->uNumBillboardsToDraw - i]));
                }
            }
            ++v7;
        }
        uNumBillboardsToDraw++;
        return v7;
    }

    if (z <= render->pBillboardRenderListD3D[v7].z_order) {
        if (render->uNumBillboardsToDraw > v7) {
            for (unsigned int i = 0; i < render->uNumBillboardsToDraw - v7;
                 i++) {
                memcpy(&render->pBillboardRenderListD3D
                            [render->uNumBillboardsToDraw - i],
                       &render->pBillboardRenderListD3D
                            [render->uNumBillboardsToDraw - (i + 1)],
                       sizeof(render->pBillboardRenderListD3D
                                  [render->uNumBillboardsToDraw - i]));
            }
        }
        uNumBillboardsToDraw++;
        return v7;
    }

    return v7;
}


// TODO: Move this to sprites ?
// combined with IndoorLocation::PrepareItemsRenderList_BLV() (0044028F)
void RenderBase::DrawSpriteObjects() {
    for (unsigned int i = 0; i < pSpriteObjects.size(); ++i) {
        // exit if we are at max sprites
        if (::uNumBillboardsToDraw >= 500) break;

        SpriteObject *object = &pSpriteObjects[i];
        if (!object->uObjectDescID) {  // item probably pciked up - this also gets wiped at end of sprite anims/ particle effects
            continue;
        }
        if (!object->HasSprite()) {
            continue;
        }

        int x = object->vPosition.x;
        int y = object->vPosition.y;
        int z = object->vPosition.z;

        // view culling
        if (uCurrentlyLoadedLevelType == LEVEL_Indoor) {
            bool onlist = false;
            for (uint j = 0; j < pBspRenderer->uNumVisibleNotEmptySectors; j++) {
                if (pBspRenderer->pVisibleSectorIDs_toDrawDecorsActorsEtcFrom[j] == object->uSectorID) {
                    onlist = true;
                    break;
                }
            }
            if (!onlist) continue;
        } else {
            if (!IsCylinderInFrustum(object->vPosition.ToFloat(), 512.0f)) continue;
        }

        // render as sprte 500 - 9081
        if (spell_fx_renderer->RenderAsSprite(object) ||
            ((object->uType < 1000 || object->uType >= 10000) &&
                (object->uType < 500 || object->uType >= 600) &&
                (object->uType < 811 || object->uType >= 815))) {
            SpriteFrame *frame = object->GetSpriteFrame();
            if (frame->icon_name == "null" || frame->texture_name == "null") {
                if (engine->config->debug.VerboseLogging.Get())
                    logger->Warning("Trying to draw sprite with null frame");
                continue;
            }

            // sprite angle to camera
            unsigned int angle = TrigLUT.Atan2(x - pCamera3D->vCameraPos.x, y - pCamera3D->vCameraPos.y);
            int octant = ((TrigLUT.uIntegerPi + (TrigLUT.uIntegerPi >> 3) + object->uFacing - angle) >> 8) & 7;

            pBillboardRenderList[::uNumBillboardsToDraw].hwsprite = frame->hw_sprites[octant];
            // error catching
            if (frame->hw_sprites[octant]->texture->GetHeight() == 0 || frame->hw_sprites[octant]->texture->GetWidth() == 0) {
                if (engine->config->debug.VerboseLogging.Get())
                    logger->Warning("Trying to draw sprite with empty octant texture");
                continue;
            }

            // centre sprite
            if (frame->uFlags & 0x20) {
                z -= (frame->scale * frame->hw_sprites[octant]->uBufferHeight) / 2;
            }

            int16_t setflags = 0;
            if (frame->uFlags & 2) setflags = 2;
            if ((256 << octant) & frame->uFlags) setflags |= 4;
            if (frame->uFlags & 0x40000) setflags |= 0x40;
            if (frame->uFlags & 0x20000) setflags |= 0x80;

            // lighting
            int lightradius = frame->uGlowRadius * object->field_22_glow_radius_multiplier;
            int red = pSpriteObjects[i].GetParticleTrailColorR();
            if (red == 0) red = 0xFF;
            int green = pSpriteObjects[i].GetParticleTrailColorG();
            if (green == 0) green = 0xFF;
            int blue = pSpriteObjects[i].GetParticleTrailColorB();
            if (blue == 0) blue = 0xFF;
            if (lightradius) {
                pMobileLightsStack->AddLight(object->vPosition.ToFloat(),
                                             object->uSectorID, lightradius, red, green, blue, _4E94D3_light_type);
            }

            int view_x = 0;
            int view_y = 0;
            int view_z = 0;
            bool visible = pCamera3D->ViewClip(x, y, z, &view_x, &view_y, &view_z);

            if (visible) {
                if (2 * abs(view_x) >= abs(view_y)) {
                    int projected_x = 0;
                    int projected_y = 0;
                    pCamera3D->Project(view_x, view_y, view_z, &projected_x, &projected_y);

                    float billb_scale = frame->scale * pCamera3D->ViewPlaneDist_X / view_x;

                    int screen_space_half_width = static_cast<int>(billb_scale * frame->hw_sprites[octant]->uBufferWidth / 2.0f);
                    int screen_space_height = static_cast<int>(billb_scale * frame->hw_sprites[octant]->uBufferHeight);

                    if (projected_x + screen_space_half_width >= (signed int)pViewport->uViewportTL_X &&
                        projected_x - screen_space_half_width <= (signed int)pViewport->uViewportBR_X) {
                        if (projected_y >= pViewport->uViewportTL_Y && (projected_y - screen_space_height) <= pViewport->uViewportBR_Y) {
                            object->uAttributes |= SPRITE_VISIBLE;
                            pBillboardRenderList[::uNumBillboardsToDraw].uPaletteIndex = frame->GetPaletteIndex();
                            pBillboardRenderList[::uNumBillboardsToDraw].uIndoorSectorID = object->uSectorID;
                            pBillboardRenderList[::uNumBillboardsToDraw].pSpriteFrame = frame;

                            pBillboardRenderList[::uNumBillboardsToDraw].screenspace_projection_factor_x = billb_scale;
                            pBillboardRenderList[::uNumBillboardsToDraw].screenspace_projection_factor_y = billb_scale;

                            pBillboardRenderList[::uNumBillboardsToDraw].field_1E = setflags;
                            pBillboardRenderList[::uNumBillboardsToDraw].world_x = x;
                            pBillboardRenderList[::uNumBillboardsToDraw].world_y = y;
                            pBillboardRenderList[::uNumBillboardsToDraw].world_z = z;

                            pBillboardRenderList[::uNumBillboardsToDraw].screen_space_x = projected_x;
                            pBillboardRenderList[::uNumBillboardsToDraw].screen_space_y = projected_y;
                            pBillboardRenderList[::uNumBillboardsToDraw].screen_space_z = view_x;

                            pBillboardRenderList[::uNumBillboardsToDraw].object_pid = PID(OBJECT_Item, i);
                            pBillboardRenderList[::uNumBillboardsToDraw].dimming_level = 0;
                            pBillboardRenderList[::uNumBillboardsToDraw].sTintColor = 0;

                            assert(::uNumBillboardsToDraw < 500);
                            ++::uNumBillboardsToDraw;
                            ++uNumSpritesDrawnThisFrame;
                        }
                    }
                }
            }
        }
    }
}

void RenderBase::TransformBillboardsAndSetPalettesODM() {
    SoftwareBillboard billboard = {0};
    billboard.sParentBillboardID = -1;
    //  billboard.pTarget = render->pTargetSurface;
    billboard.pTargetZ = render->pActiveZBuffer;
    //  billboard.uTargetPitch = render->uTargetSurfacePitch;
    billboard.uViewportX = pViewport->uViewportTL_X;
    billboard.uViewportY = pViewport->uViewportTL_Y;
    billboard.uViewportZ = pViewport->uViewportBR_X - 1;
    billboard.uViewportW = pViewport->uViewportBR_Y;
    pODMRenderParams->uNumBillboards = ::uNumBillboardsToDraw;

    for (unsigned int i = 0; i < ::uNumBillboardsToDraw; ++i) {
        RenderBillboard *p = &pBillboardRenderList[i];
        if (p->hwsprite) {
            billboard.screen_space_x = p->screen_space_x;
            billboard.screen_space_y = p->screen_space_y;
            billboard.screen_space_z = p->screen_space_z;
            billboard.sParentBillboardID = i;
            billboard.screenspace_projection_factor_x = p->screenspace_projection_factor_x;
            billboard.screenspace_projection_factor_y = p->screenspace_projection_factor_y;
            billboard.sTintColor = p->sTintColor;
            billboard.object_pid = p->object_pid;
            billboard.uFlags = p->field_1E;

            TransformBillboard(&billboard, p);
        } else {
            if (engine->config->debug.VerboseLogging.Get())
                logger->Warning("Billboard with no sprite!");
        }
    }
}

unsigned int BlendColors(unsigned int a1, unsigned int a2) {
    uint alpha =
        (uint)floorf(0.5f + (a1 >> 24) / 255.0f * (a2 >> 24) / 255.0f * 255.0f);
    uint red = (uint)floorf(0.5f + ((a1 >> 16) & 0xFF) / 255.0f *
                                       ((a2 >> 16) & 0xFF) / 255.0f * 255.0f);
    uint green = (uint)floorf(0.5f + ((a1 >> 8) & 0xFF) / 255.0f *
                                         ((a2 >> 8) & 0xFF) / 255.0f * 255.0f);
    uint blue = (uint)floorf(0.5f + ((a1 >> 0) & 0xFF) / 255.0f *
                                        ((a2 >> 0) & 0xFF) / 255.0f * 255.0f);
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

void RenderBase::TransformBillboard(SoftwareBillboard *pSoftBillboard, RenderBillboard *pBillboard) {
    Sprite *pSprite = pBillboard->hwsprite;
    // error catching
    if (pSprite->texture->GetHeight() == 0 || pSprite->texture->GetWidth() == 0)
        __debugbreak();

    unsigned int billboard_index = Billboard_ProbablyAddToListAndSortByZOrder(pSoftBillboard->screen_space_z);
    RenderBillboardD3D *billboard = &pBillboardRenderListD3D[billboard_index];

    float scr_proj_x = pSoftBillboard->screenspace_projection_factor_x;
    float scr_proj_y = pSoftBillboard->screenspace_projection_factor_y;

    int dimming_level = pBillboard->dimming_level;
    unsigned int diffuse = ::GetActorTintColor(dimming_level, 0, pSoftBillboard->screen_space_z, 0, pBillboard);

    bool opaquetest{ false };
    if (uCurrentlyLoadedLevelType == LEVEL_Outdoor) {
        opaquetest = pSoftBillboard->sTintColor & 0xFF000000;
    } else {
        opaquetest = dimming_level & 0xFF000000;
    }

    if (config->graphics.Tinting.Get() && pSoftBillboard->sTintColor & 0x00FFFFFF) {
        diffuse = BlendColors(pSoftBillboard->sTintColor, diffuse);
        if (opaquetest)
            diffuse = 0x007F7F7F & ((unsigned int)diffuse >> 1);
    }

    if (opaquetest)
        billboard->opacity = RenderBillboardD3D::Opaque_3;
    else
        billboard->opacity = RenderBillboardD3D::Transparent;

    unsigned int specular = 0;
    if (engine->IsSpecular_FogIsOn() && uCurrentlyLoadedLevelType == LEVEL_Outdoor) {
        specular = sub_47C3D7_get_fog_specular(0, 0, pSoftBillboard->screen_space_z);
    }

    float v14 = (float)((int)pSprite->uBufferWidth / 2 - pSprite->uAreaX);
    float v15 = (float)((int)pSprite->uBufferHeight - pSprite->uAreaY);
    if (pSoftBillboard->uFlags & 4) v14 *= -1.f;
    billboard->pQuads[0].diffuse = diffuse;
    billboard->pQuads[0].pos.x = (float)pSoftBillboard->screen_space_x - v14 * scr_proj_x;
    billboard->pQuads[0].pos.y = (float)pSoftBillboard->screen_space_y - v15 * scr_proj_y;
    billboard->pQuads[0].pos.z = 1.f - 1.f / (pSoftBillboard->screen_space_z * 1000.f  / pCamera3D->GetFarClip());
    billboard->pQuads[0].rhw = 1.f / pSoftBillboard->screen_space_z;
    billboard->pQuads[0].specular = specular;
    billboard->pQuads[0].texcoord.x = 0.f;
    billboard->pQuads[0].texcoord.y = 0.f;

    v14 = (float)((int)pSprite->uBufferWidth / 2 - pSprite->uAreaX);
    v15 = (float)((int)pSprite->uBufferHeight - pSprite->uAreaHeight - pSprite->uAreaY);
    if (pSoftBillboard->uFlags & 4) v14 = v14 * -1.f;
    billboard->pQuads[1].specular = specular;
    billboard->pQuads[1].diffuse = diffuse;
    billboard->pQuads[1].pos.x = (float)pSoftBillboard->screen_space_x - v14 * scr_proj_x;
    billboard->pQuads[1].pos.y = (float)pSoftBillboard->screen_space_y - v15 * scr_proj_y;
    billboard->pQuads[1].pos.z = 1.f - 1.f / (pSoftBillboard->screen_space_z * 1000.f / pCamera3D->GetFarClip());
    billboard->pQuads[1].rhw = 1.f / pSoftBillboard->screen_space_z;
    billboard->pQuads[1].texcoord.x = 0.f;
    billboard->pQuads[1].texcoord.y = 1.f;

    v14 = (float)((int)pSprite->uAreaWidth + pSprite->uAreaX + pSprite->uBufferWidth / 2 - pSprite->uBufferWidth);
    v15 = (float)((int)pSprite->uBufferHeight - pSprite->uAreaHeight - pSprite->uAreaY);
    if (pSoftBillboard->uFlags & 4) v14 *= -1.f;
    billboard->pQuads[2].diffuse = diffuse;
    billboard->pQuads[2].specular = specular;
    billboard->pQuads[2].pos.x = (float)pSoftBillboard->screen_space_x + v14 * scr_proj_x;
    billboard->pQuads[2].pos.y = (float)pSoftBillboard->screen_space_y - v15 * scr_proj_y;
    billboard->pQuads[2].pos.z = 1.f - 1.f / (pSoftBillboard->screen_space_z * 1000.f / pCamera3D->GetFarClip());
    billboard->pQuads[2].rhw = 1.f / pSoftBillboard->screen_space_z;
    billboard->pQuads[2].texcoord.x = 1.f;
    billboard->pQuads[2].texcoord.y = 1.f;

    v14 = (float)((int)pSprite->uAreaWidth + pSprite->uAreaX + pSprite->uBufferWidth / 2 - pSprite->uBufferWidth);
    v15 = (float)((int)pSprite->uBufferHeight - pSprite->uAreaY);
    if (pSoftBillboard->uFlags & 4) v14 *= -1.f;
    billboard->pQuads[3].diffuse = diffuse;
    billboard->pQuads[3].specular = specular;
    billboard->pQuads[3].pos.x = (float)pSoftBillboard->screen_space_x + v14 * scr_proj_x;
    billboard->pQuads[3].pos.y = (float)pSoftBillboard->screen_space_y - v15 * scr_proj_y;
    billboard->pQuads[3].pos.z = 1.f - 1.f / (pSoftBillboard->screen_space_z * 1000.f / pCamera3D->GetFarClip());
    billboard->pQuads[3].rhw = 1.f / pSoftBillboard->screen_space_z;
    billboard->pQuads[3].texcoord.x = 1.f;
    billboard->pQuads[3].texcoord.y = 0.f;

    billboard->uNumVertices = 4;

    billboard->texture = pSprite->texture;
    billboard->z_order = pSoftBillboard->screen_space_z;
    billboard->field_90 = pSoftBillboard->field_44;
    billboard->screen_space_z = pSoftBillboard->screen_space_z;
    billboard->object_pid = pSoftBillboard->object_pid;
    billboard->sParentBillboardID = pSoftBillboard->sParentBillboardID;
    billboard->PaletteIndex = pBillboard->uPaletteIndex;
}

double fix2double(int fix) {
    return (((double)(fix & 0xFFFF) / (double)0xFFFF) + (double)(fix >> 16));
}

void RenderBase::MakeParticleBillboardAndPush(SoftwareBillboard *a2,
                                                  Texture *texture,
                                                  unsigned int uDiffuse,
                                                  int angle) {
    unsigned int billboard_index = Billboard_ProbablyAddToListAndSortByZOrder(a2->screen_space_z);
    RenderBillboardD3D *billboard = &pBillboardRenderListD3D[billboard_index];

    billboard->opacity = RenderBillboardD3D::Opaque_1;
    billboard->field_90 = a2->field_44;
    billboard->screen_space_z = a2->screen_space_z;
    billboard->object_pid = a2->object_pid;
    billboard->sParentBillboardID = a2->sParentBillboardID;
    billboard->texture = texture;
    billboard->z_order = a2->screen_space_z;
    billboard->uNumVertices = 4;
    billboard->PaletteIndex = 0;

    float screenspace_projection_factor = a2->screenspace_projection_factor_x;

    float rhw = 1.f / a2->screen_space_z;
    float z = 1.f - 1.f / (a2->screen_space_z * 1000.f / pCamera3D->GetFarClip());

    float acos = (float)cos(angle); // TODO(captainurist): taking cos of an INT angle? WTF?
    float asin = (float)sin(angle);

    {
        float v16 = -12.f;
        float v17 = -12.f;
        billboard->pQuads[0].pos.x = (acos * v16 - asin * v17) * screenspace_projection_factor + (float)a2->screen_space_x;
        billboard->pQuads[0].pos.y = (acos * v17 + asin * v16 - 12.f) * screenspace_projection_factor + (float)a2->screen_space_y;
        billboard->pQuads[0].pos.z = z;
        billboard->pQuads[0].specular = 0;
        billboard->pQuads[0].diffuse = uDiffuse;
        billboard->pQuads[0].rhw = rhw;
        billboard->pQuads[0].texcoord.x = 0.f;
        billboard->pQuads[0].texcoord.y = 0.f;
    }

    {
        float v31 = -12;
        float v32 = 12;
        billboard->pQuads[1].pos.x = (acos * v31 - asin * v32) * screenspace_projection_factor + (float)a2->screen_space_x;
        billboard->pQuads[1].pos.y = (acos * v32 + asin * v31 - 12.f) * screenspace_projection_factor + (float)a2->screen_space_y;
        billboard->pQuads[1].pos.z = z;
        billboard->pQuads[1].specular = 0;
        billboard->pQuads[1].diffuse = uDiffuse;
        billboard->pQuads[1].rhw = rhw;
        billboard->pQuads[1].texcoord.x = 0.0;
        billboard->pQuads[1].texcoord.y = 1.0;
    }

    {
        float v23 = 12;
        float v24 = 12;
        billboard->pQuads[2].pos.x = (acos * v23 - asin * v24) * screenspace_projection_factor + (float)a2->screen_space_x;
        billboard->pQuads[2].pos.y = (acos * v24 + asin * v23 - 12.f) * screenspace_projection_factor + (float)a2->screen_space_y;
        billboard->pQuads[2].pos.z = z;
        billboard->pQuads[2].specular = 0;
        billboard->pQuads[2].diffuse = uDiffuse;
        billboard->pQuads[2].rhw = rhw;
        billboard->pQuads[2].texcoord.x = 1.0;
        billboard->pQuads[2].texcoord.y = 1.0;
    }

    {
        float v39 = 12;
        float v40 = -12;
        billboard->pQuads[3].pos.x = (acos * v39 - asin * v40) * screenspace_projection_factor + (float)a2->screen_space_x;
        billboard->pQuads[3].pos.y = (acos * v40 + asin * v39 - 12.f) * screenspace_projection_factor + (float)a2->screen_space_y;
        billboard->pQuads[3].pos.z = z;
        billboard->pQuads[3].specular = 0;
        billboard->pQuads[3].diffuse = uDiffuse;
        billboard->pQuads[3].rhw = rhw;
        billboard->pQuads[3].texcoord.x = 1.0;
        billboard->pQuads[3].texcoord.y = 0.0;
    }
}

float RenderBase::GetGamma() {
    const float base = 0.60f;
    const float mult = 0.1f;
    int level = engine->config->graphics.Gamma.Get();
    return base + mult * level;
}

HWLTexture *RenderBase::LoadHwlBitmap(const std::string &name) {
    return pD3DBitmaps.LoadTexture(name);
}

HWLTexture *RenderBase::LoadHwlSprite(const std::string &name) {
    return pD3DSprites.LoadTexture(name);
}

