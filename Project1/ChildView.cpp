
// ChildView.cpp : implementation of the CChildView class
//

#include "pch.h"
#include "framework.h"
#include "Project1.h"
#include "ChildView.h"
#include "graphics/OpenGLRenderer.h"
#include "graphics/GrTexture.h"
#include "graphics/GrTransform.h"
#include "CMyRaytraceRenderer.h"
#include <cmath>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

CChildView::CChildView()
{
	m_camera.Set(16., 7., 30., 0., -3., 0., 0., 1., 0.);

	m_raytrace = false;
	m_rayimage = NULL;

	CGrPtr<CGrComposite> scene = new CGrComposite;
	m_scene = scene;

	CGrPtr<CGrTexture> checker = new CGrTexture;
	checker->SetSize(256, 256);
	for (int y = 0; y < checker->Height(); y++)
	{
		for (int x = 0; x < checker->Width(); x++)
		{
			int tx = x / 32;
			int ty = y / 32;
			if (((tx + ty) & 1) == 0)
				checker->Set(x, y, 60, 60, 65);
			else
				checker->Set(x, y, 190, 190, 195);
		}
	}

	CGrPtr<CGrMaterial> floorpaint = new CGrMaterial;
	floorpaint->AmbientAndDiffuse(1.0f, 1.0f, 1.0f);
	floorpaint->Specular(0.15f, 0.15f, 0.15f);
	floorpaint->Shininess(20.0f);
	scene->Child(floorpaint);

	CGrPtr<CGrComposite> floorgeo = new CGrComposite;
	floorpaint->Child(floorgeo);

	CGrPtr<CGrPolygon> floorpoly = new CGrPolygon;
	floorpoly->Texture(checker);
	// verts ordered CCW from +Y so GL keeps the top face
	floorpoly->AddNormal3d(0, 1, 0);
	floorpoly->AddTexVertex3d(-25, -8, -25, 0, 0);
	floorpoly->AddTexVertex3d(-25, -8, 25, 0, 5);
	floorpoly->AddTexVertex3d(25, -8, 25, 5, 5);
	floorpoly->AddTexVertex3d(25, -8, -25, 5, 0);
	floorgeo->Child(floorpoly);

	CGrPtr<CGrMaterial> mirrorpaint = new CGrMaterial;
	mirrorpaint->AmbientAndDiffuse(0.10f, 0.10f, 0.12f);
	mirrorpaint->Specular(1.0f, 1.0f, 1.0f);
	mirrorpaint->SpecularOther(0.80f, 0.80f, 0.80f);
	mirrorpaint->Shininess(80.0f);
	scene->Child(mirrorpaint);

	CGrPtr<CGrComposite> mirrorbox = new CGrComposite;
	mirrorpaint->Child(mirrorbox);
	mirrorbox->Box(-15, -8, -6, 7, 8, 7);

	CGrPtr<CGrMaterial> brasspaint = new CGrMaterial;
	brasspaint->AmbientAndDiffuse(0.76f, 0.66f, 0.22f);
	brasspaint->Specular(0.8f, 0.7f, 0.35f);
	brasspaint->Shininess(48.0f);
	scene->Child(brasspaint);

	CGrPtr<CGrComposite> tetra = new CGrComposite;
	brasspaint->Child(tetra);
	CGrPoint p0(0, 6, 0);
	CGrPoint p1(-5, -3, 5);
	CGrPoint p2(5, -3, 5);
	CGrPoint p3(0, -3, -6);
	tetra->Poly3(p0, p1, p2);
	tetra->Poly3(p0, p2, p3);
	tetra->Poly3(p0, p3, p1);
	tetra->Poly3(p1, p3, p2);

	// square base pyramid (not the same thing as the tetrahedron)
	CGrPtr<CGrMaterial> pyrpaint = new CGrMaterial;
	pyrpaint->AmbientAndDiffuse(0.22f, 0.52f, 0.72f, 1.0f);
	pyrpaint->Specular(0.35f, 0.45f, 0.55f, 1.0f);
	pyrpaint->Shininess(28.0f);
	scene->Child(pyrpaint);
	CGrPtr<CGrComposite> pyr = new CGrComposite;
	pyrpaint->Child(pyr);
	CGrPoint pa(-12.0, 5.0, -12.0);
	CGrPoint pb0(-16.0, -8.0, -16.0);
	CGrPoint pb1(-8.0, -8.0, -16.0);
	CGrPoint pb2(-8.0, -8.0, -8.0);
	CGrPoint pb3(-16.0, -8.0, -8.0);
	pyr->Poly4(pb0, pb1, pb2, pb3);
	pyr->Poly3(pa, pb0, pb1);
	pyr->Poly3(pa, pb1, pb2);
	pyr->Poly3(pa, pb2, pb3);
	pyr->Poly3(pa, pb3, pb0);

	// 8 sided cylinder-ish solid so it is not another box
	CGrPtr<CGrMaterial> cylpaint = new CGrMaterial;
	cylpaint->AmbientAndDiffuse(0.32f, 0.62f, 0.28f, 1.0f);
	cylpaint->Specular(0.4f, 0.55f, 0.35f, 1.0f);
	cylpaint->Shininess(26.0f);
	scene->Child(cylpaint);
	CGrPtr<CGrComposite> cylgrp = new CGrComposite;
	cylpaint->Child(cylgrp);
	const double cx = 14.0;
	const double cz = 10.0;
	const double rad = 2.2;
	const double y0 = -8.0;
	const double y1 = 2.0;
	CGrPoint ringb[8];
	CGrPoint ringt[8];
	for (int i = 0; i < 8; i++)
	{
		double a = i * (GR_PI / 4.0);
		double dx = cos(a) * rad;
		double dz = sin(a) * rad;
		ringb[i] = CGrPoint(cx + dx, y0, cz + dz);
		ringt[i] = CGrPoint(cx + dx, y1, cz + dz);
	}
	for (int i = 0; i < 8; i++)
	{
		int j = (i + 1) % 8;
		cylgrp->Poly3(ringb[i], ringb[j], ringt[j]);
		cylgrp->Poly3(ringb[i], ringt[j], ringt[i]);
	}
	CGrPoint midb(cx, y0, cz);
	CGrPoint midt(cx, y1, cz);
	for (int i = 0; i < 8; i++)
	{
		int j = (i + 1) % 8;
		cylgrp->Poly3(midb, ringb[j], ringb[i]);
		cylgrp->Poly3(midt, ringt[i], ringt[j]);
	}

	CGrPtr<CGrMaterial> redpaint = new CGrMaterial;
	redpaint->AmbientAndDiffuse(0.80f, 0.14f, 0.12f);
	redpaint->Specular(0.45f, 0.35f, 0.35f);
	redpaint->Shininess(30.0f);
	scene->Child(redpaint);

	CGrPtr<CGrComposite> redbox = new CGrComposite;
	redpaint->Child(redbox);
	redbox->Box(5, -8, -2, 6, 6, 6);
}

CChildView::~CChildView()
{
	if (m_rayimage) {
		delete[] m_rayimage[0];
		delete[] m_rayimage;
	}
}


BEGIN_MESSAGE_MAP(CChildView, COpenGLWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_RENDER_RAYTRACE, &CChildView::OnRenderRaytrace)
	ON_UPDATE_COMMAND_UI(ID_RENDER_RAYTRACE, &CChildView::OnUpdateRenderRaytrace)
END_MESSAGE_MAP()



// CChildView message handlers

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!COpenGLWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), nullptr);

	return TRUE;
}

void CChildView::OnGLDraw(CDC* pDC)
{
	if (m_raytrace)
	{
		// Clear the color buffer
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Set up for parallel projection
		int width, height;
		GetSize(width, height);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width, 0, height, -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// If we got it, draw it
		if (m_rayimage)
		{
			glRasterPos3i(0, 0, 0);
			glDrawPixels(m_rayimagewidth, m_rayimageheight,
				GL_RGB, GL_UNSIGNED_BYTE, m_rayimage[0]);
		}

		glFlush();
	}
	else
	{
		//
		// Instantiate a renderer
		//

		COpenGLRenderer renderer;

		// Configure the renderer
		ConfigureRenderer(&renderer);

		//
		// Render the scene
		//

		renderer.Render(m_scene);
	}
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_camera.MouseDown(point.x, point.y);

	COpenGLWnd::OnLButtonDown(nFlags, point);
}


void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_camera.MouseMove(point.x, point.y, nFlags))
		Invalidate();

	COpenGLWnd::OnMouseMove(nFlags, point);
}


void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_camera.MouseDown(point.x, point.y, 2);

	COpenGLWnd::OnRButtonDown(nFlags, point);
}

//
// Name :         CChildView::ConfigureRenderer()
// Description :  Configures our renderer so it is able to render the scene.
//                Indicates how we'll do our projection, where the camera is,
//                and where any lights are located.
//

void CChildView::ConfigureRenderer(CGrRenderer* p_renderer)
{
	// Determine the screen size so we can determine the aspect ratio
	int width, height;
	GetSize(width, height);
	double aspectratio = double(width) / double(height);

	//
	// Set up the camera in the renderer
	//

	p_renderer->Perspective(m_camera.FieldOfView(),
		aspectratio, // The aspect ratio.
		20., // Near clipping
		1000.); // Far clipping

	// m_camera.FieldOfView is the vertical field of view in degrees.

	//
	// Set the camera location
	//

	p_renderer->LookAt(m_camera.Eye()[0], m_camera.Eye()[1], m_camera.Eye()[2],
		m_camera.Center()[0], m_camera.Center()[1], m_camera.Center()[2],
		m_camera.Up()[0], m_camera.Up()[1], m_camera.Up()[2]);

	//
	// Set the light locations and colors
	//

	float dimd = 0.2f;
	GLfloat dim[] = { dimd, dimd, dimd, 1.0f };
	GLfloat brightwhite[] = { 1.2f, 1.2f, 1.2f, 1.0f };
	GLfloat cool[] = { 0.35f, 0.45f, 0.9f, 1.0f };

	p_renderer->AddLight(CGrPoint(6.0, 10.0, 3.5, 1.0),
		dim, brightwhite, brightwhite);
	p_renderer->AddLight(CGrPoint(-10.0, 7.0, -8.0, 1.0),
		dim, cool, cool);
}


void CChildView::OnRenderRaytrace()
{
	m_raytrace = !m_raytrace;
	Invalidate();
	if (!m_raytrace)
		return;

	GetSize(m_rayimagewidth, m_rayimageheight);

	m_rayimage = new BYTE * [m_rayimageheight];

	int rowwid = m_rayimagewidth * 3;
	while (rowwid % 4)
		rowwid++;

	m_rayimage[0] = new BYTE[m_rayimageheight * rowwid];
	for (int i = 1; i < m_rayimageheight; i++)
	{
		m_rayimage[i] = m_rayimage[0] + i * rowwid;
	}

	for (int i = 0; i < m_rayimageheight; i++)
	{
		// Fill the image with blue
		for (int j = 0; j < m_rayimagewidth; j++)
		{
			m_rayimage[i][j * 3] = 0;               // red
			m_rayimage[i][j * 3 + 1] = 0;           // green
			m_rayimage[i][j * 3 + 2] = BYTE(255);   // blue
		}
	}
	
	// Instantiate a raytrace object
	CMyRaytraceRenderer raytrace;

	// Generic configurations for all renderers
	ConfigureRenderer(&raytrace);

	//
	// Render the Scene
	//
	raytrace.SetImage(m_rayimage, m_rayimagewidth, m_rayimageheight);
	raytrace.SetWindow(this);
	raytrace.Render(m_scene);
	Invalidate();
}


void CChildView::OnUpdateRenderRaytrace(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_raytrace);
}
