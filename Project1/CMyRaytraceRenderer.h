#pragma once
#include "graphics/GrRenderer.h"
#include "graphics/RayIntersection.h"

class CMyRaytraceRenderer :
	public CGrRenderer
{
public:
    CMyRaytraceRenderer() { m_window = NULL; }
    int     m_rayimagewidth;
    int     m_rayimageheight;
    BYTE** m_rayimage;
    void SetImage(BYTE** image, int w, int h) { m_rayimage = image; m_rayimagewidth = w;  m_rayimageheight = h; }

    CWnd* m_window;

    CRayIntersection m_intersection;

    std::list<CGrTransform> m_mstack;
    CGrMaterial* m_material;

    void SetWindow(CWnd* p_window);
    bool RendererStart();
    bool RendererEnd();
    void RendererMaterial(CGrMaterial* p_material);

    virtual void RendererPushMatrix();
    virtual void RendererPopMatrix();
    virtual void RendererRotate(double a, double x, double y, double z);
    virtual void RendererTranslate(double x, double y, double z);
    virtual void RendererTransform(const CGrTransform* p_transform);
    void RendererEndPolygon();

private:
    CGrPoint TraceRay(const CRay& ray, const CRayIntersection::Object* ignore, int depth);
    bool IsShadowed(const CGrPoint& point, const CGrPoint& normal,
        const CGrPoint& lightpos, const CRayIntersection::Object* self);
    CGrPoint SampleTextureColor(CGrTexture* texture, const CGrPoint& texcoord, const CGrPoint& fallback);
    static double ClampUnit(double v);
};

