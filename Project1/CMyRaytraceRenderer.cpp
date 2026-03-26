#include "pch.h"
#include "CMyRaytraceRenderer.h"
#include "graphics/GrTexture.h"
#include <cmath>

void CMyRaytraceRenderer::SetWindow(CWnd* p_window)
{
    m_window = p_window;
}

bool CMyRaytraceRenderer::RendererStart()
{
	m_intersection.Initialize();

	m_mstack.clear();


	// We have to do all of the matrix work ourselves.
	// Set up the matrix stack.
	CGrTransform t;
	t.SetLookAt(Eye().X(), Eye().Y(), Eye().Z(),
		Center().X(), Center().Y(), Center().Z(),
		Up().X(), Up().Y(), Up().Z());

	m_mstack.push_back(t);

	m_material = NULL;

	return true;
}

void CMyRaytraceRenderer::RendererMaterial(CGrMaterial* p_material)
{
	m_material = p_material;
}

void CMyRaytraceRenderer::RendererPushMatrix()
{
	m_mstack.push_back(m_mstack.back());
}

void CMyRaytraceRenderer::RendererPopMatrix()
{
	m_mstack.pop_back();
}

void CMyRaytraceRenderer::RendererRotate(double a, double x, double y, double z)
{
	CGrTransform r;
	r.SetRotate(a, CGrPoint(x, y, z));
	m_mstack.back() *= r;
}

void CMyRaytraceRenderer::RendererTranslate(double x, double y, double z)
{
	CGrTransform r;
	r.SetTranslate(x, y, z);
	m_mstack.back() *= r;
}

void CMyRaytraceRenderer::RendererTransform(const CGrTransform* p_transform)
{
	m_mstack.back() *= *p_transform;
}

//
// Name : CMyRaytraceRenderer::RendererEndPolygon()
// Description : End definition of a polygon. The superclass has
// already collected the polygon information
//

void CMyRaytraceRenderer::RendererEndPolygon()
{
    const std::list<CGrPoint>& vertices = PolyVertices();
    const std::list<CGrPoint>& normals = PolyNormals();
    const std::list<CGrPoint>& tvertices = PolyTexVertices();

    // Allocate a new polygon in the ray intersection system
    m_intersection.PolygonBegin();
    m_intersection.Material(m_material);

    if (PolyTexture())
    {
        m_intersection.Texture(PolyTexture());
    }

    std::list<CGrPoint>::const_iterator normal = normals.begin();
    std::list<CGrPoint>::const_iterator tvertex = tvertices.begin();

    for (std::list<CGrPoint>::const_iterator i = vertices.begin(); i != vertices.end(); i++)
    {
        if (normal != normals.end())
        {
            m_intersection.Normal(m_mstack.back() * *normal);
            normal++;
        }

        if (tvertex != tvertices.end())
        {
            m_intersection.TexVertex(*tvertex);
            tvertex++;
        }

        m_intersection.Vertex(m_mstack.back() * *i);
    }

    m_intersection.PolygonEnd();
}

bool CMyRaytraceRenderer::RendererEnd()
{
	m_intersection.LoadingComplete();

	double ymin = -tan(ProjectionAngle() / 2 * GR_DTOR);
	double yhit = -ymin * 2;

	double xmin = ymin * ProjectionAspect();
	double xwid = -xmin * 2;

	const double jx[2] = { 0.25, 0.75 };
	const double jy[2] = { 0.25, 0.75 };

	for (int r = 0; r < m_rayimageheight; r++)
	{
		for (int c = 0; c < m_rayimagewidth; c++)
		{
			CGrPoint color(0, 0, 0, 1);
			// 2x2 subsamples for AA
			for (int sy = 0; sy < 2; sy++)
			{
				for (int sx = 0; sx < 2; sx++)
				{
					double x = xmin + (c + jx[sx]) / m_rayimagewidth * xwid;
					double y = ymin + (r + jy[sy]) / m_rayimageheight * yhit;
					CRay ray(CGrPoint(0, 0, 0), Normalize3(CGrPoint(x, y, -1, 0)));
					color += TraceRay(ray, NULL, 0);
				}
			}
			color = color / 4.0;

			m_rayimage[r][c * 3] = BYTE(ClampUnit(color.X()) * 255.0);
			m_rayimage[r][c * 3 + 1] = BYTE(ClampUnit(color.Y()) * 255.0);
			m_rayimage[r][c * 3 + 2] = BYTE(ClampUnit(color.Z()) * 255.0);
		}
		if ((r % 50) == 0)
		{
			m_window->Invalidate();
			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				DispatchMessage(&msg);
		}
	}


	return true;
}

double CMyRaytraceRenderer::ClampUnit(double v)
{
	if (v < 0.0) return 0.0;
	if (v > 1.0) return 1.0;
	return v;
}

CGrPoint CMyRaytraceRenderer::SampleTextureColor(CGrTexture* texture, const CGrPoint& texcoord, const CGrPoint& fallback)
{
	if (texture == NULL || texture->Width() <= 0 || texture->Height() <= 0)
		return fallback;

	double s = texcoord.X() - floor(texcoord.X());
	double t = texcoord.Y() - floor(texcoord.Y());
	if (s < 0.0) s += 1.0;
	if (t < 0.0) t += 1.0;

	int x = int(s * double(texture->Width() - 1));
	int y = int(t * double(texture->Height() - 1));
	const BYTE* row = texture->Row(y);
	const BYTE* pixel = row + x * 3;

	return CGrPoint(pixel[0] / 255.0, pixel[1] / 255.0, pixel[2] / 255.0, 1);
}

bool CMyRaytraceRenderer::IsShadowed(const CGrPoint& point, const CGrPoint& normal,
	const CGrPoint& lightpos, const CRayIntersection::Object* self)
{
	CGrPoint tolight = lightpos - point;
	double maxdist = tolight.Length3();
	if (maxdist <= 1e-6) return false;

	CGrPoint ldir = tolight / maxdist;
	CGrPoint shadowstart = point + normal * 0.001;
	CRay shadowray(shadowstart, ldir);

	const CRayIntersection::Object* blocker;
	double tshadow;
	CGrPoint hit;
	return m_intersection.Intersect(shadowray, maxdist - 0.002, self, blocker, tshadow, hit);
}

CGrPoint CMyRaytraceRenderer::TraceRay(const CRay& ray, const CRayIntersection::Object* ignore, int depth)
{
	const int maxdepth = 4;
	const CGrPoint background(0.02, 0.02, 0.05, 1);

	double t;
	CGrPoint intersect;
	const CRayIntersection::Object* nearest;
	if (!m_intersection.Intersect(ray, 1e20, ignore, nearest, t, intersect))
		return background;

	CGrPoint N;
	CGrMaterial* material = NULL;
	CGrTexture* texture = NULL;
	CGrPoint texcoord;
	m_intersection.IntersectInfo(ray, nearest, t, N, material, texture, texcoord);

	if (material == NULL)
		return CGrPoint(0, 0, 0, 1);

	N = Normalize3(N);
	CGrPoint V = Normalize3(-ray.Direction());
	CGrPoint base(material->Diffuse(0), material->Diffuse(1), material->Diffuse(2), 1);
	base = SampleTextureColor(texture, texcoord, base);

	CGrPoint color(0, 0, 0, 1);

	int nlights = LightCnt();
	for (int k = 0; k < nlights; k++)
	{
		const Light& Lg = GetLight(k);

		color.X() += base.X() * material->Ambient(0) * Lg.m_ambient[0];
		color.Y() += base.Y() * material->Ambient(1) * Lg.m_ambient[1];
		color.Z() += base.Z() * material->Ambient(2) * Lg.m_ambient[2];

		if (IsShadowed(intersect, N, Lg.m_pos, nearest))
			continue;

		CGrPoint L = Normalize3(Lg.m_pos - intersect);
		double ndotl = Dot3(N, L);
		if (ndotl > 0.0)
		{
			color.X() += base.X() * material->Diffuse(0) * Lg.m_diffuse[0] * ndotl;
			color.Y() += base.Y() * material->Diffuse(1) * Lg.m_diffuse[1] * ndotl;
			color.Z() += base.Z() * material->Diffuse(2) * Lg.m_diffuse[2] * ndotl;
		}

		CGrPoint H = Normalize3(L + V);
		double ndoth = Dot3(N, H);
		if (ndoth > 0.0)
		{
			double sh = material->Shininess();
			if (sh < 1.0) sh = 1.0;
			double spec = pow(ndoth, sh);
			color.X() += material->Specular(0) * Lg.m_specular[0] * spec;
			color.Y() += material->Specular(1) * Lg.m_specular[1] * spec;
			color.Z() += material->Specular(2) * Lg.m_specular[2] * spec;
		}
	}

	double reflect = material->SpecularOther(0);
	if (material->SpecularOther(1) > reflect) reflect = material->SpecularOther(1);
	if (material->SpecularOther(2) > reflect) reflect = material->SpecularOther(2);
	reflect = ClampUnit(reflect);

	if (reflect > 0.001 && depth < maxdepth)
	{
		double dn = Dot3(ray.Direction(), N);
		CGrPoint rdir = Normalize3(ray.Direction() - N * (2.0 * dn));
		CGrPoint r0 = intersect + N * 0.001;
		CGrPoint reflcol = TraceRay(CRay(r0, rdir), nearest, depth + 1);
		color = color * (1.0 - reflect) + reflcol * reflect;
	}

	const CGrPoint fogcol(0.18, 0.20, 0.25, 1);
	double fogmix = ClampUnit((t - 14.0) / 34.0);
	color = color * (1.0 - fogmix) + fogcol * fogmix;

	color.X() = ClampUnit(color.X());
	color.Y() = ClampUnit(color.Y());
	color.Z() = ClampUnit(color.Z());
	return color;
}
