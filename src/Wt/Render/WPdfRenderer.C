/*
 * Copyright (C) 2010 Emweb bvba, Heverlee, Belgium.
 *
 * See the LICENSE file for terms of use.
 */

#include "Wt/WLogger"
#include "Wt/WPainter"
#include "Wt/WPdfImage"
#include "Wt/Render/WPdfRenderer"
#include "Wt/Render/RenderUtils.h"

#include <hpdf.h>

namespace Wt {

LOGGER("Render.WPdfRendererer");

  namespace Render {

WPdfRenderer::WPdfRenderer(HPDF_Doc pdf, HPDF_Page page)
  : pdf_(pdf),
    page_(page),
    dpi_(72),
    painter_(0)
{
  for (int i = 0; i < 4; ++i)
    margin_[i] = 0;
}

WPdfRenderer::~WPdfRenderer()
{ }

void WPdfRenderer::setDpi(int dpi)
{
  dpi_ = dpi;
}

void WPdfRenderer::setMargin(double margin, WFlags<Side> sides)
{
  if (sides & Top)
    margin_[0] = margin;
  if (sides & Right)
    margin_[1] = margin;
  if (sides & Bottom)
    margin_[2] = margin;
  if (sides & Left)
    margin_[3] = margin;
}

void WPdfRenderer::addFontCollection(const std::string& directory,
				     bool recursive)
{
  FontCollection c;
  c.directory = directory;
  c.recursive = recursive;

  fontCollections_.push_back(c);
}

HPDF_Page WPdfRenderer::createPage(int page)
{
#ifndef WT_TARGET_JAVA
  HPDF_Page result = HPDF_AddPage(pdf_);

  HPDF_Page_SetWidth(result, HPDF_Page_GetWidth(page_));
  HPDF_Page_SetHeight(result, HPDF_Page_GetHeight(page_));

  return result;
#else
  return Wt::Render::Utils::createPage(pdf_, 
				       HPDF_Page_GetWidth(page_), 
				       HPDF_Page_GetHeight(page_));
#endif
}

double WPdfRenderer::margin(Side side) const
{
  const double CmPerInch = 2.54;

  switch (side) {
  case Top:
    return margin_[0] / CmPerInch * dpi_;
  case Right:
    return margin_[1] / CmPerInch * dpi_;
  case Bottom:
    return margin_[2] / CmPerInch * dpi_;
  case Left:
    return margin_[3] / CmPerInch * dpi_;
  default:
    LOG_ERROR("margin(Side) with invalid side" << (int)side);
    return 0;
  }
}

double WPdfRenderer::pageWidth(int page) const
{
  return HPDF_Page_GetWidth(page_) * dpi_ / 72.0;
}
 
double WPdfRenderer::pageHeight(int page) const
{
  return HPDF_Page_GetHeight(page_) * dpi_ / 72.0;
}

WPaintDevice *WPdfRenderer::startPage(int page)
{
  if (page > 0)
    page_ = createPage(page);

#ifndef WT_TARGET_JAVA
  HPDF_Page_Concat (page_, 72.0f/dpi_, 0, 0, 72.0f/dpi_, 0, 0);
#endif

  WPdfImage *device = new WPdfImage(pdf_, page_, 0, 0,
				    pageWidth(page), pageHeight(page));
#ifdef WT_TARGET_JAVA
  WTransform deviceTransform;
  deviceTransform.scale(72.0f/dpi_, 72.0f/dpi_);
  device->setDeviceTransform(deviceTransform);
#endif //WT_TARGET_JAVA

  for (unsigned i = 0; i < fontCollections_.size(); ++i)
    device->addFontCollection(fontCollections_[i].directory,
			      fontCollections_[i].recursive);

  return device;
}

void WPdfRenderer::endPage(WPaintDevice *device)
{
  delete painter_;
  painter_ = 0;

  delete device;

#ifndef WT_TARGET_JAVA
  HPDF_Page_Concat (page_, dpi_/72.0f, 0, 0, dpi_/72.0f, 0, 0);
#endif
}

WPainter *WPdfRenderer::getPainter(WPaintDevice *device)
{
  if (!painter_)
    painter_ = new WPainter(device);

  return painter_;
}

  }
}
