#include <Nodal/NodeItem.hpp>
#include <Nodal/Process.hpp>
#include <Process/Style/ScenarioStyle.hpp>
#include <Process/ProcessFactory.hpp>
#include <Process/ProcessList.hpp>
#include <score/application/GUIApplicationContext.hpp>
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
#include <Effect/EffectPainting.hpp>
#include <wobjectimpl.h>
#include <score/tools/Bind.hpp>
#include <score/command/Dispatchers/CommandDispatcher.hpp>
#include <QPainter>
#include <QKeyEvent>
#include <QCursor>

namespace Nodal
{

void NodeItem::resetInlets(
    Process::ProcessModel& effect)
{
  qDeleteAll(m_inlets);
  m_inlets.clear();
  qreal x = InletX0;
  auto& portFactory
      = score::GUIAppContext().interfaces<Process::PortFactoryList>();
  for (Process::Inlet* port : effect.inlets())
  {
    if (port->hidden)
      continue;
    Process::PortFactory* fact = portFactory.get(port->concreteKey());
    Dataflow::PortItem* item = fact->makeItem(*port, m_context.context, this, this);
    item->setPos(x, InletY0);
    m_inlets.push_back(item);

    x += PortSpacing;
  }

  m_label->setPos(QPointF{x + 2., 0.});
}

void NodeItem::resetOutlets(
    Process::ProcessModel& effect)
{
  qDeleteAll(m_outlets);
  m_outlets.clear();
  qreal x = OutletX0;
  const qreal h = boundingRect().height() + OutletY0;
  auto& portFactory
      = score::AppContext().interfaces<Process::PortFactoryList>();
  for (Process::Outlet* port : effect.outlets())
  {
    if (port->hidden)
      continue;
    Process::PortFactory* fact = portFactory.get(port->concreteKey());
    auto item = fact->makeItem(*port, m_context.context, this, this);
    item->setPos(x, h);
    m_outlets.push_back(item);

    x += PortSpacing;
  }
}

NodeItem::NodeItem(const Node& model, const Process::LayerContext& ctx, QGraphicsItem* parent)
  : ItemBase{model.process(), ctx.context, parent}
  , m_model{model}
  , m_context{ctx}
{
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

  if(m_ui)
  {
    m_ui->setPos({m_size.width() + TopButtonX0, TopButtonY0});
  }

  // Positions / size
  m_fx->setPos({0, Effect::ItemBase::TitleHeight});

  ::bind(model, Node::p_position{}, this, [this] (QPointF p) {
      if(p != pos())
          setPos(p);
  });

  ::bind(model, Node::p_size{}, this, [this] (QSizeF s) {
    if(s != m_size)
      setSize(s);
  });
}

void NodeItem::setSize(QSizeF sz)
{
  if(m_presenter)
  {
    // TODO: find a way to indicate to a model what will be the size of the item
    //      - maybe set it without a command in that case ?
    prepareGeometryChange();
    m_size = sz;

    m_presenter->setWidth(sz.width(), sz.width());
    m_presenter->setHeight(sz.height());
    m_presenter->on_zoomRatioChanged(m_ratio / sz.width());
    m_presenter->parentGeometryChanged();

    resetInlets(m_model.process());
    resetOutlets(m_model.process());
    if(m_ui)
    {
      m_ui->setPos({m_size.width() + TopButtonX0, TopButtonY0});
    }
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
    if(r != m_ratio)
    {
      m_ratio = r;
      m_presenter->on_zoomRatioChanged(m_ratio / m_size.width());
      // TODO investigate why this is necessary for scenario:
      m_presenter->parentGeometryChanged();
    }
  }
}

void NodeItem::setPlayPercentage(float f)
{
  m_playPercentage = f;
  update();
}

bool NodeItem::isInSelectionCorner(QPointF p, QRectF r) const
{
  return p.x() > r.width() - 10. && p.y() > r.height() - 10.;
}

void NodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  const auto rect = boundingRect();

  ItemBase::paintNode(painter, m_selected, m_hover, rect);

  if(m_presenter)
  {
    auto& style = Process::Style::instance();
    static QBrush b = style.skin.Emphasis1.darker.brush; // TODO erk
    b.setStyle(Qt::BrushStyle::BDiagPattern);
    painter->fillRect(QRectF{
                        rect.width() - 10.,
                        rect.height() - 10.,
                        10., 10.
                      },
                      b);
  }

  // Exec
  if(m_playPercentage != 0.)
  {
    auto& style = Process::Style::instance();
    painter->setPen(style.IntervalSolidPen(style.IntervalPlayFill()));
    painter->drawLine(QPointF{0.f, 14.f}, QPointF{rect.width() * m_playPercentage, 14.});
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

  ItemBase::hoverEnterEvent(event);
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
  ItemBase::hoverMoveEvent(event);
}

void NodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
  unsetCursor();
  ItemBase::hoverLeaveEvent(event);
}
}
