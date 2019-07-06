#include <Nodal/NodeItem.hpp>
#include <Nodal/Process.hpp>
#include <Process/Style/ScenarioStyle.hpp>
#include <Process/ProcessFactory.hpp>
#include <Process/ProcessList.hpp>
#include <score/application/GUIApplicationContext.hpp>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <Control/DefaultEffectItem.hpp>
#include <score/document/DocumentContext.hpp>
#include <Process/Dataflow/PortFactory.hpp>
#include <Effect/EffectLayer.hpp>
#include <Process/Dataflow/PortItem.hpp>
#include <Process/Focus/FocusDispatcher.hpp>
#include <Nodal/Commands.hpp>
#include <Nodal/Process.hpp>
#include <score/selection/SelectionDispatcher.hpp>
#include <score/graphics/TextItem.hpp>
#include <QKeyEvent>
#include <wobjectimpl.h>
#include <score/command/Dispatchers/CommandDispatcher.hpp>
namespace Nodal
{

class TitleItem final
    : public QObject
    , public QGraphicsItem
{
  W_OBJECT(TitleItem)
public:
  TitleItem(
      Process::ProcessModel& effect,
      const score::DocumentContext& ctx,
      NodeItem& item,
      QObject* parent)
  : QObject{parent}
  , QGraphicsItem{&item}
  , m_effect{effect}
  , m_width{140}
  , m_item{item}
  {
    setFlag(ItemClipsChildrenToShape, true);

    const auto& skin = Process::Style::instance();

    if (auto ui_btn
        = Process::makeExternalUIButton(effect, ctx, this, this))
      ui_btn->setPos({5, 0});

    auto label = new score::SimpleTextItem{skin.IntervalBase, this};
    label->setText(effect.prettyShortName());
    label->setFont(skin.skin.Medium10Pt);
    label->setPos({30, 0});
  }

  void setPlayPercentage(float f)
  {
    m_playPercentage = f;
    update();
  }
  QRectF boundingRect() const override { return {0, 0, m_width, 15}; }
  void paint(
      QPainter* painter,
      const QStyleOptionGraphicsItem* option,
      QWidget* widget) override
  {
    static const auto pen = QPen{Qt::transparent};
    auto& style = Process::Style::instance();

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(pen);
    painter->setBrush(m_hover ? QBrush(style.RectPen.color().lighter(110)) : style.RectPen.brush());
    painter->drawRoundedRect(boundingRect(), 2, 2);
    painter->setRenderHint(QPainter::Antialiasing, false);
    if(m_playPercentage != 0.)
    {
      painter->setPen(style.IntervalPlayPen);
      painter->drawLine(QPointF{0.f, 14.f}, QPointF{m_width * m_playPercentage, 14.});
    }
  }

  void setWidth(qreal w) {
    prepareGeometryChange();
    m_width = w;
  }
  void setHighlight(bool b) {}
  void clicked() W_SIGNAL(clicked)


  void setHover(bool b)
  {
    m_hover = b;
    update();
  }

private:
  const Process::ProcessModel& m_effect;
  NodeItem& m_item;
  qreal m_width{};
  float m_playPercentage{};
  bool m_hover{};
};


void NodeItem::resetInlets(
    Process::ProcessModel& effect)
{
  qDeleteAll(m_inlets);
  m_inlets.clear();
  qreal x = 10;
  auto& portFactory
      = score::GUIAppContext().interfaces<Process::PortFactoryList>();
  for (Process::Inlet* port : effect.inlets())
  {
    if (port->hidden)
      continue;
    Process::PortFactory* fact = portFactory.get(port->concreteKey());
    Dataflow::PortItem* item = fact->makeItem(*port, m_context.context, this, this);
    item->setPos(x, -1.5);
    m_inlets.push_back(item);

    x += 10.;
  }
}

void NodeItem::resetOutlets(
    Process::ProcessModel& effect)
{
  qDeleteAll(m_outlets);
  m_outlets.clear();
  qreal x = 10;
  auto& portFactory
      = score::AppContext().interfaces<Process::PortFactoryList>();
  for (Process::Outlet* port : effect.outlets())
  {
    if (port->hidden)
      continue;
    Process::PortFactory* fact = portFactory.get(port->concreteKey());
    auto item = fact->makeItem(*port, m_context.context, this, this);
    item->setPos(x, boundingRect().height() - 5.);
    m_outlets.push_back(item);

    x += 10.;
  }
}

NodeItem::NodeItem(const Node& model, const Process::LayerContext& ctx, QGraphicsItem* parent)
  : QGraphicsItem{parent}
  , m_model{model}
  , m_context{ctx}
{
  setAcceptedMouseButtons(Qt::LeftButton);
  setAcceptHoverEvents(true);
  setFlag(ItemIsFocusable, true);

  // Title
  m_title = new TitleItem{model.process(), ctx.context, *this, this};
  m_title->setParentItem(this);

  // Body
  auto& fact = ctx.context.app.interfaces<Process::LayerFactoryList>();
  if (auto factory = fact.findDefaultFactory(model.process()))
  {
    if(auto fx = factory->makeItem(model.process(), ctx.context, this))
    {
        m_fx = fx;
        m_size = m_fx->boundingRect().size();
        connect(fx, &score::ResizeableItem::sizeChanged,
                this, [this] {
           prepareGeometryChange();
           const auto r = m_fx->boundingRect();
           m_title->setWidth(r.width());
           m_size = r.size();
           update();
        });
    }
    else if(auto fx = factory->makeLayerView(model.process(), this))
    {
      m_fx = fx;
      m_presenter = factory->makeLayerPresenter(model.process(), fx, ctx.context, this);
      m_size = m_model.size();
      m_presenter->setWidth(m_size.width(), m_size.width());
      m_presenter->setHeight(m_size.height());
      m_presenter->on_zoomRatioChanged(1.);
      m_presenter->parentGeometryChanged();
    }
  }

  if (!m_fx)
  {
    m_fx = new Media::Effect::DefaultEffectItem{model.process(), ctx.context, this};
    m_size = m_fx->boundingRect().size();
  }

  resetInlets(model.process());
  resetOutlets(model.process());

  // Positions / size
  m_fx->setPos({0, m_title->boundingRect().height()});
  m_title->setWidth(m_size.width());

  ::bind(model, Node::p_position{}, this, [this] (QPointF p) {
      if(p != pos())
          setPos(p);
  });

  ::bind(model, Node::p_size{}, this, [this] (QSizeF s) {
    if(s != m_size)
      setSize(s);
  });

  // Selection
  con(m_model.process().selection,
      &Selectable::changed,
      this,
      &NodeItem::setSelected);
}

void NodeItem::setSize(QSizeF sz)
{
  if(m_presenter)
  {
    // TODO: find a way to indicate to a model what will be the size of the item
    //      - maybe set it without a command in that case ?
    prepareGeometryChange();
    m_size = sz;
    m_title->setWidth(sz.width());

    m_presenter->setWidth(sz.width(), sz.width());
    m_presenter->setHeight(sz.height());
    m_presenter->on_zoomRatioChanged(m_ratio / sz.width());
    m_presenter->parentGeometryChanged();

    resetInlets(m_model.process());
    resetOutlets(m_model.process());
  }
}

const Id<Node>& NodeItem::id() const noexcept
{
  return m_model.id();
}

NodeItem::~NodeItem()
{
  delete m_presenter;
}

void NodeItem::setZoomRatio(ZoomRatio r)
{
  if(m_presenter)
  {
    m_ratio = r;
    m_presenter->on_zoomRatioChanged(m_ratio / m_size.width());
    // TODO investigate why this is necessary for scenario:
    m_presenter->parentGeometryChanged();
  }
}

void NodeItem::setPlayPercentage(float f)
{
  m_title->setPlayPercentage(f);
}

void NodeItem::setSelected(bool s)
{
  if(m_selected != s)
  {
    m_selected = s;
    if(s)
      setFocus();

    update();
  }
}

QRectF NodeItem::boundingRect() const
{
  const auto tsz = m_title->boundingRect();
  return {0, 0, m_size.width(), tsz.height() + m_size.height() + 10.};
}

bool NodeItem::isInSelectionCorner(QPointF p, QRectF r) const
{
  return p.x() > r.width() - 10. && p.y() > r.height() - 10.;
}

void NodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  auto& style = Process::Style::instance();

  QPen basePen = style.RectPen;
  if(m_selected)
    basePen.setColor(basePen.color().lighter(150));
  else if(m_hover)
    basePen.setColor(basePen.color().lighter(110));


  auto rect = boundingRect();

  painter->setPen(basePen);
  painter->setBrush(style.RectBrush);
  painter->drawRoundedRect(rect, 2., 2.);

  QBrush fillBrush = basePen.color();
  painter->setBrush(fillBrush);
  painter->drawRoundedRect(QRectF{0., rect.height() - 10., rect.width(), 10.}, 2., 2.);

  if(m_presenter)
  {
    QBrush b = basePen.brush();
    b.setColor(b.color().darker());
    b.setStyle(Qt::BrushStyle::BDiagPattern);
    painter->fillRect(QRectF{
                        rect.width() - 10.,
                        rect.height() - 10.,
                        10., 10.
                      },
                      b);
  }
}

namespace {
enum Interaction {
  Move, Resize
} nodeItemInteraction{};
QSizeF origNodeSize{};
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  if(m_presenter && isInSelectionCorner(event->pos(), boundingRect()))
  {
    nodeItemInteraction = Interaction::Resize;
    origNodeSize = m_model.size();
  }
  else
  {
    nodeItemInteraction = Interaction::Move;
  }

  if(m_presenter)
    m_context.context.focusDispatcher.focus(m_presenter);

  score::SelectionDispatcher{m_context.context.selectionStack}.setAndCommit({&m_model.process()});
  event->accept();
}

void NodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  auto origp = mapToItem(parentItem(), event->buttonDownPos(Qt::LeftButton));
  auto p = mapToItem(parentItem(), event->pos());
  switch(nodeItemInteraction)
  {
    case Interaction::Resize:
    {
      const auto sz = origNodeSize + QSizeF{p.x() - origp.x(), p.y() - origp.y()};
      m_context.context.dispatcher.submit<ResizeNode>(m_model, sz.expandedTo({10, 10}));
      break;
    }
    case Interaction::Move:
    {
      m_context.context.dispatcher.submit<MoveNode>(m_model, m_model.position() + (p - origp));
      break;
    }
  }
  event->accept();
}

void NodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  mouseMoveEvent(event);
  m_context.context.dispatcher.commit();
  event->accept();
}

void NodeItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
  if(isInSelectionCorner(event->pos(), boundingRect()))
  {
    setCursor(Qt::SizeFDiagCursor);
  }
  else
  {
    unsetCursor();
  }

  m_hover = true;
  m_title->setHover(true);
  update();
  event->accept();
}

void NodeItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
  if(isInSelectionCorner(event->pos(), boundingRect()))
  {
    setCursor(Qt::SizeFDiagCursor);
  }
  else
  {
    unsetCursor();
  }

}

void NodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
  unsetCursor();
  m_hover = false;
  m_title->setHover(false);
  update();
  event->accept();
}
}
W_OBJECT_IMPL(Nodal::TitleItem)

