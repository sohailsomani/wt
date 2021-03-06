/*
 * Copyright (C) 2008, 2011 Emweb bvba, Kessel-Lo, Belgium.
 *
 * See the LICENSE file for terms of use.
 */
#include "Wt/WApplication"
#include "Wt/WContainerWidget"
#include "Wt/WEnvironment"
#include "Wt/WPaintDevice"
#include "Wt/WPaintedWidget"
#include "Wt/WPainter"
#include "Wt/WSlider"

#include "DomElement.h"
#include "WebUtils.h"

namespace Wt {

const Wt::WFlags<WSlider::TickPosition> WSlider::NoTicks = 0;
const Wt::WFlags<WSlider::TickPosition> WSlider::TicksBothSides
  = TicksAbove | TicksBelow;

class PaintedSlider : public WPaintedWidget
{
public:
  PaintedSlider(WSlider *slider);

  void updateState();
  void updateSliderPosition();
  void doUpdateDom(DomElement& element, bool all);

  void sliderResized(const WLength& width, const WLength& height);
  virtual void propagateSetEnabled(bool enabled);

protected:
  void paintEvent(WPaintDevice *paintDevice);

private:
  WSlider *slider_;

  JSignal<int> sliderReleased_;
  JSlot mouseDownJS_, mouseMovedJS_, mouseUpJS_;

  WInteractWidget *handle_, *fill_;

  int range() const { return slider_->maximum() - slider_->minimum(); }
  double w() const;
  double h() const;

  void onSliderClick(const WMouseEvent& event);
  void onSliderReleased(int u);
};

void PaintedSlider::paintEvent(WPaintDevice *paintDevice)
{
  int tickInterval = slider_->tickInterval();
  int r = range();

  if (tickInterval == 0)
    tickInterval = r / 2;

  int numTicks = r / tickInterval + 1;
  if (numTicks < 1)
    return;

  int w = 0, h = 0;

  switch (slider_->orientation()) {
  case Horizontal:
    w = (int)paintDevice->width().toPixels();
    h = (int)paintDevice->height().toPixels();
    break;
  case Vertical:
    w = (int)paintDevice->height().toPixels();
    h = (int)paintDevice->width().toPixels();
  }

  double tickStep = ((double)w + 10 - slider_->handleWidth()) / (numTicks - 1);

  WPainter painter(paintDevice);

  for (int i = 0; i < numTicks; ++i) {
    int v = slider_->minimum() + i * tickInterval;
    int x = -5 + slider_->handleWidth()/2 + (int) (i * tickStep);

    switch (slider_->orientation()) {
    case Horizontal:
      slider_->paintTick(painter, v, x, h/2);
      break;
    case Vertical:
      slider_->paintTick(painter, v, h/2, w - x);
    }
  }
}

PaintedSlider::PaintedSlider(WSlider *slider)
  : WPaintedWidget(),
    slider_(slider),
    sliderReleased_(this, "released")
{
  setStyleClass("Wt-slider-bg");

  slider_->addStyleClass(std::string("Wt-slider-")
			 + (slider_->orientation() == Horizontal ? "h" : "v"));

  if (slider_->positionScheme() == Static) {
    slider_->setPositionScheme(Relative);
    slider_->setOffsets(0, Left | Top);
  }

  addChild(handle_ = slider_->createHandle());
  addChild(fill_ = new WContainerWidget());

  fill_->setPositionScheme(Absolute);
  fill_->setStyleClass("fill");

  handle_->setPopup(true);
  handle_->setPositionScheme(Absolute);
  handle_->setStyleClass("handle");

  handle_->mouseWentDown().connect(mouseDownJS_);
  handle_->mouseMoved().connect(mouseMovedJS_);
  handle_->mouseWentUp().connect(mouseUpJS_);

  slider->clicked().connect(this, &PaintedSlider::onSliderClick);
  sliderReleased_.connect(this, &PaintedSlider::onSliderReleased);
}

void PaintedSlider::propagateSetEnabled(bool enabled)
{
  if (enabled) {
    removeStyleClass("Wt-disabled");
    slider_->removeStyleClass("Wt-disabled");
  } else {
    addStyleClass("Wt-disabled");
    slider_->addStyleClass("Wt-disabled");
  }

  WPaintedWidget::propagateSetEnabled(enabled);
}

double PaintedSlider::w() const
{
  return width().toPixels() + (slider_->orientation() == Horizontal ? 10 : 0);
}

double PaintedSlider::h() const
{
  return height().toPixels() + (slider_->orientation() == Vertical ? 10 : 0);
}

void PaintedSlider::updateState()
{
  bool rtl = WApplication::instance()->layoutDirection() == RightToLeft;

  std::string resourcesURL = WApplication::resourcesUrl();

  Orientation o = slider_->orientation();

  if (o == Horizontal) {
    handle_->resize(slider_->handleWidth(), h());
    handle_->setOffsets(0, Top);
  } else {
    handle_->resize(w(), slider_->handleWidth());
    handle_->setOffsets(0, Left);
  }

  double l = o == Horizontal ? w() : h();
  double pixelsPerUnit = (l - slider_->handleWidth()) / range();

  std::string dir;
  std::string size;
  if (o == Horizontal) {
    dir = rtl ? "right" : "left";
    size = "width";
  } else {
    dir = "top";
    size = "height";
  }
  std::string u = (o == Horizontal ? "x" : "y");
  std::string U = (o == Horizontal ? "X" : "Y");
  std::string maxS
    = boost::lexical_cast<std::string>(l - slider_->handleWidth());
  std::string ppU = boost::lexical_cast<std::string>(pixelsPerUnit);
  std::string minimumS = boost::lexical_cast<std::string>(slider_->minimum());
  std::string maximumS = boost::lexical_cast<std::string>(slider_->maximum());

  std::string width = boost::lexical_cast<std::string>(w());
  std::string horizontal = boost::lexical_cast<std::string>(o == Horizontal);

  /*
   * Note: cancelling the mouseDown event prevents the selection behaviour
   */
  std::string mouseDownJS = 
    """obj.setAttribute('down', " WT_CLASS ".widgetCoordinates(obj, event)."
    + u + "); "
    WT_CLASS ".cancelEvent(event);";

  // = 'u' position relative to background, corrected for slider
  std::string computeD =
    ""  "var objh = " + handle_->jsRef() + ","
    ""      "objf = " + fill_->jsRef() + ","
    ""      "objb = " + jsRef() + ","
    ""      "page_u = WT.pageCoordinates(event)." + u + ","
    ""      "widget_page_u = WT.widgetPageCoordinates(objb)." + u + ","
    ""      "pos = page_u - widget_page_u,"
    ""      "rtl = " + boost::lexical_cast<std::string>(rtl) + ","
    ""      "horizontal = " + horizontal + ";"
    ""  "if (rtl && horizontal)"
    ""  "  pos = " + width + " - pos;"
    ""  "var d = pos - down;";
  

  std::string mouseMovedJS = 
    """var down = obj.getAttribute('down');"
    """var WT = " WT_CLASS ";"
    """if (down != null && down != '') {"
    + computeD +
    ""  "d = Math.max(0, Math.min(d, " + maxS + "));"
    ""  "var v = Math.round(d/" + ppU + ");"
    ""  "var intd = v*" + ppU + ";"
    ""  "if (Math.abs(WT.pxself(objh, '" + dir + "') - intd) > 1) {"
    ""    "objf.style." + size + " = intd + 'px';" + 
    ""    "objh.style." + dir + " = intd + 'px';" +
    ""    "var vs = " + (o == Horizontal ? "v + " + minimumS
			 : maximumS + " - v") + ";"
    ""    "var f = objb.parentNode.onValueChange;" +
    ""    "if (f) f(vs);"
      + slider_->sliderMoved().createCall("vs") + 
    ""  "}"
    """}";

  std::string mouseUpJS = 
    """var down = obj.getAttribute('down');"
    """var WT = " WT_CLASS ";"
    """if (down != null && down != '') {"
    + computeD +
    """d += "
    + boost::lexical_cast<std::string>(slider_->handleWidth() / 2) + ";" +
    sliderReleased_.createCall("d") + 
    ""  "obj.removeAttribute('down');"
    """}";

  bool enabled = !slider_->isDisabled();
  
  mouseDownJS_.setJavaScript(std::string("function(obj, event) {") 
			     + (enabled ? mouseDownJS : "") 
			     + "}");
  mouseMovedJS_.setJavaScript(std::string("function(obj, event) {") 
			      + (enabled ? mouseMovedJS : "") 
			      + "}");
  mouseUpJS_.setJavaScript(std::string("function(obj, event) {") 
			   + (enabled ? mouseUpJS : "") 
			   + "}");

  update();
  updateSliderPosition();
}

void PaintedSlider::doUpdateDom(DomElement& element, bool all)
{
  if (all) {
    WApplication *app = WApplication::instance();

    element.addChild(createSDomElement(app));
    element.addChild(((WWebWidget *)handle_)->createSDomElement(app));
    element.addChild(((WWebWidget *)fill_)->createSDomElement(app));

    DomElement *west = DomElement::createNew(DomElement_DIV);
    west->setProperty(PropertyClass, "Wt-w");
    element.addChild(west);

    DomElement *east = DomElement::createNew(DomElement_DIV);
    east->setProperty(PropertyClass, "Wt-e");
    element.addChild(east);
  }
}

void PaintedSlider::sliderResized(const WLength& width, const WLength& height)
{
  if (slider_->orientation() == Horizontal) {
    WLength w = width;
    if (!w.isAuto())
      w = WLength(w.toPixels() - 10);

    resize(w, height);
  } else {
    WLength h = height;
    if (!h.isAuto())
      h = WLength(h.toPixels() - 10);

    resize(width, h);    
  }

  updateState();
}
 
void PaintedSlider::onSliderClick(const WMouseEvent& event)
{
  int x = event.widget().x;
  int y = event.widget().y;

  if (WApplication::instance()->layoutDirection() == RightToLeft)
    x = (int)(w() - x);

  onSliderReleased(slider_->orientation() == Horizontal ? x : y);
}

void PaintedSlider::onSliderReleased(int u)
{
  if (slider_->orientation() == Horizontal)
    u -= slider_->handleWidth() / 2;
  else
    u = (int)h() - (u + slider_->handleWidth() / 2);

  double l = (slider_->orientation() == Horizontal) ? w() : h();

  double pixelsPerUnit = (l - slider_->handleWidth()) / range();

  double v = std::max(slider_->minimum(),
		      std::min(slider_->maximum(),
			       slider_->minimum() 
			       + (int)((double)u / pixelsPerUnit + 0.5)));

  // TODO changed() ?
  slider_->sliderMoved().emit(static_cast<int>(v));

  slider_->setValue(static_cast<int>(v));
  slider_->valueChanged().emit(slider_->value());  

  updateSliderPosition();
}

void PaintedSlider::updateSliderPosition()
{
  double l = (slider_->orientation() == Horizontal) ? w() : h();
  double pixelsPerUnit = (l - slider_->handleWidth()) / range();

  double u = ((double)slider_->value() - slider_->minimum()) * pixelsPerUnit;

  if (slider_->orientation() == Horizontal) {
    handle_->setOffsets(u, Left);
    fill_->setWidth(u);
  } else {
    handle_->setOffsets(h() - slider_->handleWidth() - u, Top);
    fill_->setHeight(u);
  }
}

WSlider::WSlider(WContainerWidget *parent)
  : WFormWidget(parent),
    orientation_(Horizontal),
    tickInterval_(0),
    tickPosition_(0),
    preferNative_(false),
    changed_(false),
    changedConnected_(false),
    handleWidth_(20),
    minimum_(0),
    maximum_(99),
    value_(0),
    valueChanged_(this),
    sliderMoved_(this, "moved"),
    paintedSlider_(0)
{ 
  resize(150, 50);
}

WSlider::WSlider(Orientation orientation, WContainerWidget *parent)
  : WFormWidget(parent),
    orientation_(orientation),
    tickInterval_(0),
    tickPosition_(0),
    preferNative_(false),
    changed_(false),
    changedConnected_(false),
    handleWidth_(20),
    minimum_(0),
    maximum_(99),
    value_(0),
    valueChanged_(this),
    sliderMoved_(this, "moved"),
    paintedSlider_(0)
{ 
  if (orientation == Horizontal)
    resize(150, 50);
  else
    resize(50, 150);
}

WSlider::~WSlider()
{ }

void WSlider::setNativeControl(bool nativeControl)
{
  preferNative_ = nativeControl;
}

bool WSlider::nativeControl() const
{
  if (preferNative_) {
    const WEnvironment& env = WApplication::instance()->environment();
    if ((env.agentIsChrome() && env.agent() >= WEnvironment::Chrome5)
	|| (env.agentIsSafari() && env.agent() >= WEnvironment::Safari4)
	|| (env.agentIsOpera() && env.agent() >= WEnvironment::Opera10))
      return true;
  }

  return false;
}

void WSlider::resize(const WLength& width, const WLength& height)
{
  WFormWidget::resize(width, height);

  if (paintedSlider_)
    paintedSlider_->sliderResized(width, height);
}

void WSlider::layoutSizeChanged(int width, int height)
{
  WFormWidget::resize(WLength::Auto, WLength::Auto);

  if (paintedSlider_) {
    paintedSlider_->sliderResized(width, height);
  }
}

void WSlider::setOrientation(Orientation orientation)
{
  orientation_ = orientation;

  if (paintedSlider_)
    paintedSlider_->updateState();
}

void WSlider::setTickPosition(WFlags<TickPosition> tickPosition)
{
  tickPosition_ = tickPosition;

  if (paintedSlider_)
    paintedSlider_->updateState();
}

void WSlider::setTickInterval(int tickInterval)
{
  tickInterval_ = tickInterval;

  if (paintedSlider_)
    paintedSlider_->updateState();
}

void WSlider::setHandleWidth(int handleWidth)
{
  handleWidth_ = handleWidth;
}

WInteractWidget *WSlider::createHandle()
{
  return new WContainerWidget();
}

void WSlider::update()
{
  if (paintedSlider_)
    paintedSlider_->updateState();
  else {
    changed_ = true;
    repaint();
  }
}

void WSlider::setMinimum(int minimum)
{
  minimum_ = minimum;
  value_ = std::max(minimum_, value_);
  maximum_ = std::max(minimum_ + 1, maximum_);

  update();
}

void WSlider::setMaximum(int maximum)
{
  maximum_ = maximum;
  value_ = std::min(maximum_, value_);
  minimum_ = std::min(maximum_ - 1, minimum_);

  update();
}

void WSlider::setRange(int minimum, int maximum)
{
  minimum_ = minimum;
  maximum_ = maximum;
  value_ = std::min(maximum_, std::max(minimum_, value_));

  update();
}

void WSlider::setValue(int value)
{
  value_ = std::min(maximum_, std::max(minimum_, value));

  if (paintedSlider_)
    paintedSlider_->updateSliderPosition();
  else
    update();
}

void WSlider::signalConnectionsChanged()
{
  WFormWidget::signalConnectionsChanged();

  update();
}

void WSlider::onChange()
{
  valueChanged_.emit(value_);
  sliderMoved_.emit(value_);
}

DomElementType WSlider::domElementType() const
{
  return paintedSlider_ ? DomElement_DIV : DomElement_INPUT;
}

void WSlider::render(WFlags<RenderFlag> flags)
{
  /*
   * In theory we are a bit late here to decide what we want to become:
   * somebody could already have asked the domElementType()
   */
  if (flags & RenderFull) {
    bool useNative = nativeControl();

    if (!useNative) {
      if (!paintedSlider_) {
	addChild(paintedSlider_ = new PaintedSlider(this));
	paintedSlider_->sliderResized(width(), height());
      }
    } else {
      delete paintedSlider_;
      paintedSlider_ = 0;
    }

    setLayoutSizeAware(!useNative);
    setFormObject(useNative);
  }

  WFormWidget::render(flags);
}

void WSlider::updateDom(DomElement& element, bool all)
{
  if (paintedSlider_)
    paintedSlider_->doUpdateDom(element, all);
  else {
    if (all || changed_) {
      element.setAttribute("type", "range");
      element.setProperty(Wt::PropertyValue,
			  boost::lexical_cast<std::string>(value_));
      element.setAttribute("min",
			   boost::lexical_cast<std::string>(minimum_));
      element.setAttribute("max",
			   boost::lexical_cast<std::string>(maximum_));

      if (!changedConnected_
	  && (valueChanged_.isConnected() || sliderMoved_.isConnected())) {
	changedConnected_ = true;
	changed().connect(this, &WSlider::onChange);
      }

      changed_ = false;
    }
  }

  WFormWidget::updateDom(element, all);
}

void WSlider::setFormData(const FormData& formData)
{
  // if the value was updated through the API, then ignore the update from
  // the browser, this happens when an action generated multiple events,
  // and we do not want to revert the changes made through the API
  if (changed_ || isReadOnly())
    return;

  if (!Utils::isEmpty(formData.values)) {
    const std::string& value = formData.values[0];
    try {
      value_ = boost::lexical_cast<int>(value);
    } catch (boost::bad_lexical_cast& e) { }
  }
}

WT_USTRING WSlider::valueText() const
{
  return WT_USTRING::fromUTF8(boost::lexical_cast<std::string>(value_));
}

void WSlider::setValueText(const WT_USTRING& value)
{
  try {
    value_ = boost::lexical_cast<int>(value);
  } catch (boost::bad_lexical_cast& e) { }
}

void WSlider::setDisabled(bool disabled)
{
  if (paintedSlider_)
    paintedSlider_->setDisabled(disabled);

  WFormWidget::setDisabled(disabled);
}

void WSlider::paintTick(WPainter& painter, int value, int x, int y)
{
  if (tickPosition_) {
    int h = 0;

    if (orientation_ == Horizontal)
      h = (int)painter.device()->height().toPixels();
    else
      h = (int)painter.device()->width().toPixels();

    WPen pen;
    pen.setColor(WColor(0xd7, 0xd7, 0xd7));
    pen.setCapStyle(FlatCap);
    pen.setWidth(1);
    painter.setPen(pen);

    int y1 = h / 4;
    int y2 = h / 2 - 4;
    int y3 = h / 2 + 4;
    int y4 = h - h/4;

    switch (orientation_) {
    case Horizontal:
      if (tickPosition_ & WSlider::TicksAbove)
	painter.drawLine(x + 0.5, y1, x + 0.5, y2);
      if (tickPosition_ & WSlider::TicksBelow)
	painter.drawLine(x + 0.5, y3, x + 0.5, y4);

      break;
    case Vertical:
      if (tickPosition_ & WSlider::TicksAbove)
	painter.drawLine(y1, y + 0.5, y2, y + 0.5);
      if (tickPosition_ & WSlider::TicksBelow)
	painter.drawLine(y3, y + 0.5, y4, y + 0.5);
    }
  }
}

}
