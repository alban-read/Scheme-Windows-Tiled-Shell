
#include "stdafx.h"
#include "D2DView.h"
#include "ContainerApp.h"
#include "Text.h"
#include "Engine.h"
#include <fmt/format.h>
#include <Shlwapi.h>
#include <CommCtrl.h>
#include "scheme.h"
#include <dwrite.h>
#include "Utility.h"
#include <wincodec.h>
#include <wrl.h>
#include <wil/com.h>
#include <deque>
 
 

#define CALL0(who) Scall0(Stop_level_value(Sstring_to_symbol(who)))
#define CALL1(who, arg) Scall1(Stop_level_value(Sstring_to_symbol(who)), arg)
#define CALL2(who, arg, arg2) Scall2(Stop_level_value(Sstring_to_symbol(who)), arg, arg2)

HWND main_window = nullptr;
HANDLE g_image_rotation_mutex=nullptr;
HANDLE g_sprite_commands_mutex = nullptr;

float prefer_width = 800.0f;
float prefer_height = 600.0f;
// represents the visible surface on the window itself.

ID2D1HwndRenderTarget* pRenderTarget;

// stoke width 
float d2d_stroke_width = 1.3;
ID2D1StrokeStyle* d2d_stroke_style = nullptr;

// colours and brushes used when drawing
ID2D1SolidColorBrush* pColourBrush = nullptr;       // line-color
ID2D1SolidColorBrush* pfillColourBrush = nullptr;   // fill-color
ID2D1BitmapBrush* pPatternBrush = nullptr;          // brush-pattern
ID2D1BitmapBrush* pTileBrush = nullptr;				// tile its U/S
ID2D1LinearGradientBrush* pLinearBrush = nullptr;	// linear brush
ID2D1RadialGradientBrush* pRadialBrush = nullptr;	// gradient brush

// double buffers
// this is normally pointed at bitmap 
ID2D1RenderTarget* ActiveRenderTarget = nullptr;

// bitmap and bitmap2 are swapped 
// functions normally draw on this
ID2D1Bitmap* bitmap = nullptr;
ID2D1BitmapRenderTarget* BitmapRenderTarget = nullptr;

// this is normally what is being displayed
ID2D1Bitmap* bitmap2 = nullptr;
ID2D1BitmapRenderTarget* BitmapRenderTarget2 = nullptr;

// sprite batch support
const auto batch_size = 512;
 
 
const auto bank_size = 2048;
// images bank
ID2D1Bitmap* pSpriteSheet[bank_size];

struct sprite_att {
	int width;
	int height;
};

sprite_att sprite_attributes[bank_size];






ID2D1Factory* pD2DFactory;

// hiDPI
float g_DPIScaleX = 1.0f;
float g_DPIScaleY = 1.0f;
float graphics_pen_width = static_cast<float>(1.2);

// fill 
float fill_r = 0.0f;
float fill_g = 0.0f;
float fill_b = 0.0f;
float fill_a = 0.0f;
// line
float line_r = 0.0f;
float line_g = 0.0f;
float line_b = 0.0f;
float line_a = 0.0f;

#pragma warning(disable : 4996)

void d2d_DPIScale(ID2D1Factory* pFactory)
{
	FLOAT dpiX, dpiY;
	pFactory->GetDesktopDpi(&dpiX, &dpiY);
	g_DPIScaleX = dpiX / 96.0f;
	g_DPIScaleY = dpiY / 96.0f;
}

void d2d_set_main_window(HWND w) {
	main_window = w;
}

ptr d2d_color(float r, float g, float b, float a) {

	line_r = r; line_g = g; line_b = b; line_a = a;
	if (pRenderTarget == NULL)
	{
		return Snil;
	}
	SafeRelease(&pColourBrush);
	HRESULT hr = pRenderTarget->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF(r, g, b, a)),
		&pColourBrush
	);
	return Strue;
}

ptr d2d_fill_color(float r, float g, float b, float a) {

	fill_r = r; fill_g = g; fill_b = b; fill_a = a;
	if (pRenderTarget == nullptr)
	{
		return Snil;
	}
	SafeRelease(&pfillColourBrush);
	HRESULT hr = pRenderTarget->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF(r, g, b, a)),
		&pfillColourBrush
	);
	return Strue;
}

ptr d2d_linear_gradient_color
		(float p1, float r1, float g1, float b1, float a1,
		 float p2, float r2, float g2, float b2, float a2,
   	     float p3, float r3, float g3, float b3, float a3) {
	
	if (pRenderTarget == nullptr)
	{
		return Snil;
	}

	D2D1_GRADIENT_STOP stops[] = { 
		{p1, D2D1::ColorF(D2D1::ColorF(r1, g1, b1, a1))},
		{p2, D2D1::ColorF(D2D1::ColorF(r2, g2, b2, a2))},
		{p3, D2D1::ColorF(D2D1::ColorF(r3, g3, b3, a3))} };
 
 
	ID2D1GradientStopCollection* pGradientStops = NULL;
	HRESULT hr = pRenderTarget->CreateGradientStopCollection(
		stops,
		2,
		D2D1_GAMMA_2_2,
		D2D1_EXTEND_MODE_CLAMP,
		&pGradientStops
	);

	const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES& linearGradientBrushProperties = {};

	SafeRelease(&pLinearBrush);

	 hr = pRenderTarget->CreateLinearGradientBrush(
		linearGradientBrushProperties,
		pGradientStops,
		&pLinearBrush
	);
	return Strue;
}

ptr d2d_radial_gradient_color
	(float p1, float r1, float g1, float b1, float a1,
	float p2, float r2, float g2, float b2, float a2,
	float p3, float r3, float g3, float b3, float a3) {

	if (pRenderTarget == nullptr)
	{
		return Snil;
	}

	D2D1_GRADIENT_STOP stops[] = {
		{p1, D2D1::ColorF(D2D1::ColorF(r1, g1, b1, a1))},
		{p2, D2D1::ColorF(D2D1::ColorF(r2, g2, b2, a2))},
		{p3, D2D1::ColorF(D2D1::ColorF(r3, g3, b3, a3))} };

	Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> stop_collection;
	const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES& radialGradientBrushProperties = {};

	SafeRelease(&pRadialBrush);

	HRESULT hr = pRenderTarget->CreateRadialGradientBrush(
		radialGradientBrushProperties,
		stop_collection.Get(),
		&pRadialBrush
	);
	return Strue;
}



void CheckFillBrush()
{
	if (pfillColourBrush == nullptr) {
		d2d_fill_color(0.0, 0.0, 0.0, 1.0);
	}
}

void CheckLineBrush()
{
	if (pColourBrush == nullptr) {
		d2d_color(0.0, 0.0, 0.0, 1.0);
	}
}

void d2d_CreateOffscreenBitmap()
{
	if (pRenderTarget == nullptr)
	{
		return;
	}

	if (BitmapRenderTarget == NULL) {
		pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(prefer_width, prefer_height), &BitmapRenderTarget);
		BitmapRenderTarget->GetBitmap(&bitmap);
	}
	if (BitmapRenderTarget2 == NULL) {
		pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(prefer_width, prefer_height), &BitmapRenderTarget2);
		BitmapRenderTarget2->GetBitmap(&bitmap2);
	}

	ActiveRenderTarget = BitmapRenderTarget;
}

ptr sprite_size(int n) {
	if (n > bank_size - 1) {
		return Sfalse;
	}
	ptr l = Snil;
	l = CALL2("cons", Sflonum(float(sprite_attributes[n].height)), l);
	l = CALL2("cons", Sflonum(float(sprite_attributes[n].width)), l);
	return l;
}

ptr d2d_FreeAllSprites() {

	for (int n = 0; n < bank_size - 1; n++) {
		SafeRelease(&pSpriteSheet[n]);
		sprite_attributes[n].width = 0;
		sprite_attributes[n].height = 0;
	}
	return Strue;
}

ptr d2d_FreeSpriteInBank(int n) {
	if (n > bank_size - 1) {
		return Sfalse;
	}
	SafeRelease(&pSpriteSheet[n]);
	sprite_attributes[n].width = 0;
	sprite_attributes[n].height = 0;
	return Strue;
}

ptr d2d_MakeSpriteInBank(int n, int w, int h, ptr f)
{
	ID2D1RenderTarget* oldRenderTarget;
	ID2D1BitmapRenderTarget* BitmapRender2Bank = nullptr;
	if (n > bank_size - 1) {
		return Sfalse;
	}
	SafeRelease(&pSpriteSheet[n]);
	if (pRenderTarget == nullptr)
	{
		return Sfalse;
	}
	oldRenderTarget = ActiveRenderTarget;
	if (BitmapRender2Bank == NULL) {
		pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(w, h), &BitmapRender2Bank);
		BitmapRender2Bank->GetBitmap(&pSpriteSheet[n]);
	}
	if (BitmapRender2Bank == nullptr) {
		ActiveRenderTarget = oldRenderTarget;
		return Sfalse;
	}
	ActiveRenderTarget = BitmapRender2Bank;
	CheckFillBrush();
	CheckLineBrush();
	ActiveRenderTarget->BeginDraw();
	ptr result = Engine::RunNaked(f);
	HRESULT hr=ActiveRenderTarget->EndDraw();
	ActiveRenderTarget = oldRenderTarget;
	BitmapRender2Bank->Release();
	SafeRelease(&pColourBrush);
	SafeRelease(&pfillColourBrush);
	if (SUCCEEDED(hr)) {
		auto size = pSpriteSheet[n]->GetPixelSize();
		sprite_attributes[n].width = size.width;
		sprite_attributes[n].height = size.height;
	}
	else {
		pSpriteSheet[n] = nullptr;
		sprite_attributes[n].width = 0;
		sprite_attributes[n].height = 0;
		return Sfalse;
	}
	return Strue;
}

void swap_buffers(int n) {

	if (pRenderTarget == nullptr) {
		return;
	}

	ID2D1Bitmap* temp;
	d2d_CreateOffscreenBitmap();
	if (n == 1) {
		BitmapRenderTarget2->BeginDraw();
		BitmapRenderTarget2->DrawBitmap(bitmap, D2D1::RectF(0.0f, 0.0f, prefer_width, prefer_height));
		BitmapRenderTarget2->EndDraw();
	}
	temp = bitmap;
	bitmap = bitmap2;
	bitmap2 = temp;
	ID2D1BitmapRenderTarget* temptarget;
	temptarget = BitmapRenderTarget;
	BitmapRenderTarget = BitmapRenderTarget2;
	BitmapRenderTarget2 = temptarget;
	 
	ActiveRenderTarget = BitmapRenderTarget;
	if (main_window != nullptr) {

		InvalidateRect(main_window, NULL, FALSE);
	}
	Sleep(1);
}

ptr d2d_zclear(float r, float g, float b, float a) {
	ActiveRenderTarget->Clear((D2D1::ColorF(r, g, b, a)));
	return Strue;
}

ptr d2d_clear(float r, float g, float b, float a) {

	if (pRenderTarget == nullptr || ActiveRenderTarget ==nullptr) {
		return Sfalse;
	}
	WaitForSingleObject(g_image_rotation_mutex, INFINITE);
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->Clear((D2D1::ColorF(r, g, b, a)));
	ActiveRenderTarget->EndDraw();
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->Clear((D2D1::ColorF(r, g, b, a)));
	ActiveRenderTarget->EndDraw();
	ReleaseMutex(g_image_rotation_mutex);
	return Strue;
}

// cannot save hw rendered bitmap to image :( 
ptr d2d_save(char* filename) {
	return Snil;
}

void render_sprite_commands();
ptr d2d_show(int n)
{
	if (n == 2) render_sprite_commands();  
	swap_buffers(n);
	if (main_window != nullptr)
		PostMessageW(main_window, WM_USER + 501, (WPARAM)0, (LPARAM)0);
	return Strue;
}

ptr d2d_set_stroke_width(float w) {
	d2d_stroke_width = w;
	return Strue;
}


// unable to access these new D2Dfeatures.
ptr d2d_DrawSpriteBatch() {




	return Strue;
}












ptr d2d_zfill_ellipse(float x, float y, float w, float h) {
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);
	ActiveRenderTarget->FillEllipse(ellipse, pfillColourBrush);
	return Strue;
}

ptr d2d_fill_ellipse(float x, float y, float w, float h) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	CheckFillBrush();
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->FillEllipse(ellipse, pfillColourBrush);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_zellipse(float x, float y, float w, float h) {
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);
	ActiveRenderTarget->DrawEllipse(ellipse, pColourBrush, d2d_stroke_width);
	return Strue;
}

ptr d2d_ellipse(float x, float y, float w, float h) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	if (pColourBrush == nullptr) {
		d2d_color(0.0, 0.0, 0.0, 1.0);
	}
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawEllipse(ellipse, pColourBrush, d2d_stroke_width);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_zline(float x, float y, float x1, float y1) {
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	auto p1 = D2D1::Point2F(x, y);
	auto p2 = D2D1::Point2F(x1, y1);
	ActiveRenderTarget->DrawLine(p1, p2, pColourBrush, stroke_width, stroke_style);
	return Strue;
}

ptr d2d_line(float x, float y, float x1, float y1) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	CheckLineBrush();
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	auto p1 = D2D1::Point2F(x, y);
	auto p2 = D2D1::Point2F(x1, y1);
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawLine(p1, p2, pColourBrush, stroke_width, stroke_style);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

// runs lambda f inside a draw operation
ptr d2d_draw_func(ptr f) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	CheckLineBrush();
	CheckFillBrush();
	try {
		ActiveRenderTarget->BeginDraw();
		ptr result=Engine::RunNaked(f);
		ActiveRenderTarget->EndDraw();
		return result;
	}
	catch (const CException& e)
	{
		return Sfalse;
	}

	return Strue;
}

ptr d2d_zrectangle(float x, float y, float w, float h) {
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);
	ActiveRenderTarget->DrawRectangle(&rectangle1, pColourBrush, stroke_width);
	return Strue;
}

ptr d2d_rectangle(float x, float y, float w, float h) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	CheckLineBrush();
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawRectangle(&rectangle1, pColourBrush, stroke_width);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_rounded_rectangle(float x, float y, float w, float h, float rx, float ry) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	CheckLineBrush();
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	D2D1_ROUNDED_RECT rectangle1 = { D2D1::RectF(x, y, w, h), rx, ry };
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawRoundedRectangle(rectangle1, pfillColourBrush, stroke_width, stroke_style);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_zfill_rectangle(float x, float y, float w, float h) {
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);
	ActiveRenderTarget->FillRectangle(&rectangle1, pfillColourBrush);
	return Strue;
}

ptr d2d_fill_rectangle(float x, float y, float w, float h) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	CheckFillBrush();
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->FillRectangle(&rectangle1, pfillColourBrush);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_zlinear_gradient_fill_rectangle(
	float x, float y, float w, float h,
	float x1, float y1, float x2, float y2) {
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);
	pLinearBrush->SetStartPoint(D2D1_POINT_2F({ x1, y1 }));
	pLinearBrush->SetEndPoint(D2D1_POINT_2F({ x2, y2 }));
	ActiveRenderTarget->FillRectangle(&rectangle1, pLinearBrush);
	return Strue;
}

ptr d2d_linear_gradient_fill_rectangle(
	float x, float y, float w, float h,
	float x1, float y1, float x2, float y2) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr || pLinearBrush == nullptr) {
		return Sfalse;
	}
 
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);
	ActiveRenderTarget->BeginDraw();
	pLinearBrush->SetStartPoint(D2D1_POINT_2F({ x1, y1 }));
	pLinearBrush->SetEndPoint(D2D1_POINT_2F({ x2, y2 }));
	ActiveRenderTarget->FillRectangle(&rectangle1, pLinearBrush);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_zradial_gradient_fill_rectangle(
	float x, float y, float w, float h,
	float x1, float y1, float r1, float r2) {
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);
	pRadialBrush->SetCenter(D2D1_POINT_2F({ x1, y1 }));
	pRadialBrush->SetRadiusX(r1);
	pRadialBrush->SetRadiusY(r2);
	ActiveRenderTarget->FillRectangle(&rectangle1, pRadialBrush);
	return Strue;
}

ptr d2d_radial_gradient_fill_rectangle(
	float x, float y, float w, float h,
	float x1, float y1, float r1, float r2) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr || pRadialBrush == nullptr) {
		return Sfalse;
	}
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);
	ActiveRenderTarget->BeginDraw();
	pRadialBrush->SetCenter(D2D1_POINT_2F({ x1, y1 }));
	pRadialBrush->SetRadiusX(r1);
	pRadialBrush->SetRadiusY(r2);
	ActiveRenderTarget->FillRectangle(&rectangle1, pRadialBrush);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_zradial_gradient_fill_ellipse(
	float x, float y, float w, float h,
	float x1, float y1, float r1, float r2) {
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);
	pRadialBrush->SetCenter(D2D1_POINT_2F({ x1, y1 }));
	pRadialBrush->SetRadiusX(r1);
	pRadialBrush->SetRadiusY(r2);
	ActiveRenderTarget->FillEllipse(ellipse, pLinearBrush);
	return Strue;
}

ptr d2d_radial_gradient_fill_ellipse(
	float x, float y, float w, float h,
	float x1, float y1, float r1, float r2) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);
	ActiveRenderTarget->BeginDraw();
	pRadialBrush->SetCenter(D2D1_POINT_2F({ x1, y1 }));
	pRadialBrush->SetRadiusX(r1);
	pRadialBrush->SetRadiusY(r2);
	ActiveRenderTarget->FillEllipse(ellipse, pLinearBrush);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_zlinear_gradient_fill_ellipse(
	float x, float y, float w, float h,
	float x1, float y1, float x2, float y2) {
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);
	pLinearBrush->SetStartPoint(D2D1_POINT_2F({ x1, y1 }));
	pLinearBrush->SetEndPoint(D2D1_POINT_2F({ x2, y2 }));
	ActiveRenderTarget->FillEllipse(ellipse, pLinearBrush);
	return Strue;
}

ptr d2d_linear_gradient_fill_ellipse(
	float x, float y, float w, float h,
	float x1, float y1, float x2, float y2) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr || pLinearBrush == nullptr) {
		return Sfalse;
	}
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);
	ActiveRenderTarget->BeginDraw();
	pLinearBrush->SetStartPoint(D2D1_POINT_2F({ x1, y1 }));
	pLinearBrush->SetEndPoint(D2D1_POINT_2F({ x2, y2 }));
	ActiveRenderTarget->FillEllipse(ellipse, pLinearBrush);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_zmatrix_identity() {
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	return Strue;
}

ptr d2d_matrix_identity() {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	return Strue;
}

ptr d2d_matrix_rotate(float a, float x, float y) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y)));
	return Strue;
}

ptr d2d_zmatrix_translate(float x, float y) {
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Translation(x, y));
	return Strue;
}

ptr d2d_matrix_translate(float x, float y) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Translation(40, 10));
	return Strue;
}

ptr d2d_zmatrix_rotrans(float a, float x, float y, float x1, float y1) {
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1, y1);
	ActiveRenderTarget->SetTransform(rot * trans);
	return Strue;
}

ptr d2d_matrix_rotrans(float a, float x, float y, float x1, float y1) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1, y1);
	ActiveRenderTarget->SetTransform(rot * trans);
	return Strue;
}

ptr d2d_zmatrix_transrot(float a, float x, float y, float x1, float y1) {
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1, y1);
	ActiveRenderTarget->SetTransform(trans * rot);
	return Strue;
}

ptr d2d_matrix_transrot(float a, float x, float y, float x1, float y1) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1, y1);
	ActiveRenderTarget->SetTransform(trans * rot);
	return Strue;
}

ptr d2d_matrix_scale(float x, float y) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Scale(
			D2D1::Size(x, y),
			D2D1::Point2F(prefer_width / 2, prefer_height / 2))
	);
	return Strue;
}

ptr d2d_matrix_rotscale(float a, float x, float y, float x1, float y1) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(x, y),
		D2D1::Point2F(prefer_width / 2, prefer_height / 2));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	ActiveRenderTarget->SetTransform(rot * scale);
	return Strue;
}

ptr d2d_matrix_scalerot(float a, float x, float y, float x1, float y1) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(x, y),
		D2D1::Point2F(prefer_width / 2, prefer_height / 2));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	ActiveRenderTarget->SetTransform(rot * scale);
	return Strue;
}

ptr d2d_matrix_scalerottrans(float a, float x, float y, float x1, float y1, float x2, float y2) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(x, y),
		D2D1::Point2F(prefer_width / 2, prefer_height / 2));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1, y1);
	ActiveRenderTarget->SetTransform(scale * rot * trans);
	return Strue;
}

ptr d2d_zmatrix_skew(float x, float y, float w, float h) {
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Skew(
			x, y,
			D2D1::Point2F(w, h))
	);
	return Strue;
}

ptr d2d_matrix_skew(float x, float y) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Skew(
			x, y,
			D2D1::Point2F(prefer_width / 2, prefer_height / 2))
	);
	return Strue;
}

// display current display buffer.
ptr d2d_render(float x, float y) {

	swap_buffers(1);
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawBitmap(bitmap2, D2D1::RectF(x, y, prefer_width, prefer_height));
	ActiveRenderTarget->EndDraw();
	return Strue;
}

int d2d_CreateGridPatternBrush(
	ID2D1RenderTarget* pRenderTarget,
	ID2D1BitmapBrush** ppBitmapBrush
)
{
	if (pPatternBrush != nullptr) {
		return 0;
	}
	// Create a compatible render target.
	ID2D1BitmapRenderTarget* pCompatibleRenderTarget = nullptr;
	HRESULT hr = pRenderTarget->CreateCompatibleRenderTarget(
		D2D1::SizeF(10.0f, 10.0f),
		&pCompatibleRenderTarget
	);
	if (SUCCEEDED(hr))
	{
		// Draw a pattern.
		ID2D1SolidColorBrush* pGridBrush = nullptr;
		hr = pCompatibleRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF(0.93f, 0.94f, 0.96f, 1.0f)),
			&pGridBrush
		);

		// create offscreen bitmap
		d2d_CreateOffscreenBitmap();

		if (SUCCEEDED(hr))
		{
			pCompatibleRenderTarget->BeginDraw();
			pCompatibleRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, 10.0f, 1.0f), pGridBrush);
			pCompatibleRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.1f, 1.0f, 10.0f), pGridBrush);
			pCompatibleRenderTarget->EndDraw();

			// Retrieve the bitmap from the render target.
			ID2D1Bitmap* pGridBitmap = nullptr;
			hr = pCompatibleRenderTarget->GetBitmap(&pGridBitmap);
			if (SUCCEEDED(hr))
			{
				// Choose the tiling mode for the bitmap brush.
				D2D1_BITMAP_BRUSH_PROPERTIES brushProperties =
					D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP);

				// Create the bitmap brush.
				hr = pRenderTarget->CreateBitmapBrush(pGridBitmap, brushProperties, ppBitmapBrush);

				SafeRelease(&pGridBitmap);
			}

			SafeRelease(&pGridBrush);

		}

		SafeRelease(&pCompatibleRenderTarget);

	}

	return hr;
}

void d2d_make_default_stroke() {
	HRESULT r = pD2DFactory->CreateStrokeStyle(
		D2D1::StrokeStyleProperties(
			D2D1_CAP_STYLE_FLAT,
			D2D1_CAP_STYLE_FLAT,
			D2D1_CAP_STYLE_ROUND,
			D2D1_LINE_JOIN_MITER,
			1.0f,
			D2D1_DASH_STYLE_SOLID,
			0.0f),
		nullptr, 0,
		&d2d_stroke_style
	);
}


// direct write
IDWriteFactory* pDWriteFactory;
IDWriteTextFormat* TextFont;
ID2D1SolidColorBrush* pBlackBrush;

ptr d2d_text_mode(int n) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	D2D1_TEXT_ANTIALIAS_MODE mode = (enum D2D1_TEXT_ANTIALIAS_MODE)n;
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->SetTextAntialiasMode(mode);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_zwrite_text(float x, float y, char* s) {
	const auto text = Utility::widen(s);
	const auto len = text.length();
	D2D1_RECT_F layoutRect = D2D1::RectF(x, y, prefer_width - x, prefer_height - y);
	ActiveRenderTarget->DrawTextW(text.c_str(), len, TextFont, layoutRect, pfillColourBrush);
	return Strue;
}

ptr d2d_write_text(float x, float y, char* s) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	CheckFillBrush();

	const auto text = Utility::widen(s);
	const auto len = text.length();

	D2D1_RECT_F layoutRect = D2D1::RectF(x, y, prefer_width - x, prefer_height - y);

	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawTextW(text.c_str(), len, TextFont, layoutRect, pfillColourBrush);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_set_font(char* s, float size) {

	SafeRelease(&TextFont);
	auto face = Utility::widen(s);
	HRESULT
		hr = pDWriteFactory->CreateTextFormat(
			face.c_str(),
			NULL,
			DWRITE_FONT_WEIGHT_REGULAR,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			size,
			L"en-us",
			&TextFont
		);
	if (SUCCEEDED(hr)) {
		return Strue;
	}
	return Snil;
}

void d2d_sprite_loader(char* filename, int n)
{
	if (n > bank_size - 1) {
		return;
	}

	const auto locate_file = Utility::GetFullPathFor(Utility::widen(filename).c_str());
	if ((INVALID_FILE_ATTRIBUTES == GetFileAttributes(locate_file.c_str()) &&
		GetLastError() == ERROR_FILE_NOT_FOUND)) return;

	HRESULT hr;
	CoInitialize(NULL);
	IWICImagingFactory* wicFactory = NULL;
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory,
		(LPVOID*)&wicFactory);

	if (FAILED(hr)) {
		return;
	}
	//create a decoder
	IWICBitmapDecoder* wicDecoder = NULL;
	std::wstring fname = locate_file;
	hr = wicFactory->CreateDecoderFromFilename(
		fname.c_str(),
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&wicDecoder);

	if (wicDecoder == NULL) {
		return;
	}

	IWICBitmapFrameDecode* wicFrame = NULL;
	hr = wicDecoder->GetFrame(0, &wicFrame);

	// create a converter
	IWICFormatConverter* wicConverter = NULL;
	hr = wicFactory->CreateFormatConverter(&wicConverter);

	// setup the converter
	hr = wicConverter->Initialize(
		wicFrame,
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		NULL,
		0.0,
		WICBitmapPaletteTypeCustom
	);
	if (SUCCEEDED(hr))
	{
		hr = pRenderTarget->CreateBitmapFromWicBitmap(
			wicConverter,
			NULL,
			&pSpriteSheet[n]
		);
		if (SUCCEEDED(hr)) {
			auto size = pSpriteSheet[n]->GetPixelSize();
			sprite_attributes[n].width = size.width;
			sprite_attributes[n].height = size.height;
		}
		else {
			pSpriteSheet[n] = nullptr;
			sprite_attributes[n].width = 0;
			sprite_attributes[n].height = 0;
		}
	}
	SafeRelease(&wicFactory);
	SafeRelease(&wicDecoder);
	SafeRelease(&wicConverter);
	SafeRelease(&wicFrame);
}

ptr d2d_load_sprites(char* filename, int n) {
	if (n > bank_size - 1) {
		return Snil;
	}
	d2d_sprite_loader(filename, n);
	return Strue;
}

ptr d2d_zrender_sprite(int n, float dx, float dy) {

	if (n > bank_size - 1) {
		return Snil;
	}
 
	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, dx + size.width, dy + size.height);
	const auto opacity = 1.0f;
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Scale(
			D2D1::Size(1.0, 1.0),
			D2D1::Point2F(size.width / 2, size.height / 2)));
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest);
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	return Strue;
}

// from sheet n; at sx, sy to dx, dy, w,h
ptr d2d_render_sprite(int n, float dx, float dy) {

	if (n > bank_size - 1) {
		return Snil;
	}
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, dx + size.width, dy + size.height);
	const auto opacity = 1.0f;
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Scale(
			D2D1::Size(1.0, 1.0),
			D2D1::Point2F(size.width / 2, size.height / 2)));
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest);
	ActiveRenderTarget->EndDraw();
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	return Strue;
}

ptr d2d_zrender_sprite_rotscale(int n, float dx, float dy, float a, float s) {

	if (n > bank_size - 1) {
		return Snil;
	}
 
	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, dx + size.width, dy + size.height);
	const auto opacity = 1.0f;

	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(s, s),
		D2D1::Point2F(dx, dy));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(dx + (size.width / 2), dy + (size.height / 2)));
	ActiveRenderTarget->SetTransform(rot * scale);
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest);
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	return Strue;
}

ptr d2d_render_sprite_rotscale(int n, float dx, float dy, float a, float s) {

	if (n > bank_size - 1) {
		return Snil;
	}
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, dx + size.width, dy + size.height);
	const auto opacity = 1.0f;

	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(s, s),
		D2D1::Point2F(dx, dy));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(dx + (size.width / 2), dy + (size.height / 2)));

	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->SetTransform(rot * scale);
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest);
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	ActiveRenderTarget->EndDraw();
	return Strue;
}

// from sheet n; at sx, sy to dx, dy, w,h scale up
ptr d2d_zrender_sprite_sheet(int n, float dx, float dy, float dw, float dh,
	float sx, float sy, float sw, float sh, float scale) {

	if (n > bank_size - 1) {
		return Snil;
	}

	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, scale * (dx + dw), scale * (dy + dh));
	const auto source = D2D1::RectF(sx, sy, sx + sw, sy + sh);
	const auto opacity = 1.0f;
 
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest, 1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, source);
 
	return Strue;
}

// from sheet n; at sx, sy to dx, dy, w,h scale up
ptr d2d_render_sprite_sheet(int n, float dx, float dy, float dw, float dh,
	float sx, float sy, float sw, float sh, float scale) {

	if (n > bank_size - 1) {
		return Snil;
	}

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}

	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, scale * (dx + dw), scale * (dy + dh));
	const auto source = D2D1::RectF(sx, sy, sx + sw, sy + sh);
	const auto opacity = 1.0f;
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest, 1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, source);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

// from sheet n; at sx, sy to dx, dy, w,h scale up
ptr d2d_zrender_sprite_sheet_rot_scale(int n, float dx, float dy, float dw, float dh,
	float sx, float sy, float sw, float sh, float scale, float a, float x2, float y2) {

	if (n > bank_size - 1) {
		return Snil;
	}
 
	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, scale * (dx + dw), scale * (dy + dh));
	const auto source = D2D1::RectF(sx, sy, sx + sw, sy + sh);
	const auto opacity = 1.0f;
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x2, y2));
 
	ActiveRenderTarget->SetTransform(rot);
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest, 1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, source);
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
 
	return Strue;
}

// from sheet n; at sx, sy to dx, dy, w,h scale up
ptr d2d_render_sprite_sheet_rot_scale(int n, float dx, float dy, float dw, float dh,
	float sx, float sy, float sw, float sh, float scale, float a, float x2, float y2) {

	if (n > bank_size - 1) {
		return Snil;
	}
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}

	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, scale * (dx + dw), scale * (dy + dh));
	const auto source = D2D1::RectF(sx, sy, sx + sw, sy + sh);
	const auto opacity = 1.0f;
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x2, y2));
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->SetTransform(rot);
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest, 1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, source);
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	ActiveRenderTarget->EndDraw();
	return Strue;
}

void CreateFactory() {

	if (pD2DFactory == NULL) {
		HRESULT hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2DFactory);
		d2d_DPIScale(pD2DFactory);
	}
	if (pDWriteFactory == NULL) {
		HRESULT hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&pDWriteFactory)
		);
	}
}

HRESULT Create_D2D_Device_Dep(HWND h)
{
	if (IsWindow(h)) {

		if (pRenderTarget == NULL)
		{

			CreateFactory();

			RECT rc;
			GetClientRect(h, &rc);

			D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

			HRESULT hr = pD2DFactory->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(h,
					D2D1::SizeU((UINT32)rc.right, (UINT32)rc.bottom)),
				&pRenderTarget);

			if (FAILED(hr)) {
		
				return hr;
			}

			d2d_CreateGridPatternBrush(pRenderTarget, &pPatternBrush);

			hr = pDWriteFactory->CreateTextFormat(
				L"Consolas",
				NULL,
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				32.0f,
				L"en-us",
				&TextFont
			);

			if (FAILED(hr)) {
		
				return hr;
			}

			hr = pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Black),
				&pBlackBrush
			);
			CheckFillBrush();
			CheckLineBrush();

			if (FAILED(hr)) {
	
				return hr;
			}

			return hr;
		}
	}
	else {
	
	}
	return 0;
}

void safe_release() {

	SafeRelease(&pRenderTarget);
	SafeRelease(&pPatternBrush);
	SafeRelease(&pBlackBrush);
	SafeRelease(&pD2DFactory);
	SafeRelease(&TextFont);
	SafeRelease(&BitmapRenderTarget);
	SafeRelease(&BitmapRenderTarget2);
}

int complexity_limit = 500;
int commands_length;
const int ignore_clip = 200;
const int sprite_command_size=8192;

struct sprite_command {
	bool active=false;
	bool persist = false;
	float r;
	float g;
	float b;
	float a;
	float x;
	float y;
	float w;
	float h;
	float sx;
	float sy;
	float sw;
	float sh;
	float xn = 0.0f;
	float yn = 0.0f;
	float s;
	float angle;
	int	bank;
	int render_type;
	int f = 0;
	std::wstring text;
};

sprite_command sprite_commands[sprite_command_size];

ptr set_draw_sprite(int c, int n, float x, float y) {

	if (x > prefer_width+ignore_clip) return Snil;
	if (x < -ignore_clip) return Snil;
	if (y < -ignore_clip) return Snil;
	if (y > prefer_height+ignore_clip) return Snil;

	if (c > sprite_command_size - 1) {
		return Snil;
	}
	if (n > bank_size - 1) {
		return Snil;
	}
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	sprite_commands[c].active = true;
	sprite_commands[c].bank = n;
	sprite_commands[c].x = x;
	sprite_commands[c].y = y;
	sprite_commands[c].render_type = 1;  
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

ptr add_draw_sprite(int n, float x, float y) {

	if (x > prefer_width + ignore_clip) return Snil;
	if (x < -ignore_clip) return Snil;
	if (y < -ignore_clip) return Snil;
	if (y > prefer_height + ignore_clip) return Snil;
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	sprite_commands[c].active = true;
	sprite_commands[c].bank = n;
	sprite_commands[c].x = x;
	sprite_commands[c].y = y;
	sprite_commands[c].render_type = 1;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

ptr add_draw_rect(float x, float y, float w, float h) {
	
	if (x > prefer_width + ignore_clip) return Snil;
	if (x < -ignore_clip) return Snil;
	if (y < -ignore_clip) return Snil;
	if (y > prefer_height + ignore_clip) return Snil;

	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	sprite_commands[c].active = true;
	sprite_commands[c].x = x;
	sprite_commands[c].y = y;
	sprite_commands[c].w = w;
	sprite_commands[c].h = h;
	sprite_commands[c].render_type = 20;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

ptr add_fill_rect(float x, float y, float w, float h) {

	if (x > prefer_width + ignore_clip) return Snil;
	if (x < -ignore_clip) return Snil;
	if (y < -ignore_clip) return Snil;
	if (y > prefer_height + ignore_clip) return Snil;

	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	sprite_commands[c].active = true;
	sprite_commands[c].x = x;
	sprite_commands[c].y = y;
	sprite_commands[c].w = w;
	sprite_commands[c].h = h;
	sprite_commands[c].render_type = 21;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}


ptr add_linear_gradient_fill_rect(
	float x, float y, float w, float h,
	float x1, float y1, float x2, float y2) {

	if (x > prefer_width + ignore_clip) return Snil;
	if (x < -ignore_clip) return Snil;
	if (y < -ignore_clip) return Snil;
	if (y > prefer_height + ignore_clip) return Snil;

	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	sprite_commands[c].active = true;
	sprite_commands[c].x = x;
	sprite_commands[c].y = y;
	sprite_commands[c].w = w;
	sprite_commands[c].h = h;
	sprite_commands[c].sx = x1;
	sprite_commands[c].sy = y1;
	sprite_commands[c].xn = x2;
	sprite_commands[c].yn = y2;
	sprite_commands[c].render_type = 41;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

ptr add_radial_gradient_fill_rect(
	float x, float y, float w, float h,
	float x1, float y1, float x2, float y2) {

	if (x > prefer_width + ignore_clip) return Snil;
	if (x < -ignore_clip) return Snil;
	if (y < -ignore_clip) return Snil;
	if (y > prefer_height + ignore_clip) return Snil;

	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	sprite_commands[c].active = true;
	sprite_commands[c].x = x;
	sprite_commands[c].y = y;
	sprite_commands[c].w = w;
	sprite_commands[c].h = h;
	sprite_commands[c].sx = x1;
	sprite_commands[c].sy = y1;
	sprite_commands[c].xn = x2;
	sprite_commands[c].yn = y2;
	sprite_commands[c].render_type = 42;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}



ptr add_ellipse (float x, float y, float w, float h) {
	
	if (x > prefer_width + ignore_clip) return Snil;
	if (x < -ignore_clip) return Snil;
	if (y < -ignore_clip) return Snil;
	if (y > prefer_height + ignore_clip) return Snil;

	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	sprite_commands[c].active = true;
	sprite_commands[c].x = x;
	sprite_commands[c].y = y;
	sprite_commands[c].w = w;
	sprite_commands[c].h = h;
	sprite_commands[c].render_type = 22;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

ptr add_fill_ellipse(float x, float y, float w, float h) {
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	if (x > prefer_width + ignore_clip) return Snil;
	if (x < -ignore_clip) return Snil;
	if (y < -ignore_clip) return Snil;
	if (y > prefer_height + ignore_clip) return Snil;

	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	sprite_commands[c].active = true;
	sprite_commands[c].x = x;
	sprite_commands[c].y = y;
	sprite_commands[c].w = w;
	sprite_commands[c].h = h;
	sprite_commands[c].render_type = 23;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

ptr add_line(float x, float y, float w, float h) {

	if (x > prefer_width + ignore_clip) return Snil;
	if (x < -ignore_clip) return Snil;
	if (y < -ignore_clip) return Snil;
	if (y > prefer_height + ignore_clip) return Snil;
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	sprite_commands[c].active = true;
	sprite_commands[c].x = x;
	sprite_commands[c].y = y;
	sprite_commands[c].w = w;
	sprite_commands[c].h = h;
	sprite_commands[c].render_type = 24;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);	
	return Strue;
}

ptr add_clear_image(float r, float g, float b, float a) {
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	sprite_commands[c].active = true;
	sprite_commands[c].r = r;
	sprite_commands[c].g = g;
	sprite_commands[c].b = b;
	sprite_commands[c].a = a;
	sprite_commands[c].render_type = 8;	
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

ptr add_line_colour(float r, float g, float b, float a) {
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	sprite_commands[c].active = true;
	sprite_commands[c].r = r;
	sprite_commands[c].g = g;
	sprite_commands[c].b = b;
	sprite_commands[c].a = a;
	sprite_commands[c].render_type = 10;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

ptr add_fill_colour(float r, float g, float b, float a) {
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	sprite_commands[c].active = true;
	sprite_commands[c].r = r;
	sprite_commands[c].g = g;
	sprite_commands[c].b = b;
	sprite_commands[c].a = a;
	sprite_commands[c].render_type = 11;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

ptr add_pen_width(float w) {
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	sprite_commands[c].active = true;
	sprite_commands[c].w = w;
	sprite_commands[c].render_type = 12;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

ptr add_write_text(float x, float y, char*s) {
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	sprite_commands[c].active = true;
	sprite_commands[c].x = x;
	sprite_commands[c].y = y;

	sprite_commands[c].r = fill_r;
	sprite_commands[c].g = fill_g;
	sprite_commands[c].b = fill_b;
	sprite_commands[c].a = fill_a;

	sprite_commands[c].text = Utility::widen(s);
	sprite_commands[c].render_type = 9;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

ptr add_scaled_rotated_sprite(int n, float x, float y, float a, float s) {

	if (x > prefer_width + ignore_clip) return Snil;
	if (x < -ignore_clip) return Snil;
	if (y < -ignore_clip) return Snil;
	if (y > prefer_height + ignore_clip) return Snil;
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	sprite_commands[c].active = true;
	sprite_commands[c].bank = n;
	sprite_commands[c].x = x;
	sprite_commands[c].y = y;
	sprite_commands[c].angle = a;
	sprite_commands[c].s = s;
	sprite_commands[c].render_type = 2;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

ptr add_render_sprite_sheet(int n, 
	float dx, float dy, float dw, float dh,
	float sx, float sy, float sw, float sh, 
	float scale) {

	if (dx > prefer_width + ignore_clip) return Snil;
	if (dx < -ignore_clip) return Snil;
	if (dy < -ignore_clip) return Snil;
	if (dy > prefer_height + ignore_clip) return Snil;
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	int c = 1;
	while (c < sprite_command_size && sprite_commands[c].render_type > 0) c++;
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	sprite_commands[c].active = true;
	sprite_commands[c].bank = n;
	sprite_commands[c].x = dx;
	sprite_commands[c].y = dy;
	sprite_commands[c].w = dw;
	sprite_commands[c].h = dh;
	sprite_commands[c].sx = sx;
	sprite_commands[c].sy = sy;
	sprite_commands[c].sw = sw;
	sprite_commands[c].sh = sh; 
	sprite_commands[c].s = scale;
	sprite_commands[c].render_type = 3;
	commands_length++;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

ptr clear_all_draw_sprite() {
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	for (int i = 0; i < sprite_command_size - 1; i++) {
		sprite_commands[i].active = false;
		sprite_commands[i].bank = bank_size + 1;
		sprite_commands[i].render_type = 0;
	}
	ReleaseMutex(g_sprite_commands_mutex);
	return Snil;
}

ptr clear_draw_sprite(int c) {
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	if (c > sprite_command_size - 1) {
		ReleaseMutex(g_sprite_commands_mutex);
		return Snil;
	}
	sprite_commands[c].active = false;
	sprite_commands[c].bank = bank_size+1;
	sprite_commands[c].render_type = 0;
	ReleaseMutex(g_sprite_commands_mutex);
	return Strue;
}

void do_write_text(float x, float y, std::wstring s) {
	const auto len = s.length();
	D2D1_RECT_F layoutRect = D2D1::RectF(x, y, prefer_width - x, prefer_height - y);
	ActiveRenderTarget->DrawTextW(s.c_str(), len, TextFont, layoutRect, pfillColourBrush);
}

void do_draw_sprite(int n, float x, float y) {
	d2d_zrender_sprite(n, x, y);
}

void do_scaled_rotated_sprite(int n, float x, float y, float a, float s) {
	d2d_zrender_sprite_rotscale(n, x, y, a, s);
}

void do_render_sprite_sheet(int n, float dx, float dy, float dw, float dh,
	float sx, float sy, float sw, float sh, float scale) {
	d2d_zrender_sprite_sheet(n, dx, dy, dw, dh,
		sx, sy, sw, sh, scale);
}

void render_sprite_commands() {
	 
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return;
	}
	WaitForSingleObject(g_sprite_commands_mutex, INFINITE);
	ActiveRenderTarget->BeginDraw();

	for (int i = 1; i < commands_length+1; i++) {

		if (sprite_commands[i].active == true) {
			switch (sprite_commands[i].render_type) {
			case 1:
				do_draw_sprite(
					sprite_commands[i].bank,
					sprite_commands[i].x,
					sprite_commands[i].y);
				break;
			case 2:
				do_scaled_rotated_sprite(
					sprite_commands[i].bank,
					sprite_commands[i].x,
					sprite_commands[i].y,
					sprite_commands[i].angle,
					sprite_commands[i].s);
				break;
			case 3:
				do_render_sprite_sheet(
					sprite_commands[i].bank,
					sprite_commands[i].x,
					sprite_commands[i].y,
					sprite_commands[i].w,
					sprite_commands[i].h,
					sprite_commands[i].sx,
					sprite_commands[i].sy,
					sprite_commands[i].sw,
					sprite_commands[i].sh,
					sprite_commands[i].s);
				break;
			case 8:
				d2d_zclear(
					sprite_commands[i].r,
					sprite_commands[i].g,
					sprite_commands[i].b,
					sprite_commands[i].a);
				break;
			case 9:
				do_write_text(
					sprite_commands[i].x,
					sprite_commands[i].y,
					sprite_commands[i].text);
				break;
			case 10:
				d2d_color(
					sprite_commands[i].r,
					sprite_commands[i].g,
					sprite_commands[i].b,
					sprite_commands[i].a);
				break;
			case 11:
				d2d_fill_color(
					sprite_commands[i].r,
					sprite_commands[i].g,
					sprite_commands[i].b,
					sprite_commands[i].a);
				break;
			case 12:
				d2d_stroke_width = sprite_commands[i].w;
				break;
			case 20:
				d2d_zrectangle(
					sprite_commands[i].x,
					sprite_commands[i].y,
					sprite_commands[i].w,
					sprite_commands[i].h);
				break;
			case 21:
				d2d_zfill_rectangle(
					sprite_commands[i].x,
					sprite_commands[i].y,
					sprite_commands[i].w,
					sprite_commands[i].h);
				break;
			case 22:
				d2d_zellipse(
					sprite_commands[i].x,
					sprite_commands[i].y,
					sprite_commands[i].w,
					sprite_commands[i].h);
				break;
			case 23:
				d2d_zfill_ellipse(
					sprite_commands[i].x,
					sprite_commands[i].y,
					sprite_commands[i].w,
					sprite_commands[i].h);
				break;
			case 24:
				d2d_zline(
					sprite_commands[i].x,
					sprite_commands[i].y,
					sprite_commands[i].w,
					sprite_commands[i].h);
				break;
			case 41:
				d2d_zlinear_gradient_fill_rectangle(
					sprite_commands[i].x,
					sprite_commands[i].y,
					sprite_commands[i].w,
					sprite_commands[i].h,
					sprite_commands[i].sx,
					sprite_commands[i].sy,
					sprite_commands[i].xn,
					sprite_commands[i].yn);
				break;
			case 42:
				d2d_zradial_gradient_fill_rectangle(
					sprite_commands[i].x,
					sprite_commands[i].y,
					sprite_commands[i].w,
					sprite_commands[i].h,
					sprite_commands[i].sx,
					sprite_commands[i].sy,
					sprite_commands[i].xn,
					sprite_commands[i].yn);
				break;
			}

		}
		
		if ((i % complexity_limit) == 0) {
			ActiveRenderTarget->EndDraw();
			ActiveRenderTarget->BeginDraw();
		}
	}
	ActiveRenderTarget->EndDraw();
	// clear for next step
	for (int i = 0; i < sprite_command_size - 1; i++) {
		sprite_commands[i].active = false;
		sprite_commands[i].bank = bank_size + 1;
		sprite_commands[i].render_type = 0;
		sprite_commands[i].text.clear();
	}
	commands_length = 0;
	ReleaseMutex(g_sprite_commands_mutex);
}

ptr d2d_image_size(int w, int h)
{
	prefer_width = w;
	prefer_height = h;

	safe_release();

	Create_D2D_Device_Dep(main_window);
	while (pRenderTarget == nullptr) {
		Sleep(10);
	}
	d2d_fill_rectangle(0.0, 0.0, 1.0 * w, 1.0 * h);

	return Strue;
}


ptr d2d_release() {
	safe_release();
	return Strue;
}

void onPaint(HWND hWnd) {

	WaitForSingleObject(g_image_rotation_mutex, INFINITE);

	if (main_window == nullptr) {
		main_window = hWnd;
	}

	if (main_window != hWnd) {
		main_window = hWnd;
		safe_release();
	}

	RECT rc;
	::GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
	HRESULT
	hr = Create_D2D_Device_Dep(hWnd);
	if (SUCCEEDED(hr))
	{

		pRenderTarget->Resize(size);
		pRenderTarget->BeginDraw();
		D2D1_SIZE_F renderTargetSize = pRenderTarget->GetSize();
		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::LightGray));

		pRenderTarget->FillRectangle(
			D2D1::RectF(0.0f, 0.0f, renderTargetSize.width, renderTargetSize.height), pPatternBrush);

		if (bitmap2 != nullptr) {
			pRenderTarget->DrawBitmap(bitmap2, D2D1::RectF(0.0f, 0.0f, prefer_width, prefer_height));
		}
		hr = pRenderTarget->EndDraw();

		if (FAILED(hr)) {

			safe_release();
		}

	}

	if (hr == D2DERR_RECREATE_TARGET)
	{
		safe_release();
	}
	if (FAILED(hr)) {
		safe_release();
	}
	else
	{
		::ValidateRect(hWnd, NULL);
	}

	ReleaseMutex(g_image_rotation_mutex);
}

void step(ptr lpParam) {

	WaitForSingleObject(g_image_rotation_mutex, INFINITE);

	if (Sprocedurep(lpParam)) {
		Scall0(lpParam);
	}

	ReleaseMutex(g_image_rotation_mutex);
}

ptr step_func(ptr lpParam) {

	WaitForSingleObject(g_image_rotation_mutex, INFINITE);
	if (Sprocedurep(lpParam)) {
		Scall0(lpParam);
	}
	ReleaseMutex(g_image_rotation_mutex);
	if(main_window!=nullptr)
		PostMessageW(main_window, WM_USER + 501, (WPARAM)0, (LPARAM)0);
	return Strue;
}

// constructor
CD2DView::CD2DView()
{
}

// destructor
CD2DView::~CD2DView()
{
	safe_release();
}

// Create the resources used by OnRender
HRESULT CD2DView::CreateDeviceResources()
{
    HRESULT hr = S_OK;
    return hr;
}

void CD2DView::DiscardDeviceResources()
{
	safe_release();
}

HRESULT CD2DView::OnRender()
{
    HRESULT hr = S_OK;
    onPaint(GetHwnd());
    return 0;
}


void CD2DView::OnResize(UINT width, UINT height)
{
  
}

void CD2DView::Stop()
{	
	safe_release();
	ReleaseMutex(g_image_rotation_mutex);
}



void scan_keys();
void CD2DView::Step(ptr n)
{
	scan_keys();
	step(n);
}

void CD2DView::Swap(int n)
{
	if( n==2) render_sprite_commands();
	swap_buffers(n);
}


void CD2DView::PreCreate(CREATESTRUCT& cs)
{
    cs.cx = 640;
    cs.cy = 480;
}

void CD2DView::PreRegisterClass(WNDCLASS& wc)
{
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor = ::LoadCursor(NULL, IDI_APPLICATION);
    wc.lpszClassName = _T("Direct2D");
}

int CD2DView::OnCreate(CREATESTRUCT& cs)
{
    UNREFERENCED_PARAMETER(cs);

    // Set the window's icon
    SetIconSmall(IDW_MAIN);
    SetIconLarge(IDW_MAIN);
    return 0;
}

void CD2DView::OnDestroy()
{
	safe_release();
  
}

LRESULT CD2DView::OnPaint(UINT, WPARAM, LPARAM)
{
    if (GetHwnd() != nullptr) {
        OnRender();
        ValidateRect();
    }
    return 0;
}

LRESULT CD2DView::OnSize(UINT, WPARAM, LPARAM lparam)
{
    UINT width = LOWORD(lparam);
    UINT height = HIWORD(lparam);
    OnResize(width, height);

    return 0;
}

LRESULT CD2DView::OnDisplayChange(UINT, WPARAM, LPARAM)
{
    Invalidate();
    return 0;
}

struct kp {
    int when;
    boolean left;
    boolean right;
    boolean up;
    boolean down;
    boolean ctrl;
    boolean space;
    int key_code;
} graphics_keypressed;

ptr cons_sbool(const char* symbol, bool value, ptr l)
{
    ptr a = Snil;
    if (value) {
        a = CALL2("cons", Sstring_to_symbol(symbol), Strue);
    }
    else
    {
        a = CALL2("cons", Sstring_to_symbol(symbol), Sfalse);
    }
    l = CALL2("cons", a, l);
    return l;
}
ptr cons_sfixnum(const char* symbol, const int value, ptr l)
{
    ptr a = Snil;
    a = CALL2("cons", Sstring_to_symbol(symbol), Sfixnum(value));
    l = CALL2("cons", a, l);
    return l;
}

int debounce_delay = 80;
int debounce = 0;

void scan_keys() {
	if (debounce_delay == 0) return;
	if (GetTickCount() - debounce > debounce_delay) {
		if (GetAsyncKeyState(VK_LEFT) != 0)
			graphics_keypressed.left = true;
		if (GetAsyncKeyState(VK_RIGHT) != 0)
			graphics_keypressed.right = true;
		if (GetAsyncKeyState(VK_UP) != 0)
			graphics_keypressed.up = true;
		if (GetAsyncKeyState(VK_DOWN) != 0)
			graphics_keypressed.up = true;
		if (GetAsyncKeyState(VK_SPACE) != 0)
			graphics_keypressed.space = true;
		if (GetAsyncKeyState(VK_CONTROL) != 0)
			graphics_keypressed.ctrl = true;
		debounce = GetTickCount();
	}
}

ptr graphics_keys(void) {

	ptr a = Snil;
	a = cons_sbool("left", graphics_keypressed.left, a);
	a = cons_sbool("right", graphics_keypressed.right, a);
	a = cons_sbool("up", graphics_keypressed.up, a);
	a = cons_sbool("down", graphics_keypressed.down, a);
	a = cons_sbool("ctrl", graphics_keypressed.ctrl, a);
	a = cons_sbool("space", graphics_keypressed.space, a);
	a = cons_sfixnum("key", graphics_keypressed.key_code, a);
	a = cons_sfixnum("recent", GetTickCount() - graphics_keypressed.when, a);

	graphics_keypressed.ctrl = false;
	graphics_keypressed.left = false;
	graphics_keypressed.right = false;
	graphics_keypressed.down = false;
	graphics_keypressed.up = false;
	graphics_keypressed.space = false;
	graphics_keypressed.key_code = 0;
	graphics_keypressed.when = GetTickCount();

	return a;
}

ptr keyboard_debounce(int n) {
	debounce_delay = n;
	return Snil;
}

LRESULT CD2DView::WndProc(UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        graphics_keypressed.key_code = wparam;
        graphics_keypressed.when = GetTickCount();
        break;

    case WM_DISPLAYCHANGE: 
		return OnDisplayChange(msg, wparam, lparam);
    case WM_SIZE:           return OnSize(msg, wparam, lparam);
    }

    return WndProcDefault(msg, wparam, lparam);
}


void add_d2d_commands() {

	Sforeign_symbol("d2d_matrix_identity", static_cast<ptr>(d2d_matrix_identity));
	Sforeign_symbol("d2d_matrix_rotate", static_cast<ptr>(d2d_matrix_rotate));
	Sforeign_symbol("d2d_render_sprite", static_cast<ptr>(d2d_render_sprite));
	Sforeign_symbol("d2d_render_sprite_rotscale", static_cast<ptr>(d2d_render_sprite_rotscale));
	Sforeign_symbol("d2d_render_sprite_sheet", static_cast<ptr>(d2d_render_sprite_sheet));
	Sforeign_symbol("d2d_render_sprite_sheet_rot_scale", static_cast<ptr>(d2d_render_sprite_sheet_rot_scale));
	Sforeign_symbol("sprite_size", static_cast<ptr>(sprite_size));
	Sforeign_symbol("d2d_load_sprites", static_cast<ptr>(d2d_load_sprites));
	Sforeign_symbol("d2d_FreeAllSprites", static_cast<ptr>(d2d_FreeAllSprites));
	Sforeign_symbol("d2d_FreeSpriteInBank", static_cast<ptr>(d2d_FreeSpriteInBank));
	Sforeign_symbol("d2d_MakeSpriteInBank", static_cast<ptr>(d2d_MakeSpriteInBank));
	Sforeign_symbol("d2d_write_text", static_cast<ptr>(d2d_write_text));
	Sforeign_symbol("d2d_zwrite_text", static_cast<ptr>(d2d_zwrite_text));
	Sforeign_symbol("d2d_text_mode", static_cast<ptr>(d2d_text_mode));
	Sforeign_symbol("d2d_set_font", static_cast<ptr>(d2d_set_font));
	Sforeign_symbol("d2d_color", static_cast<ptr>(d2d_color));
	Sforeign_symbol("d2d_fill_color", static_cast<ptr>(d2d_fill_color));
	Sforeign_symbol("d2d_linear_gradient_color", static_cast<ptr>(d2d_linear_gradient_color));
	Sforeign_symbol("d2d_radial_gradient_color", static_cast<ptr>(d2d_radial_gradient_color));
	Sforeign_symbol("d2d_rectangle", static_cast<ptr>(d2d_rectangle));
	Sforeign_symbol("d2d_zrectangle", static_cast<ptr>(d2d_zrectangle));
	Sforeign_symbol("d2d_fill_rectangle", static_cast<ptr>(d2d_fill_rectangle));
	Sforeign_symbol("d2d_zfill_rectangle", static_cast<ptr>(d2d_zfill_rectangle));
	Sforeign_symbol("d2d_zradial_gradient_fill_rectangle", static_cast<ptr>(d2d_zradial_gradient_fill_rectangle));
	Sforeign_symbol("d2d_zradial_gradient_fill_ellipse", static_cast<ptr>(d2d_zradial_gradient_fill_ellipse));
	Sforeign_symbol("d2d_radial_gradient_fill_rectangle", static_cast<ptr>(d2d_radial_gradient_fill_rectangle));
	Sforeign_symbol("d2d_radial_gradient_fill_ellipse", static_cast<ptr>(d2d_radial_gradient_fill_ellipse));
	Sforeign_symbol("d2d_zlinear_gradient_fill_rectangle", static_cast<ptr>(d2d_zlinear_gradient_fill_rectangle));
	Sforeign_symbol("d2d_zlinear_gradient_fill_ellipse", static_cast<ptr>(d2d_zlinear_gradient_fill_ellipse));
	Sforeign_symbol("d2d_linear_gradient_fill_rectangle", static_cast<ptr>(d2d_linear_gradient_fill_rectangle));
	Sforeign_symbol("d2d_linear_gradient_fill_ellipse", static_cast<ptr>(d2d_linear_gradient_fill_ellipse));
	Sforeign_symbol("d2d_ellipse", static_cast<ptr>(d2d_ellipse));
	Sforeign_symbol("d2d_zellipse", static_cast<ptr>(d2d_zellipse));
	Sforeign_symbol("d2d_fill_ellipse", static_cast<ptr>(d2d_fill_ellipse));
	Sforeign_symbol("d2d_zfill_ellipse", static_cast<ptr>(d2d_zfill_ellipse));
	Sforeign_symbol("d2d_line", static_cast<ptr>(d2d_line));
	Sforeign_symbol("d2d_zline", static_cast<ptr>(d2d_zline));
	Sforeign_symbol("d2d_render", static_cast<ptr>(d2d_render));
	Sforeign_symbol("d2d_show", static_cast<ptr>(d2d_show));
	Sforeign_symbol("d2d_save", static_cast<ptr>(d2d_save)); 
	Sforeign_symbol("d2d_clear", static_cast<ptr>(d2d_clear));
	Sforeign_symbol("d2d_zclear", static_cast<ptr>(d2d_zclear));
	Sforeign_symbol("d2d_set_stroke_width", static_cast<ptr>(d2d_set_stroke_width));
	Sforeign_symbol("d2d_image_size", static_cast<ptr>(d2d_image_size));
	Sforeign_symbol("d2d_release", static_cast<ptr>(d2d_release));
	Sforeign_symbol("d2d_draw_func", static_cast<ptr>(d2d_draw_func));
	Sforeign_symbol("step", static_cast<ptr>(step_func));
	Sforeign_symbol("add_clear_image", static_cast<ptr>(add_clear_image));
	Sforeign_symbol("add_write_text", static_cast<ptr>(add_write_text));
	Sforeign_symbol("add_draw_sprite", static_cast<ptr>(add_draw_sprite));
	Sforeign_symbol("add_scaled_rotated_sprite", static_cast<ptr>(add_scaled_rotated_sprite));
	Sforeign_symbol("set_draw_sprite", static_cast<ptr>(set_draw_sprite));
	Sforeign_symbol("clear_draw_sprite", static_cast<ptr>(clear_draw_sprite));
	Sforeign_symbol("clear_all_draw_sprite", static_cast<ptr>(clear_all_draw_sprite));
	Sforeign_symbol("add_ellipse", static_cast<ptr>(add_ellipse));
	Sforeign_symbol("add_fill_colour", static_cast<ptr>(add_fill_colour));
	Sforeign_symbol("add_line_colour", static_cast<ptr>(add_line_colour));
	Sforeign_symbol("add_fill_ellipse", static_cast<ptr>(add_fill_ellipse));
	Sforeign_symbol("add_draw_rect", static_cast<ptr>(add_draw_rect));
	Sforeign_symbol("add_fill_rect", static_cast<ptr>(add_fill_rect));
	Sforeign_symbol("add_pen_width", static_cast<ptr>(add_pen_width));
	Sforeign_symbol("d2d_zmatrix_skew", static_cast<ptr>(d2d_zmatrix_skew));
	Sforeign_symbol("d2d_zmatrix_translate", static_cast<ptr>(d2d_zmatrix_translate));
	Sforeign_symbol("d2d_zmatrix_transrot", static_cast<ptr>(d2d_zmatrix_transrot));
	Sforeign_symbol("d2d_zmatrix_rotrans", static_cast<ptr>(d2d_zmatrix_identity));
	Sforeign_symbol("d2d_zmatrix_identity", static_cast<ptr>(d2d_zmatrix_rotrans));
	Sforeign_symbol("graphics_keys", static_cast<ptr>(graphics_keys));
	Sforeign_symbol("keyboard_debounce", static_cast<ptr>(keyboard_debounce));
}

void CD2DView::AddCommands()
{
	if (g_image_rotation_mutex == nullptr) {
		g_image_rotation_mutex = CreateMutex(nullptr, FALSE, nullptr);
	}
	if (g_sprite_commands_mutex == nullptr) {
		g_sprite_commands_mutex = CreateMutex(nullptr, FALSE, nullptr);
	}
	add_d2d_commands();
}

void CD2DView::ScanKeys()
{
	scan_keys();
}
